from django.contrib import admin

from supplychain.models import ProductEvent
@admin.register(ProductEvent)
class ProductEventAdmin(admin.ModelAdmin):
    list_display = ('id', 'product', 'event_type', 'timestamp', 'recorded_by')
    list_filter = ('event_type', 'timestamp', 'product')
    search_fields = ('data_hash',)


from supplychain.models import CustodyTransfer
@admin.register(CustodyTransfer)
class CustodyTransferAdmin(admin.ModelAdmin):
    list_display = ('product', 'from_company', 'to_company', 'timestamp', 'recorded_by')
    list_filter = ('from_company', 'to_company', 'timestamp')
    search_fields = ('product__product_key',)


from supplychain.models import ProductComposition
class ProductCompositionInline(admin.TabularInline):
    model = ProductComposition
    fk_name = 'parent'
    extra = 1

@admin.register(ProductComposition)
class ProductCompositionAdmin(admin.ModelAdmin):
    list_display = ('parent', 'component', 'quantity', 'created_timestamp')
    list_filter = ('parent', 'component')
    search_fields = ('parent__product_key', 'component__product_key')


from supplychain.models import Product
@admin.register(Product)
class ProductAdmin(admin.ModelAdmin):
    list_display = ('product_key', 'batch', 'owner', 'created_timestamp')
    inlines = [ProductCompositionInline]
    search_fields = ('product_key', 'batch')
    list_filter = ('owner',)


from supplychain.models import ProductOrderRequirement
@admin.register(ProductOrderRequirement)
class ProductOrderRequirementAdmin(admin.ModelAdmin):
    list_display = ('order', 'requirement', 'assigned_timestamp')
    list_filter = ('requirement', 'assigned_timestamp')
    search_fields = (
        'order__pk',
        'requirement__name',
    )


from supplychain.models import ProductOrderItem
@admin.register(ProductOrderItem)
class ProductOrderItemAdmin(admin.ModelAdmin):
    """
    Admin view for line‐items on a ProductOrder.
    """
    list_display = (
        'order',
        'product',
        'quantity',
    )
    list_filter = (
        'order',
        'product',
    )
    search_fields = (
        'order__id',
        'product__product_key',
    )
    raw_id_fields = (
        'order',
        'product',
    )
    ordering = (
        'order__order_timestamp',
        'product__product_key',
    )


class ProductOrderItemInline(admin.TabularInline):
    """
    Inline for line-items on a ProductOrder.
    """
    model = ProductOrderItem
    extra = 1
    raw_id_fields = ('product',)


class ProductOrderRequirementInline(admin.TabularInline):
    """
    Inline for requirements linked to a ProductOrder.
    """
    model = ProductOrderRequirement
    extra = 1
    raw_id_fields = ('requirement',)


from supplychain.models import ProductOrder
@admin.register(ProductOrder)
class ProductOrderAdmin(admin.ModelAdmin):
    """
    Admin view for ProductOrder.
    """
    list_display = ('id', 'supplier', 'receiver', 'order_timestamp', 'delivery_location')
    list_filter  = ('supplier', 'receiver', 'order_timestamp')
    search_fields = (
        'supplier__name',
        'receiver__name',
        'delivery_location',
    )
    inlines = [
        ProductOrderItemInline,
        ProductOrderRequirementInline,
    ]
