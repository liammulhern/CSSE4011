from __future__ import annotations

from rest_framework import serializers

from supplychain.models import Product
from supplychain.models import ProductType
from supplychain.models import Company
from supplychain.models import ProductComposition

class ProductNestedSerializer(serializers.ModelSerializer):
    """
    A slimmed‐down ProductSerializer used only for nesting inside components
    (so we don’t recursively re‐nest forever).
    """
    class Meta:
        model = Product
        fields = [
            'id',
            'product_key',
            'product_type',
            'batch',
            'owner',
            'created_timestamp',
        ]


class ProductCompositionDetailSerializer(serializers.ModelSerializer):
    """
    Serializes each through‐row, nesting the component product.
    """
    component = ProductNestedSerializer(read_only=True)      # full product info

    class Meta:
        model = ProductComposition
        fields = ['component']


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
    owner_name = serializers.CharField(source='owner.name', read_only=True)
    recorded_by = serializers.PrimaryKeyRelatedField(
        read_only=True,
        default=serializers.CurrentUserDefault(),
    )

    components = ProductCompositionDetailSerializer(
        source='composition_lines',  # the related_name on through‐model FK: parent→composition_lines
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
            'owner_name',
            'created_timestamp',
            'recorded_by',
            'components',   # nested list of component products + qty
        ]
        read_only_fields = ['created_timestamp']

