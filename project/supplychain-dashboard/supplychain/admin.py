from django.contrib import admin


from supplychain.models import Product
@admin.register(Product)
class ProductAdmin(admin.ModelAdmin):
    list_display = ('product_key', 'batch', 'owner', 'created_at')
    list_filter = ('owner',)
    search_fields = ('product_key', 'batch')


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
