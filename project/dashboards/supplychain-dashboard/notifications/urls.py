
from django.urls import path
from notifications.views import TrackerNotificationCreateAPIView, ProductNotificationCreateAPIView

urlpatterns = [
    path('notifications/tracker/', TrackerNotificationCreateAPIView.as_view(), name='create-tracker-notification'),
    path('notifications/productevent/', ProductNotificationCreateAPIView.as_view(), name='create-productevent-notification'),
]

