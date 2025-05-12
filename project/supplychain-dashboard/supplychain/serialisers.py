from __future__ import annotations

from rest_framework import serializers

from supplychain.models import Product
from supplychain.models import ProductType
from supplychain.models import ProductComposition


class ProductCompositionSerializer(serializers.ModelSerializer):
    class Meta:
        model = ProductComposition
        fields = ('component', 'quantity')


class ProductSerializer(serializers.ModelSerializer):
    components = ProductCompositionSerializer(
        source='composition_lines', many=True, required=False
    )

    class Meta:
        model = Product
        fields = (
            'id',
            'product_key',
            'product_type',
            'batch',
            'owner',
            'created_timestamp',
            'components',
        )
        read_only_fields = ('created_timestamp',)

    def create(self, validated_data):
        productcompositions = validated_data.pop('composition_lines', [])

        product = Product.objects.create(**validated_data)

        for productcomposition in productcompositions:
            ProductComposition.objects.create(
                parent=product,
                component=productcomposition['component'],
                quantity=productcomposition['quantity']
            )

        return product

    def update(self, instance, validated_data):
        # update simple fields
        for attr, val in validated_data.items():
            if attr != 'composition_lines':
                setattr(instance, attr, val)

        instance.save()

        return instance


class ProductTypeSerializer(serializers.ModelSerializer):
    class Meta:
        model = ProductType
        fields = (
            'id',
            'product_number',
            'name',
            'description',
            'created_timestamp',
            'owner',
            'recorded_by',
        )
        read_only_fields = ('created_timestamp',)

    def create(self, validated_data) -> ProductType:

        producttype = ProductType.objects.create(**validated_data)

        return producttype

    def update(self, instance, validated_data) -> ProductType:

        for attr, val in validated_data.items():
            setattr(instance, attr, val)

        instance.save()

        return instance



