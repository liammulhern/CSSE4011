from rest_framework import serializers
from notifications.models import ProductNotification, TrackerNotification
from supplychain.models import Tracker

class ProductNotificationSerializer(serializers.ModelSerializer):
    class Meta:
        model = ProductNotification
        fields = [
            'id',
            'notication_type',
            'productevent',
            'requirement',
            'order',
            'timestamp',
            'created_timestamp',
            'message',
            'acknowledged_timestamp',
            'acknowledged_by',
        ]
        read_only_fields = [
            'id',
            'timestamp',
            'created_timestamp',
            'acknowledged_timestamp',
            'acknowledged_by',
        ]

class TrackerNotificationSerializer(serializers.ModelSerializer):
    tracker = serializers.SlugRelatedField(
        queryset=Tracker.objects.all(),
        slug_field='tracker_key'
    )

    class Meta:
        model = TrackerNotification
        fields = [
            'id',
            'notication_type',
            'tracker',
            'timestamp',
            'created_timestamp',
            'message',
            'acknowledged_timestamp',
            'acknowledged_by',
        ]
        read_only_fields = [
            'id',
            'timestamp',
            'created_timestamp',
            'acknowledged_timestamp',
            'acknowledged_by',
        ]

class PublicProductNotificationSerializer(serializers.ModelSerializer):
    type = serializers.CharField(source="notication_type")
    message = serializers.CharField(source="message")
    timestamp = serializers.DateTimeField(
        format="%Y-%m-%dT%H:%M:%SZ",
        source="timestamp"
    )

    class Meta:
        model = ProductNotification
        fields = ("id", "type", "message", "timestamp")


class IoTHubMessageSerializer(serializers.Serializer):
    deviceID = serializers.CharField(max_length=128)
    messageType = serializers.CharField(max_length=64)
    # allow any JSON value (string, number, dict, list, etc.)
    message = serializers.JSONField(
        help_text="The actual content of the message; can be string, number, object, etc."
    )
