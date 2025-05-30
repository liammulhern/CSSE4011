
from rest_framework.routers import DefaultRouter
from django.urls import path

from notifications.views import TrackerNotificationCreateAPIView, ProductNotificationViewSet, ApiKeyProductNotificationViewSet, SendIoTHubMessageView

router = DefaultRouter()
router.register(r'notifications/productevent', ProductNotificationViewSet, basename='product-notification')
router.register(r'notifications/public', ApiKeyProductNotificationViewSet, basename="public-notifications")

urlpatterns = [
    path('notifications/tracker/', TrackerNotificationCreateAPIView.as_view(), name='create-tracker-notification'),
    path('iot-hub/send/', SendIoTHubMessageView.as_view(), name='send-iot-hub-message'),
] + router.urls
