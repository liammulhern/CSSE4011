
from rest_framework.routers import DefaultRouter
from django.urls import path

from notifications.views import TrackerNotificationCreateAPIView, ProductNotificationViewSet

router = DefaultRouter()
router.register(r'notifications/productevent', ProductNotificationViewSet, basename='product-notification')

urlpatterns = [
    path('notifications/tracker/', TrackerNotificationCreateAPIView.as_view(), name='create-tracker-notification'),
] + router.urls

