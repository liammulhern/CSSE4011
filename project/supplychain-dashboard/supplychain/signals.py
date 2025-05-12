from django.db.models.signals import post_save
from django.dispatch import receiver

from supplychain.models import (
    ProductOrder,
    ProductOrderStatus
)

@receiver(post_save, sender=ProductOrder)
def create_initial_order_status(sender, instance, created, **kwargs):
    if created:
        # only fire once, when the order is first saved
        ProductOrderStatus.objects.create(
            order=instance,
            status=ProductOrderStatus.STATUS_NEW,
            timestamp=instance.created_timestamp or None,
            created_by=instance.created_by
        )

