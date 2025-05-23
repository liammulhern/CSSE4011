from rest_framework import serializers

from supplychain.models import (
    ProductOrder,
    ProductOrderItem,
    ProductOrderRequirement,
    ProductOrderTracker,
    ProductOrderStatus,
    Company
)

from accounts.models import User

class ProductOrderItemSerializer(serializers.ModelSerializer):
    product_key = serializers.CharField(source='product.product_key', read_only=True)

    class Meta:
        model = ProductOrderItem
        fields = [
            'product',
            'product_key',
            'quantity',
        ]


class ProductOrderRequirementSerializer(serializers.ModelSerializer):
    requirement_name = serializers.CharField(source='requirement.name', read_only=True)
    unit = serializers.CharField(source='requirement.get_unit_display', read_only=True)

    class Meta:
        model = ProductOrderRequirement
        fields = [
            'requirement',
            'requirement_name',
            'unit',
            'assigned_timestamp',
        ]


class ProductOrderTrackerSerializer(serializers.ModelSerializer):
    tracker_identifier = serializers.CharField(source='tracker.identifier', read_only=True)

    class Meta:
        model = ProductOrderTracker
        fields = [
            'tracker',
            'tracker_identifier',
            'assigned_timestamp',
            'created_timestamp',
        ]


class ProductOrderStatusSerializer(serializers.ModelSerializer):
    created_by = serializers.PrimaryKeyRelatedField(read_only=True)

    class Meta:
        model = ProductOrderStatus
        fields = [
            'status',
            'timestamp',
            'created_by',
        ]


class ProductOrderSerializer(serializers.ModelSerializer):
    supplier = serializers.PrimaryKeyRelatedField(
        queryset=Company.objects.all()
    )
    receiver = serializers.PrimaryKeyRelatedField(
        queryset=Company.objects.all()
    )

    created_by = serializers.PrimaryKeyRelatedField(
        read_only=True,
        default=serializers.CurrentUserDefault(),
    )

    items = ProductOrderItemSerializer(source='items', many=True, read_only=True)
    order_requirements = ProductOrderRequirementSerializer(
        source='order_requirements', many=True, read_only=True
    )
    order_trackers = ProductOrderTrackerSerializer(
        source='order_trackers', many=True, read_only=True
    )
    status_history = ProductOrderStatusSerializer(
        source='status_history', many=True, read_only=True
    )

    current_status = serializers.SerializerMethodField()

    class Meta:
        model = ProductOrder
        fields = [
            'id',
            'supplier',
            'receiver',
            'order_timestamp',
            'delivery_location',
            'items',
            'order_requirements',
            'order_trackers',
            'status_history',
            'current_status',
            'created_timestamp',
            'created_by',
        ]
        read_only_fields = ['created_timestamp']

    def get_current_status(self, obj):
        return obj.current_status().value if obj.current_status() else None
