
from django.urls import path
from telemetry.views import TelemetryView

urlpatterns = [
    path('telemetry/gateway/', TelemetryView.as_view(), name='telemetry-gateway'),
]
