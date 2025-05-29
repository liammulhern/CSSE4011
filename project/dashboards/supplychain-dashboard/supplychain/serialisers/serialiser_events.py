from rest_framework import serializers
from supplychain.models import TrackerEvent, ProductEvent

class TrackerEventSerializer(serializers.ModelSerializer):
    """
    Serializer for on-chain/off-chain events recorded by a tracker.
    """
    class Meta:
        model = TrackerEvent
        fields = [
            'message_id',
            'event_type',
            'payload',
            'timestamp',
            'data_hash',
            'block_id',
            'created_timestamp',
        ]
        read_only_fields = [
            'message_id',
            'created_timestamp',
        ]


class ProductEventSerializer(serializers.ModelSerializer):
    """
    Serializer for lifecycle events attached to a Product.
    """
    # link back to the TrackerEvent if present
    trackerevent = serializers.PrimaryKeyRelatedField(
        queryset=TrackerEvent.objects.all(),
        allow_null=True,
        required=False,
    )
    # record which user created this
    recorded_by = serializers.PrimaryKeyRelatedField(
        read_only=True,
        default=serializers.CurrentUserDefault()
    )

    class Meta:
        model = ProductEvent
        fields = [
            'message_id',
            'product',
            'trackerevent',
            'event_type',
            'payload',
            'timestamp',
            'recorded_by',
            'created_timestamp',
        ]
        read_only_fields = [
            'message_id',
            'created_timestamp',
            'recorded_by',
        ]


class EventFilterSerializer(serializers.Serializer):
    start = serializers.DateTimeField(
        help_text="Window start (ISO-8601)."
    )
    end   = serializers.DateTimeField(
        help_text="Window end (ISO-8601)."
    )


class VerifyHashInputSerializer(serializers.Serializer):
    """
    Input payload for block‐hash verification.
    """
    message_ids = serializers.ListField(
        child=serializers.UUIDField(),
        help_text="List of TrackerEvent or ProductEvent UUIDs to verify"
    )


class VerifyHashResultSerializer(serializers.Serializer):
    """
    Single verification result.
    """
    message_id = serializers.UUIDField()
    event_type = serializers.ChoiceField(choices=['tracker', 'product'])
    verified = serializers.BooleanField()
    error = serializers.CharField(required=False)

