from rest_framework_api_key.permissions import HasAPIKey
from rest_framework.permissions import IsAuthenticated

from accounts.permissions import IsCompanyAdminOrReadOnly

from rest_framework.views import APIView
from rest_framework import viewsets, status, mixins, generics
from rest_framework.decorators import action
from rest_framework.response import Response

from notifications.models import ProductNotification
from notifications.serialisers import TrackerNotificationSerializer, ProductNotificationSerializer, PublicProductNotificationSerializer, IoTHubMessageSerializer
from notifications.scripts import azure_notification

class TrackerNotificationCreateAPIView(generics.CreateAPIView):
    """
    POST /api/tracker-notifications/
    Body: {
      "notication_type": "alert",
      "tracker": <tracker_id>,
      "message": "…"
    }
    """
    permission_classes = [IsCompanyAdminOrReadOnly | HasAPIKey]
    serializer_class = TrackerNotificationSerializer


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
    # permission_classes = [ IsCompanyAdminOrReadOnly | HasAPIKey ]
    permission_classes = [IsAuthenticated]

    def get_queryset(self):
        # only notifications for products in companies the user belongs to
        qs = ProductNotification.objects.all()

        user = self.request.user

        return qs.filter(
            productevent__product__owner__in=user.user_companies.filter(
                is_active=True
            ).values_list('company_id', flat=True)
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
        POST /api/productnotifications/{pk}/acknowledge/
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


class ApiKeyProductNotificationViewSet(viewsets.ReadOnlyModelViewSet):
    """
    GET  /api/public-notifications/        →  newest 3 notifications
    GET  /api/public-notifications/{pk}/   →  one notification
    """
    serializer_class = PublicProductNotificationSerializer
    permission_classes = [HasAPIKey]

    def get_queryset(self):
        # order newest first, then grab three
        return (
            ProductNotification.objects
            .order_by('-timestamp')[:3]
        )

class SendIoTHubMessageView(APIView):
    """
    POST /api/iot-hub/send/
    {
      "deviceID": "pathledger-gateway-uart-0",
      "messageType": "deviceIDUpdate",
      "message": 10
    }
    """
    permission_classes = [IsAuthenticated]

    def post(self, request, *args, **kwargs):
        serializer = IoTHubMessageSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)

        if serializer.validated_data is None:
            return Response(
                {"detail": f"Failed to set message"},
                status=status.HTTP_400_BAD_REQUEST
            )

        device_id = serializer.validated_data['deviceID']
        m_type = serializer.validated_data['messageType']
        m_payload = serializer.validated_data['message']

        # craft the envelope you want to send
        envelope = {
            "messageType": m_type,
            "message": m_payload
        }

        try:
            azure_notification.send_iot_hub_test_message(device_id, envelope)
        except Exception as exc:
            return Response(
                {"detail": f"Failed to send to IoT Hub: {str(exc)}"},
                status=status.HTTP_502_BAD_GATEWAY
            )

        return Response({"detail": "Message queued to IoT Hub."},
                        status=status.HTTP_202_ACCEPTED)

