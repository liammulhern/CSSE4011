from django.contrib import admin

from telemetry.models import GatewayEventRaw
@admin.register(GatewayEventRaw)
class GatewayEventRaw(admin.ModelAdmin):
    list_display = ('message_id', 'gateway_id', 'message_type')
    list_filter = ('gateway_id', 'message_type')
    search_fields = ('gateway_id',)


