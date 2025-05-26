from rest_framework import generics
from rest_framework_api_key.permissions import HasAPIKey

from notifications.models import TrackerNotification, ProductNotification
from notifications.serialisers import TrackerNotificationSerializer, ProductNotificationSerializer

class TrackerNotificationCreateAPIView(generics.CreateAPIView):
    """
    POST /api/tracker-notifications/
    Body: {
      "notication_type": "alert",
      "tracker": <tracker_id>,
      "message": "…"
    }
    """
    permission_classes     = [HasAPIKey]
    serializer_class       = TrackerNotificationSerializer


class ProductNotificationCreateAPIView(generics.CreateAPIView):
    """
    POST /api/product-notifications/
    Body: {
      "notication_type": "alert",
      "productevent": "<uuid>",
      "requirement": <requirement_id>,
      "order": <order_id>,
      "message": "…"
    }
    """
    permission_classes     = [HasAPIKey]
    serializer_class       = ProductNotificationSerializer

