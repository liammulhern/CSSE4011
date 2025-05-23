from django.contrib import admin
from django.utils import timezone

from supplychain.models import ProductEvent
@admin.register(ProductEvent)
class ProductEventAdmin(admin.ModelAdmin):
    list_display = ('message_id', 'product', 'event_type', 'timestamp', 'recorded_by')
    list_filter = ('event_type', 'timestamp', 'product')
    search_fields = ('data_hash',)


from supplychain.models import ProductType
@admin.register(ProductType)
class ProductTypeAdmin(admin.ModelAdmin):
    list_display = ('product_number', 'name', 'owner', 'recorded_by')
    list_filter = ('owner',)
    search_fields = ('product_number', 'name')

from supplychain.models import Gateway
@admin.register(Gateway)
class GatewayAdmin(admin.ModelAdmin):
    list_display = ('gateway_key', 'owner', 'created_timestamp')
    list_filter = ('owner',)
    search_fields = ('gateway_key',)


from supplychain.models import GatewayEventRaw
@admin.register(GatewayEventRaw)
class GatewayEventRaw(admin.ModelAdmin):
    list_display = ('message_id', 'gateway_id', 'message_type')
    list_filter = ('gateway_id', 'message_type')
    search_fields = ('gateway_id',)


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
    list_display = ('parent', 'component', 'created_timestamp')
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

from supplychain.models import ProductOrderStatus
@admin.register(ProductOrderStatus)
class ProductOrderStatusAdmin(admin.ModelAdmin):
    list_display = ('order', 'status', 'timestamp', 'created_by')
    list_filter = ('status', 'timestamp')
    search_fields = ('order__id', 'status')
    ordering = ('-timestamp',)
    autocomplete_fields = ('order', 'created_by')
    readonly_fields = ('created_by',)

    def save_model(self, request, obj, form, change):
        # On creation, set created_by to the current admin user
        if not change or obj.created_by is None:
            obj.created_by = request.user
        # Ensure timestamp is set if missing
        if not obj.timestamp:
            obj.timestamp = timezone.now()
        super().save_model(request, obj, form, change)


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

class ProductOrderStatusInline(admin.TabularInline):
    model = ProductOrderStatus
    extra = 0
    fields = ('status', 'timestamp', 'created_by')
    readonly_fields = ('created_by',)
    ordering = ('-timestamp',)
    show_change_link = True


from supplychain.models import ProductOrder
@admin.register(ProductOrder)
class ProductOrderAdmin(admin.ModelAdmin):
    list_display = ('id', 'supplier', 'receiver', 'order_timestamp', 'current_status')
    list_filter = ('supplier', 'receiver', 'status_history__status')
    search_fields = ('id', 'supplier__name', 'receiver__name')
    inlines = [
        ProductOrderItemInline,
        ProductOrderRequirementInline,
        ProductOrderStatusInline,
    ]
    readonly_fields = ('current_status',)
    fieldsets = (
        (None, {
            'fields': ('supplier', 'receiver', 'order_timestamp', 'delivery_location', 'created_by')
        }),
        ('Status', {
            'fields': ('current_status',),
            'description': 'Current status is computed from the latest status history entry.'
        }),
    )


from supplychain.models import SupplyChainRequirement
@admin.register(SupplyChainRequirement)
class SupplyChainRequirementAdmin(admin.ModelAdmin):
    """
    Admin view for supply-chain requirements.
    """
    list_display = (
        'name',
        'unit',
        'attribute_type',
        'company',
        'created_timestamp',
    )
    list_filter = (
        'attribute_type',
        'unit',
        'company',
        'created_timestamp',
    )
    search_fields = (
        'name',
        'company__name',
    )
    raw_id_fields = (
        'company',
    )
    date_hierarchy = 'created_timestamp'
