from __future__ import annotations

from rest_framework import serializers

from supplychain.models import Product
from supplychain.models import ProductType
from supplychain.models import Company
from supplychain.models import ProductComposition

class ProductCompositionSerializer(serializers.ModelSerializer):
    """
    Show which components (and how many) go into a Product.
    """
    component_key = serializers.CharField(source='component.product_key', read_only=True)

    class Meta:
        model = ProductComposition
        fields = [
            'component',
            'component_key',
            'quantity',
        ]


class ProductSerializer(serializers.ModelSerializer):
    product_type = serializers.PrimaryKeyRelatedField(
        queryset=ProductType.objects.all(),
        allow_null=True,
        required=False,
    )
    owner = serializers.PrimaryKeyRelatedField(
        queryset=Company.objects.all(),
        allow_null=True,
        required=False,
    )
    recorded_by = serializers.PrimaryKeyRelatedField(
        read_only=True,
        default=serializers.CurrentUserDefault(),
    )
    components = ProductCompositionSerializer(
        source='productcomposition_set',
        many=True,
        read_only=True,
    )

    class Meta:
        model = Product
        fields = [
            'id',
            'product_key',
            'product_type',
            'batch',
            'owner',
            'created_timestamp',
            'components',
            'recorded_by',
        ]
        read_only_fields = ['created_timestamp']
