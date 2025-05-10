from django.db import models
from django.conf import settings

import uuid


class Product(models.Model):
    """
    A product tracked by the Digital Product Passport.
    """
    product_key = models.CharField(
        max_length=100,
        unique=True,
        help_text="Unique identifier (e.g. NFC/QR code) for the product."
    )
    batch = models.CharField(
        max_length=100,
        blank=True,
        help_text="Optional batch number or lot code."
    )
    owner = models.ForeignKey(
        'accounts.Company',
        null=True,
        blank=True,
        on_delete=models.SET_NULL,
        related_name='products',
        help_text="Current owning company."
    )
    created_at = models.DateTimeField(
        auto_now_add=True,
        help_text="When this product record was created."
    )

    class Meta:
        ordering = ['created_at']
        verbose_name = "Product"
        verbose_name_plural = "Products"

    def __str__(self):
        return self.product_key


class ProductEvent(models.Model):
    """
    Records an event in a product's lifecycle, anchored on-chain/off-chain.
    """
    id = models.UUIDField(
        primary_key=True,
        default=uuid.uuid4,
        editable=False,
        help_text="Unique event identifier."
    )
    product = models.ForeignKey(
        'supplychain.Product',
        on_delete=models.CASCADE,
        related_name='events',
        help_text="Product to which this event applies."
    )
    event_type = models.CharField(
        max_length=50,
        help_text="Type of event (e.g. 'manufactured', 'temperature_reading')."
    )
    timestamp = models.DateTimeField(
        help_text="When the event occurred."
    )
    data_hash = models.CharField(
        max_length=64,
        help_text="SHA-256 hash of the raw payload."
    )
    data_uri = models.TextField(
        blank=True,
        help_text="URI to off-chain storage (e.g. CosmosDB link or IPFS CID)."
    )
    recorded_by = models.ForeignKey(
        settings.AUTH_USER_MODEL,
        null=True,
        blank=True,
        on_delete=models.SET_NULL,
        related_name='recorded_events',
        help_text="User who recorded this event."
    )
    created_at = models.DateTimeField(
        auto_now_add=True,
        help_text="When this record was created in the dashboard database."
    )

    class Meta:
        ordering = ['timestamp']
        indexes = [
            models.Index(fields=['product', 'timestamp']),
            models.Index(fields=['data_hash']),
        ]
        verbose_name = "Product Event"
        verbose_name_plural = "Product Events"

    def __str__(self):
        return f"{self.event_type} @ {self.timestamp.isoformat()} for {self.product}"


class CustodyTransfer(models.Model):
    """
    Represents a change in product custody from one company to another.
    """
    product = models.ForeignKey(
        'supplychain.Product',
        on_delete=models.CASCADE,
        related_name='custody_transfers',
        help_text="Product being transferred."
    )
    from_company = models.ForeignKey(
        'accounts.Company',
        null=True,
        blank=True,
        on_delete=models.SET_NULL,
        related_name='transfers_out',
        help_text="Company transferring out custody."
    )
    to_company = models.ForeignKey(
        'accounts.Company',
        null=True,
        blank=True,
        on_delete=models.SET_NULL,
        related_name='transfers_in',
        help_text="Company receiving custody."
    )
    timestamp = models.DateTimeField(
        help_text="When the transfer occurred."
    )
    recorded_by = models.ForeignKey(
        settings.AUTH_USER_MODEL,
        null=True,
        blank=True,
        on_delete=models.SET_NULL,
        related_name='recorded_transfers',
        help_text="User who recorded the transfer."
    )
    created_at = models.DateTimeField(
        auto_now_add=True,
        help_text="When this record was created."
    )

    class Meta:
        ordering = ['timestamp']
        indexes = [
            models.Index(fields=['product', 'timestamp']),
        ]
        verbose_name = "Custody Transfer"
        verbose_name_plural = "Custody Transfers"

    def __str__(self):
        return (
            f"{self.product} from {self.from_company} → "
            f"{self.to_company} @ {self.timestamp.isoformat()}"
        )
