from django.db.models.signals import post_save
from django.dispatch import receiver

from telemetry.models import GatewayEventRaw
from telemetry.scripts.ingest.tracker_ingest import tracker_raw_data_ingest_from_gateway

@receiver(post_save, sender=GatewayEventRaw)
def gatewayeventraw_post_save(sender, instance, created, **kwargs):
    """
        After we create a new GatewayEventRaw, we want to trigger the 
        creation of tracker events based on the raw data.
    """
    if created:
        tracker_raw_data_ingest_from_gateway(instance.gateway)

