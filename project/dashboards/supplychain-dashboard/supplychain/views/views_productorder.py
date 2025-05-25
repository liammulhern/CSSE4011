from rest_framework import viewsets, permissions, status
from rest_framework.fields import timezone
from rest_framework.decorators import action
from rest_framework.response import Response

from django_filters.rest_framework import DjangoFilterBackend
from django.utils.dateparse import parse_datetime
from django.db.models import Q

from accounts.permissions import IsCompanyAdminOrReadOnly

from datetime import timedelta

from supplychain.serialisers.serialiser_productorder import ProductOrderSerializer
from supplychain.models import ProductOrder, ProductOrderStatus, ProductEvent, TrackerEvent, ComplianceEvent

class ProductOrderViewSet(viewsets.ModelViewSet):
    """
        list / retrieve / create / update / delete ProductOrders
    """
    queryset = ProductOrder.objects.all().select_related('supplier', 'receiver', 'created_by')
    serializer_class = ProductOrderSerializer

    filter_backends = [DjangoFilterBackend]
    filterset_fields = {
        'order_timestamp': ['gte', 'lte'],
    }

    def perform_create(self, serializer):
        serializer.save(created_by=self.request.user)

    def get_queryset(self):
        qs = super().get_queryset()
        user = self.request.user

        # also include any objects explicitly assigned via `receiver`, etc.
        return qs.filter(
            Q(supplier_id__in=user.user_roles.values_list('role__company', flat=True)) |
            Q(receiver_id__in=user.user_roles.values_list('role__company', flat=True))
        )

    @action(detail=True, methods=['get'], url_path='events')
    def get_events(self, request, pk=None):
        """
            GET /product-orders/{pk}/events/

            Returns all ProductEvents and their linked TrackerEvents
            for products in this order, between order_timestamp and
            the timestamp when the order was marked DELIVERED.
        """
        order = self.get_object()

        # 1. Determine the delivery timestamp
        delivered_entry = (
            order.status_history
                 .filter(status=ProductOrderStatus.STATUS_DELIVERED)
                 .order_by('-timestamp')
                 .first()
        )

        start_ts = order.order_timestamp
        end_ts = timezone.now()

        if delivered_entry:
            end_ts = delivered_entry.timestamp

        # 2. Fetch all product-events in window
        product_ids = order.items.values_list('product_id', flat=True)
        prod_ev_qs = ProductEvent.objects.filter(
            product_id__in=product_ids,
            timestamp__gte=start_ts,
            timestamp__lte=end_ts,
        )

        prod_ev_ser = ProductEventSerializer(prod_ev_qs, many=True)

        # 3. Fetch tracker-events referenced by those product-events
        tracker_ids = order.order_trackers.values_list('tracker_id', flat=True)

        trk_ev_qs = TrackerEvent.objects.filter(
            message_id__in=tracker_ids,
            timestamp__gte=start_ts,
            timestamp__lte=end_ts,
        )

        trk_ev_ser = TrackerEventSerializer(trk_ev_qs, many=True)

        return Response({
            "order_id": order.pk,
            "time_window": {
                "from": start_ts,
                "to":   end_ts,
            },
            "product_events": prod_ev_ser.data,
            "tracker_events": trk_ev_ser.data,
        })

    @action(detail=False, methods=['get'], url_path='summary')
    def get_summary(self, request):
        """
        GET /api/product-orders/summary/?start=2025-05-01T00:00:00Z&end=2025-05-23T23:59:59Z
        Returns totals and deltas for orders and alerts.
        """
        user = self.request.user

        # parse dates
        end_str = request.query_params.get('end')
        start_str = request.query_params.get('start')

        try:
            end_dt = parse_datetime(end_str) if end_str else timezone.now()
            start_dt = parse_datetime(start_str) if start_str else end_dt - timedelta(days=1)
        except (ValueError, TypeError):
            return Response({"detail":"Invalid date format"}, status=status.HTTP_400_BAD_REQUEST)

        # Total orders up to each date
        total_end = ProductOrder.objects.filter(
            Q(supplier__in=user.user_roles.values_list('role__company', flat=True)) |
            Q(receiver__in=user.user_roles.values_list('role__company', flat=True))
        ).filter(
            order_timestamp__lte=end_dt
        ).count()

        total_start = ProductOrder.objects.filter(
            Q(supplier__in=user.user_roles.values_list('role__company', flat=True)) |
            Q(receiver__in=user.user_roles.values_list('role__company', flat=True))
        ).filter(
            order_timestamp__lte=start_dt
        ).count()

        total_delta = total_end - total_start

        # Delivered orders (distinct orders whose latest status ≤ each date is 'delivered')
        delivered_end = ProductOrderStatus.objects.filter(
            Q(order__supplier__in=user.user_roles.values_list('role__company', flat=True)) |
            Q(order__receiver__in=user.user_roles.values_list('role__company', flat=True))
        ).filter(
            status=ProductOrderStatus.STATUS_DELIVERED,
            timestamp__lte=end_dt
        ).values('order').distinct().count()

        delivered_start = ProductOrderStatus.objects.filter(
            Q(order__supplier__in=user.user_roles.values_list('role__company', flat=True)) |
            Q(order__receiver__in=user.user_roles.values_list('role__company', flat=True))
        ).filter(
            status=ProductOrderStatus.STATUS_DELIVERED,
            timestamp__lte=start_dt
        ).values('order').distinct().count()

        delivered_delta = delivered_end - delivered_start

        # In-transit = total – delivered
        in_transit_end = total_end - delivered_end
        in_transit_start = total_start - delivered_start
        in_transit_delta = in_transit_end - in_transit_start

        # Compliance alerts up to each date
        alert_end = ComplianceEvent.objects.filter(
            product__owner__in=user.user_roles.values_list('role__company', flat=True),
            timestamp__lte=end_dt
        ).count()

        alert_start = ComplianceEvent.objects.filter(
            product__owner__in=user.user_roles.values_list('role__company', flat=True),
            timestamp__lte=start_dt
        ).count()

        alert_delta = alert_end - alert_start

        return Response({
            "start": start_dt,
            "end":   end_dt,
            "total_orders": total_end,
            "total_orders_delta": total_delta,
            "delivered_orders": delivered_end,
            "delivered_orders_delta": delivered_delta,
            "in_transit_orders": in_transit_end,
            "in_transit_orders_delta": in_transit_delta,
            "compliance_alerts": alert_end,
            "compliance_alerts_delta": alert_delta,
        })
