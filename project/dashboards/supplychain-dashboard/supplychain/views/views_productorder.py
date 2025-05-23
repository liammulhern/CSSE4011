from rest_framework import viewsets, permissions
from rest_framework.fields import timezone
from rest_framework.decorators import action

from supplychain.serialisers.serialiser_productorder import ProductOrderSerializer
from supplychain.models import ProductOrder

class ProductOrderViewSet(viewsets.ModelViewSet):
    """
        list / retrieve / create / update / delete ProductOrders
    """
    queryset = ProductOrder.objects.all().select_related('supplier', 'receiver', 'created_by')
    serializer_class = ProductOrderSerializer
    permission_classes = [permissions.IsAuthenticated]

    def perform_create(self, serializer):
        serializer.save(created_by=self.request.user)

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

