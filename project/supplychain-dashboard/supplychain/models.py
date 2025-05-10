from django.db import models
from django.conf import settings

from accounts.models import Company

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

    class Meta:
        ordering = ['created_timestamp']
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
        Product,
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

    quantity = models.PositiveIntegerField(
        default=1,
        help_text="How many units of the component are used in one unit of the parent."
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
        return f"{self.quantity} × {self.component.product_key} → {self.parent.product_key}"


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

    company = models.ForeignKey(
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

    def __str__(self):
        return (
            f'Order #{self.pk}: {self.supplier} → {self.receiver} '
            f'at {self.order_time.isoformat()}'
        )


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

    quantity = models.PositiveIntegerField(
        help_text='Number of units ordered.'
    )

    class Meta:
        verbose_name = 'Product Order Item'
        verbose_name_plural = 'Product Order Items'
        unique_together = ('order', 'product')
        ordering = ['order']

    def __str__(self):
        return f'{self.quantity} × {self.product.product_key}'


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
