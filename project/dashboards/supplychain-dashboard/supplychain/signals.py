from django.db.models.signals import post_save
from django.dispatch import receiver

from supplychain.models import ProductOrder, ProductOrderStatus, ProductEvent, TrackerEvent

from supplychain.scripts.productevent_builder import create_productevent_from_trackerevent
from notifications.scripts.productevent_notifications import create_notifications_from_productevent

@receiver(post_save, sender=ProductOrder)
def create_initial_order_status(sender, instance, created, **kwargs):
    if created:
        ProductOrderStatus.objects.create(
            order=instance,
            status=ProductOrderStatus.STATUS_NEW,
            timestamp=instance.created_timestamp or None,
            created_by=instance.created_by
        )

@receiver(post_save, sender=ProductEvent)
def productevent_post_save(sender, instance, created, **kwargs):
    if created:
        create_notifications_from_productevent(instance)

@receiver(post_save, sender=TrackerEvent)
def tracker_event_post_save(sender, instance, created, **kwargs):
    if created:
        create_productevent_from_trackerevent(instance)
