from rest_framework import serializers
from supplychain.models import Tracker, Company

class TrackerSerializer(serializers.ModelSerializer):
    owner = serializers.PrimaryKeyRelatedField(
        queryset=Company.objects.all(),
        allow_null=True,
        required=False,
        help_text="Current owning company (nullable)."
    )

    class Meta:
        model = Tracker
        fields = [
            'id',
            'tracker_key',
            'owner',
            'created_timestamp',
        ]
        read_only_fields = ['id', 'created_timestamp']

