from django.contrib import admin

from notifications.models import ProductNotification
@admin.register(ProductNotification)
class NotificationAdmin(admin.ModelAdmin):
    list_display = ('notication_type', 'productevent', 'requirement', 'order', 'created_timestamp')
    search_fields = ('notication_type', 'productevent__name', 'requirement__name', 'order__order_number')
    list_filter = ('notication_type', 'created_timestamp')
    date_hierarchy = 'created_timestamp'
    ordering = ('-created_timestamp',)
