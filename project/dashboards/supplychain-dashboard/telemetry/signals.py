from django.db.models.signals import post_save
from django.dispatch import receiver

from telemetry.models import GatewayEventRaw
from telemetry.scripts.ingest.tracker_ingest import tracker_raw_data_ingest_from_gatewayevent

import logging

logger = logging.getLogger(__name__)

@receiver(post_save, sender=GatewayEventRaw)
def gatewayeventraw_post_save(sender, instance, created, **kwargs):
    """
        After we create a new GatewayEventRaw, we want to trigger the 
        creation of tracker events based on the raw data.
    """
    if created:
        try:
            tracker_raw_data_ingest_from_gatewayevent(instance)
        except Exception as e:
            logger.error(f"Creating tracker raw event cause {e}")

