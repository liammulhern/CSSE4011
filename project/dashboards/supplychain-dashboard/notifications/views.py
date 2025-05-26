from rest_framework_api_key.permissions import HasAPIKey

from accounts.permissions import IsCompanyAdminOrReadOnly

from notifications.serialisers import TrackerNotificationSerializer, ProductNotificationSerializer

from rest_framework import viewsets, status, mixins, generics
from rest_framework.decorators import action
from rest_framework.response import Response

from notifications.models import ProductNotification
from notifications.serialisers import ProductNotificationSerializer



class TrackerNotificationCreateAPIView(generics.CreateAPIView):
    """
    POST /api/tracker-notifications/
    Body: {
      "notication_type": "alert",
      "tracker": <tracker_id>,
      "message": "…"
    }
    """
    permission_classes     = [HasAPIKey | IsCompanyAdminOrReadOnly]
    serializer_class       = TrackerNotificationSerializer


class ProductNotificationViewSet(
    mixins.CreateModelMixin,
    mixins.ListModelMixin,
    mixins.RetrieveModelMixin,
    viewsets.GenericViewSet,
):
    """
    create: POST /api/product-notifications/
    list:   GET  /api/product-notifications/
    retrieve: GET /api/product-notifications/{pk}/
    """
    serializer_class = ProductNotificationSerializer
    permission_classes = [HasAPIKey | IsCompanyAdminOrReadOnly]

    def get_queryset(self):
        # only notifications for products in companies the user belongs to
        qs = ProductNotification.objects.all()
        user = self.request.user
        return qs.filter(
            productevent__product__owner__in=user.user_roles.values_list(
                'role__company', flat=True
            )
        )

    @action(detail=False, methods=['get'])
    def alerts(self, request):
        """
        GET /api/product-notifications/alerts/
        Returns only notifications of type 'alert'
        """
        qs = self.get_queryset().filter(
            notication_type=ProductNotification.NOTICATION_TYPE_ALERT
        )
        page = self.paginate_queryset(qs)
        if page is not None:
            serializer = self.get_serializer(page, many=True)
            return self.get_paginated_response(serializer.data)

        serializer = self.get_serializer(qs, many=True)
        return Response(serializer.data)

    @action(detail=True, methods=['post'])
    def acknowledge(self, request, pk=None):
        """
        POST /api/product-notifications/{pk}/acknowledge/
        Marks the notification as acknowledged by the current user.
        """
        notification = self.get_object()
        try:
            notification.acknowledge(request.user)
        except ValueError as e:
            return Response(
                {"detail": str(e)},
                status=status.HTTP_400_BAD_REQUEST
            )

        serializer = self.get_serializer(notification)
        return Response(serializer.data, status=status.HTTP_200_OK)

