from supplychain.models import Gateway
from django.db import models

class GatewayEventRaw(models.Model):
    """
        Records a raw event from a gateway to be converted into ProductEvent a anchored on-chain/off-chain.
    """
    MESSAGE_TYPE_TELEMETRY = "telemetry"
    MESSAGE_TYPE_BATCH_TELEMETRY = "batch_telemetry"
    MESSAGE_TYPE_EVENT = "event"
    MESSAGE_TYPE_HEARTBEAT = "heartbeat"


    MESSAGE_TYPE_CHOICES = [
        (MESSAGE_TYPE_TELEMETRY, "Telemetry"),
        (MESSAGE_TYPE_BATCH_TELEMETRY, "Batch Telemetry"),
        (MESSAGE_TYPE_EVENT, "Event"),
        (MESSAGE_TYPE_HEARTBEAT, "Heart Beat"),
    ]

    message_id = models.UUIDField(
        help_text="Unique event identifier."
    )

    gateway_id = models.CharField(
        max_length=20,
        help_text="Unique gateway identifier"
    )

    gateway = models.ForeignKey(
        Gateway,
        on_delete=models.SET_NULL,
        null=True,
        blank=True,
        help_text="Foreign key to the Gateway model."
    )

    message_type = models.CharField(
        max_length=20,
        choices=MESSAGE_TYPE_CHOICES,
        help_text='Unit of measurement for this requirement.'
    )

    payload = models.JSONField(
        help_text=(
            'JSON parameters for this payload. '
            'E.g. {"deviceId":1.0,"nominal":4.0,"max":8.0} '
        )
    )

    timestamp = models.DateTimeField(
        help_text="When the event occurred."
    )


    created_timestamp = models.DateTimeField(
        auto_now_add=True,
        help_text="When this record was created in the dashboard database."
    )

    signature = models.JSONField(
        help_text="Signature of the payload, used for verification."
    )

    class Meta:
        ordering = ['timestamp']
        indexes = [
            models.Index(fields=['gateway_id', 'timestamp']),
        ]
        verbose_name = "Gateway Event"
        verbose_name_plural = "Gateway Events"

    def __str__(self):
        return f"{self.message_type} @ {self.timestamp.isoformat()} for {self.gateway_id}"



