from rest_framework import serializers

from supplychain.models import Product
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
            'batch',
            'owner',
            'created_timestamp',
            'components',
        )
        read_only_fields = ('created_timestamp',)

    def create(self, validated_data):
        comps = validated_data.pop('composition_lines', [])
        product = Product.objects.create(**validated_data)

        for comp in comps:
            ProductComposition.objects.create(
                parent=product,
                component=comp['component'],
                quantity=comp['quantity']
            )

        return product

    def update(self, instance, validated_data):
        # update simple fields
        for attr, val in validated_data.items():
            if attr != 'composition_lines':
                setattr(instance, attr, val)

        instance.save()

        return instance
