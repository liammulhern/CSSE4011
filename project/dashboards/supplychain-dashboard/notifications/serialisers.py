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

