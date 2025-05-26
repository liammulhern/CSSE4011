from __future__ import annotations

from iota_sdk import HexStr

from django.db import models
from django.conf import settings
from django.utils import timezone

from accounts.models import Company

from supplychain.scripts import iota_client

import json
import uuid
import hashlib

class Tracker(models.Model):
    """
    Tracker device that collects sensor data for products 
    """
    tracker_key = models.CharField(
        max_length=100,
        unique=True,
        help_text="Unique identifier (e.g. NFC/QR code) for the tracker."
    )

    owner = models.ForeignKey(
        Company,
        null=True,
        blank=True,
        on_delete=models.SET_NULL,
        related_name='trackers',
        help_text="Current owning company."
    )

    created_timestamp = models.DateTimeField(
        auto_now_add=True,
        help_text="When this tracker record was created."
    )


    class Meta:
        ordering = ['created_timestamp']
        verbose_name = "Tracker"
        verbose_name_plural = "Trackers"
        indexes = [
            models.Index(fields=['tracker_key']),
        ]


class Gateway(models.Model):
    """
    Tracker device that collects sensor data for products 
    """
    gateway_key = models.CharField(
        max_length=100,
        unique=True,
        help_text="Unique identifier (e.g. NFC/QR code) for the gateway."
    )

    owner = models.ForeignKey(
        Company,
        null=True,
        blank=True,
        on_delete=models.SET_NULL,
        related_name='gateways',
        help_text="Current owning company."
    )

    created_timestamp = models.DateTimeField(
        auto_now_add=True,
        help_text="When this tracker record was created."
    )


    class Meta:
        ordering = ['created_timestamp']
        verbose_name = "Gatway"
        verbose_name_plural = "Gatways"
        indexes = [
            models.Index(fields=['gateway_key']),
        ]


class ProductType(models.Model):
    """
    A static product definition or SKU.
    All manufactured products reference a ProductType.
    """
    product_number = models.CharField(
        max_length=100,
        unique=True,
        help_text="SKU or part number for this product type."
    )

    name = models.CharField(
        max_length=255,
        help_text="Human‐readable name."
    )

    description = models.TextField(
        blank=True,
        help_text="Optional longer description."
    )

    created_timestamp = models.DateTimeField(
        auto_now_add=True,
        help_text="When this product type was first defined."
    )

    owner = models.ForeignKey(
        Company,
        null=True,
        blank=True,
        on_delete=models.SET_NULL,
        related_name='product_types_owner',
        help_text="Current owning company."
    )

    recorded_by = models.ForeignKey(
        settings.AUTH_USER_MODEL,
        null=True,
        blank=True,
        on_delete=models.SET_NULL,
        related_name='recorded_producttypes',
        help_text="User who recorded this event."
    )

    class Meta:
        ordering = ["product_number"]
        verbose_name = "Product Type"
        verbose_name_plural = "Product Types"

    def __str__(self):
        return f"{self.product_number} — {self.name}"


class Product(models.Model):
    """
    A product tracked by the Digital Product Passport.
    """
    product_key = models.CharField(
        max_length=100,
        unique=True,
        help_text="Unique identifier (e.g. NFC/QR code) for the product."
    )

    product_type = models.ForeignKey(
        ProductType,
        on_delete=models.PROTECT,
        related_name="products",
        help_text="The SKU / type this serial belongs to.",
        null=True,
        blank=True
    )

    batch = models.CharField(
        max_length=100,
        blank=True,
        help_text="Optional batch number or lot code."
    )

    owner = models.ForeignKey(
        Company,
        null=True,
        blank=True,
        on_delete=models.SET_NULL,
        related_name='products',
        help_text="Current owning company."
    )

    created_timestamp = models.DateTimeField(
        auto_now_add=True,
        help_text="When this product record was created."
    )

    components = models.ManyToManyField(
        'self',
        through='ProductComposition',
        symmetrical=False,
        related_name='used_in',
        help_text=(
            "Other products (and quantities) used to build this one. "
            "Use the through‐model ProductComposition to assign quantities."
        )
    )

    recorded_by = models.ForeignKey(
        settings.AUTH_USER_MODEL,
        null=True,
        blank=True,
        on_delete=models.SET_NULL,
        related_name='recorded_products',
        help_text="User who recorded this event."
    )

    class Meta:
        ordering = ['created_timestamp']
        verbose_name = "Product"
        verbose_name_plural = "Products"
        indexes = [
            models.Index(fields=['product_type']),
            models.Index(fields=['batch']),
        ]

    def __str__(self):
        return self.product_key


class TrackerEvent(models.Model):
    """
    Records an event for a tracker anchored on-chain/off-chain.
    """
    EVENT_TYPE_TELEMETRY = "telemetry"

    EVENT_TYPE_CHOICES = [
        (EVENT_TYPE_TELEMETRY, "Telemetry"),
    ]

    message_id = models.UUIDField(
        primary_key=True,
        default=uuid.uuid4,
        editable=False,
        help_text="Unique event identifier."
    )

    gateway = models.ForeignKey(
        Gateway,
        on_delete=models.SET_NULL,
        related_name='trackerevent_gateways',
        help_text="Gateway that collected this event.",
        null=True,
        blank=True,
    )

    tracker = models.ForeignKey(
        Tracker,
        on_delete=models.SET_NULL,
        related_name='trackeevent_trackers',
        help_text="Tracker to which this event applies.",
        null=True,
        blank=True,
    )

    event_type = models.CharField(
        max_length=50,
        choices=EVENT_TYPE_CHOICES,
        help_text="Type of event (e.g. 'manufactured', 'temperature_reading')."
    )

    payload = models.JSONField(
        help_text=(
            'JSON parameters for this payload. '
            'E.g. {"deviceId": 1.0,"nominal": 4.0,"max": 8.0} '
        )
    )

    timestamp = models.DateTimeField(
        help_text="When the event occurred."
    )

    data_hash = models.CharField(
        max_length=64,
        help_text="SHA-256 hash of the raw payload."
    )

    block_id = models.CharField(
        max_length=64,
        help_text="IOTA block id of the onchain hash."
    )

    created_timestamp = models.DateTimeField(
        auto_now_add=True,
        help_text="When this record was created in the dashboard database."
    )

    class Meta:
        ordering = ['timestamp']
        indexes = [
            models.Index(fields=['event_type', 'timestamp']),
        ]
        verbose_name = "Tracker Event"
        verbose_name_plural = "Tracker Events"

    def __str__(self):
        return f"{self.event_type} @ {self.timestamp.isoformat()} for {self.product}"

    def compute_hash(self) -> HexStr:
        """
            SHA-256 over payload and message with sorted keys.

            Returns:
                HexString for hashed ProductEvent

                e.g. "0x12398a12bc14e09"
        """
        serialized_key: str = json.dumps(
            {
                "message_id": self.message_id, 
                "payload": self.payload
            },
            sort_keys=True
        )

        serialized_key_enc = hashlib.sha256(serialized_key.encode("utf-8")).hexdigest()

        return HexStr(serialized_key_enc)

    def verify_block_hash(self) -> bool:
        """
            Verify that an IOTA block has the same hash as the 
            database model.

            Args:
                block: IOTA blockchain 

            Return:
                True if the on-chain hash matches the off-chain model
        """
        chain_message_id, chain_hash = iota_client.iota_get_block_data(self.message_id)

        model_hash: HexStr  = self.compute_hash()

        return chain_message_id == self.message_id and model_hash == chain_hash


class ProductEvent(models.Model):
    """
    Records an event in a product's lifecycle, anchored on-chain/off-chain.
    """
    EVENT_TYPE_MANUFACTURED = "manufactured"
    EVENT_TYPE_TEMPERATURE_READING = "temperature_reading"
    EVENT_TYPE_POSITION_READING = "position_reading"
    EVENT_TYPE_GAS_READING = "gas_reading"
    EVENT_TYPE_ACCELERATION_READING = "acceleration_reading"
    EVENT_TYPE_HUMIDITY_READING = "humidity_reading"

    EVENT_TYPE_CHOICES = [
        (EVENT_TYPE_MANUFACTURED, "Manufactured"),
        (EVENT_TYPE_TEMPERATURE_READING, "Temperature Reading"),
        (EVENT_TYPE_POSITION_READING, "Position Reading"),
        (EVENT_TYPE_GAS_READING, "Gas Reading"),
        (EVENT_TYPE_ACCELERATION_READING, "Acceleration Reading"),
        (EVENT_TYPE_HUMIDITY_READING, "Humidity Reading"),
    ]

    message_id = models.UUIDField(
        primary_key=True,
        default=uuid.uuid4,
        editable=False,
        help_text="Unique event identifier."
    )

    product = models.ForeignKey(
        Product,
        on_delete=models.CASCADE,
        related_name='products',
        help_text="Product to which this event applies."
    )

    trackerevent = models.ForeignKey(
        TrackerEvent,
        on_delete=models.SET_NULL,
        related_name='trackerevents',
        help_text="Tracker event to which this was created from.",
        null=True,
        blank=True,
    )

    event_type = models.CharField(
        choices=EVENT_TYPE_CHOICES,
        max_length=50,
        help_text="Type of event (e.g. 'manufactured', 'temperature_reading')."
    )

    payload = models.JSONField(
        help_text=(
            'JSON parameters for this payload. '
            'E.g. {"deviceId": 1.0,"nominal": 4.0,"max": 8.0} '
        )
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
        related_name='recorded_productevents',
        help_text="User who recorded this event."
    )

    created_timestamp = models.DateTimeField(
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

    def compute_hash(self) -> HexStr:
        """
            SHA-256 over payload and message with sorted keys.

            Returns:
                HexString for hashed ProductEvent

                e.g. "0x12398a12bc14e09"
        """
        serialized_key: str = json.dumps(
            {
                "message_id": self.message_id, 
                "payload": self.payload
            },
            sort_keys=True
        )

        serialized_key_enc = hashlib.sha256(serialized_key.encode("utf-8")).hexdigest()

        return HexStr(serialized_key_enc)

    def verify_block_hash(self) -> bool:
        """
            Verify that an IOTA block has the same hash as the 
            database model.

            Args:
                block: IOTA blockchain 

            Return:
                True if the on-chain hash matches the off-chain model
        """
        chain_message_id, chain_hash = iota_client.iota_get_block_data(self.message_id)

        model_hash: HexStr  = self.compute_hash()

        return chain_message_id == self.message_id and model_hash == chain_hash


class ProductComposition(models.Model):
    """
    Through‐model linking a parent product to its component products.
    Records how many units of each component go into one unit of the parent.
    """
    parent = models.ForeignKey(
        Product,
        on_delete=models.CASCADE,
        related_name='composition_lines',
        help_text="The product being built."
    )

    component = models.ForeignKey(
        Product,
        on_delete=models.CASCADE,
        related_name='component_of',
        help_text="A product used as an ingredient/part."
    )

    created_timestamp = models.DateTimeField(
        auto_now_add=True,
        help_text="When this composition entry was created."
    )

    class Meta:
        unique_together = ('parent', 'component')
        verbose_name = "Product Composition"
        verbose_name_plural = "Product Compositions"
        ordering = ['parent', 'component']

    def __str__(self):
        return f"{self.component.product_key} → {self.parent.product_key}"


class CustodyTransfer(models.Model):
    """
    Represents a change in product custody from one company to another.
    """
    product = models.ForeignKey(
        Product,
        on_delete=models.CASCADE,
        related_name='custody_transfers',
        help_text="Product being transferred."
    )

    from_company = models.ForeignKey(
        Company,
        null=True,
        blank=True,
        on_delete=models.SET_NULL,
        related_name='transfers_out',
        help_text="Company transferring out custody."
    )

    to_company = models.ForeignKey(
        Company,
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

    created_timestamp = models.DateTimeField(
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


class ComplianceEvent(models.Model):
    """
    Records a compliance event for a product, such as passing a quality check.
    """
    product = models.ForeignKey(
        Product,
        on_delete=models.CASCADE,
        related_name='compliance_events',
        help_text="Product to which this event applies."
    )

    event_type = models.CharField(
        max_length=50,
        help_text="Type of compliance event (e.g. 'quality_check_passed')."
    )

    payload = models.JSONField(
        help_text=(
            'JSON parameters for this payload. '
            'E.g. {"deviceId": 1.0,"nominal": 4.0,"max": 8.0} '
        )
    )

    timestamp = models.DateTimeField(
        help_text="When the event occurred."
    )

    created_timestamp = models.DateTimeField(
        auto_now_add=True,
        help_text="When this record was created in the dashboard database."
    )

    class Meta:
        ordering = ['timestamp']
        indexes = [
            models.Index(fields=['product', 'timestamp']),
        ]
        verbose_name = "Compliance Event"
        verbose_name_plural = "Compliance Events"


class SupplyChainRequirement(models.Model):
    """
    Defines a requirement (e.g., temperature, humidity) that products must meet.
    """
    ATTRIBUTE_NUMBER = 'number'
    ATTRIBUTE_LOCATION = 'location'

    ATTRIBUTE_TYPE_CHOICES = [
        (ATTRIBUTE_NUMBER, 'Number'),
        (ATTRIBUTE_LOCATION, 'Location'),
    ]

    UNIT_DEGREES_C    = 'degrees_C'
    UNIT_REL_HUMIDITY = 'relative_%'
    UNIT_TVOC         = 'TVOC'
    UNIT_LUX          = 'Lux'
    UNIT_HPA          = 'hPa'
    UNIT_MS2          = 'm/s²'
    UNIT_TIME         = 'seconds'
    UNIT_DISTANCE     = 'meters'
    UNIT_LOCATION     = 'location'

    UNIT_CHOICES = [
        (UNIT_DEGREES_C,    'Degrees C'),
        (UNIT_REL_HUMIDITY, 'Relative %'),
        (UNIT_TVOC,         'TVOC'),
        (UNIT_LUX,          'Lux'),
        (UNIT_HPA,          'hPa'),
        (UNIT_MS2,          'm/s²'),
        (UNIT_LOCATION,     'Location'),
        (UNIT_TIME,         'Seconds'),
        (UNIT_DISTANCE,     'Meters'),
    ]

    name = models.CharField(
        max_length=100,
        unique=True,
        help_text='Name of the requirement (e.g. "Temperature Control").'
    )

    unit = models.CharField(
        max_length=20,
        choices=UNIT_CHOICES,
        help_text='Unit of measurement for this requirement.'
    )

    attribute_type = models.CharField(
        max_length=20,
        choices=ATTRIBUTE_TYPE_CHOICES,
        help_text='Type of data this requirement expects.'
    )

    owner = models.ForeignKey(
        Company,
        on_delete=models.CASCADE,
        related_name='supply_requirements',
        help_text='Company that defines this requirement.'
    )

    details = models.JSONField(
        help_text=(
            'JSON parameters for this requirement. '
            'E.g. {"min":1.0,"nominal":4.0,"max":8.0} '
            'or {"whitelist":["USA","CAN"],"blacklist":["RUS"]}'
        )
    )

    created_timestamp = models.DateTimeField(auto_now_add=True)

    created_by = models.ForeignKey(
        settings.AUTH_USER_MODEL,
        null=True,
        blank=True,
        on_delete=models.SET_NULL,
        related_name='created_requirements',
        help_text="User who created this requirement."
    )

    class Meta:
        verbose_name = 'Supply Chain Requirement'
        verbose_name_plural = 'Supply Chain Requirements'
        ordering = ['name']

    def __str__(self):
        return f'{self.name} ({self.get_unit_display()})'


class ProductOrder(models.Model):
    """
    An order of one or more products, placed by one company and received by another.
    """
    order_number = models.CharField(
        max_length=100,
        help_text='Product order number.'
    )

    supplier = models.ForeignKey(
        Company,
        on_delete=models.CASCADE,
        related_name='orders_supplied',
        help_text='Company supplying the products.'
    )

    receiver = models.ForeignKey(
        Company,
        on_delete=models.CASCADE,
        related_name='orders_received',
        help_text='Company receiving the products.'
    )

    order_timestamp = models.DateTimeField(
        help_text='Timestamp when the order was placed.'
    )

    delivery_location = models.CharField(
        max_length=255,
        help_text='Destination address or location identifier.'
    )

    requirements = models.ManyToManyField(
        SupplyChainRequirement,
        through='ProductOrderRequirement',
        blank=True,
        related_name='orders',
        help_text='Supply chain requirements that apply to this order.'
    )

    trackers = models.ManyToManyField(
        Tracker,
        through='ProductOrderTracker',
        blank=True,
        related_name='trackers',
        help_text='Supply chain tracker/s that apply to this order.'
    )

    products = models.ManyToManyField(
        Product,
        through='ProductOrderItem',
        blank=True,
        related_name='product_orders',
        help_text='Products included in this order.'
    )

    created_timestamp = models.DateTimeField(auto_now_add=True)

    created_by = models.ForeignKey(
        settings.AUTH_USER_MODEL,
        null=True,
        blank=True,
        on_delete=models.SET_NULL,
        related_name='created_product_orders',
        help_text="User who created this product order."
    )

    class Meta:
        verbose_name = 'Product Order'
        verbose_name_plural = 'Product Orders'
        ordering = ['-order_timestamp']

    def __str__(self) -> str:
        return (
            f'Order #{self.pk}: {self.supplier} → {self.receiver} '
            f'at {self.order_timestamp.isoformat()}'
        )

    def current_status(self) -> ProductOrderStatus | None:
        """
        Return the latest status, or None if never set.
        """
        latest = self.status_history.order_by('-timestamp').first() # type: ignore reverse foreign key name

        return latest.status if latest else None

    def status_at(self, when) -> ProductOrderStatus | None:
        """
        Return the status of this order at the given datetime `when`.
        If no status <= when, returns None.
        """
        entry = (
            self.status_history # type: ignore reverse foreign key name
                .filter(timestamp__lte=when)
                .order_by('-timestamp')
                .first()
        )

        return entry.status if entry else None


class ProductOrderItem(models.Model):
    """
    A line-item in a product order, specifying quantity for a given product.
    """
    order = models.ForeignKey(
        ProductOrder,
        on_delete=models.CASCADE,
        related_name='items',
        help_text='The order this item belongs to.'
    )

    product = models.ForeignKey(
        Product,
        on_delete=models.CASCADE,
        related_name='order_items',
        help_text='The specific product being ordered.'
    )

    class Meta:
        verbose_name = 'Product Order Item'
        verbose_name_plural = 'Product Order Items'
        unique_together = ('order', 'product')
        ordering = ['order']

    def __str__(self):
        return f'{self.product.product_key}'


class ProductOrderRequirement(models.Model):
    """
    Through-table linking a ProductOrder to a SupplyChainRequirement.
    You can add extra fields here later (e.g. overrides, custom thresholds).
    """
    order = models.ForeignKey(
        ProductOrder,
        on_delete=models.CASCADE,
        related_name='order_requirements',
        help_text='The order to which this requirement applies.'
    )

    requirement = models.ForeignKey(
        SupplyChainRequirement,
        on_delete=models.CASCADE,
        related_name='order_requirements',
        help_text='The requirement that must be met for this order.'
    )

    assigned_timestamp = models.DateTimeField(
        auto_now_add=True,
        help_text='When this requirement was attached to the order.'
    )

    class Meta:
        verbose_name = 'Product Order Requirement'
        verbose_name_plural = 'Product Order Requirements'
        unique_together = ('order', 'requirement')
        ordering = ['assigned_timestamp']

    def __str__(self):
        return (
            f'Requirement "{self.requirement.name}" '
            f'for Order #{self.order.pk}'
        )


class ProductOrderTracker(models.Model):
    """
    Assign a tracker to a product order
    """
    order = models.ForeignKey(
        ProductOrder,
        on_delete=models.CASCADE,
        related_name='order_trackers',
        help_text='The order to which this requirement applies.'
    )

    tracker = models.ForeignKey(
        Tracker,
        on_delete=models.CASCADE,
        related_name='order_requirements',
        help_text='The order to which this requirement applies.'
    )

    assigned_timestamp = models.DateTimeField(
        blank=True,
        null=True,
        help_text='When this tracker was physically attached to the order.'
    )

    created_timestamp = models.DateTimeField(
        auto_now_add=True,
        help_text='When this tracker was assigned to the order.'
    )

    created_by = models.ForeignKey(
        settings.AUTH_USER_MODEL,
        null=True,
        blank=True,
        on_delete=models.SET_NULL,
        related_name='created_product_order_trackers',
        help_text="User who created this product order tracker assignement."
    )


class ProductOrderStatus(models.Model):
    """
    Records each status change for a ProductOrder, with timestamp.
    """
    STATUS_NEW = 'new'
    STATUS_ON_HOLD = 'on_hold'
    STATUS_DELAYED = 'delayed'
    STATUS_SHIPPED = 'shipped'
    STATUS_DELIVERED = 'delivered'

    STATUS_CHOICES = [
        (STATUS_NEW,      'New'),
        (STATUS_ON_HOLD,  'On Hold'),
        (STATUS_DELAYED,  'Delayed'),
        (STATUS_SHIPPED,  'Shipped'),
        (STATUS_DELIVERED,'Delivered'),
    ]

    order = models.ForeignKey(
        ProductOrder,
        on_delete=models.CASCADE,
        related_name='status_history',
        help_text='Order whose status is changing.'
    )

    status = models.CharField(
        max_length=20,
        choices=STATUS_CHOICES,
        help_text='New status for this order.'
    )

    timestamp = models.DateTimeField(
        default=timezone.now,
        help_text='When this status took effect.'
    )

    created_by = models.ForeignKey(
        settings.AUTH_USER_MODEL,
        null=True,
        blank=True,
        on_delete=models.SET_NULL,
        related_name='created_product_order_status',
        help_text="User who created this product order status assignment."
    )

    class Meta:
        ordering = ['timestamp']
        verbose_name = 'Product Order Status'
        verbose_name_plural = 'Product Order Status History'
        get_latest_by = 'timestamp'

    def __str__(self):
        return f'{self.get_status_display()} @ {self.timestamp.isoformat()}' # type: ignore get_status_display
