from django.db import models
from django.utils import timezone

from accounts.utils import get_user_roles

from accounts.models import User, Role

from supplychain.models import ProductEvent, ProductOrder, SupplyChainRequirement, Tracker

class ProductNotification(models.Model):
    """
    Represents an alert generated from a product event that indicates a requirement was not met.
    """
    NOTICATION_TYPE_ALERT = 'alert'
    NOTICATION_TYPE_NOTIFICATION = 'notification'
    NOTICATION_TYPE_CHOICES = [
        (NOTICATION_TYPE_ALERT, 'Alert'),
        (NOTICATION_TYPE_NOTIFICATION, 'Notification'),
    ]

    notication_type = models.CharField(
        max_length=20,
        choices=NOTICATION_TYPE_CHOICES,
        default=NOTICATION_TYPE_NOTIFICATION,
        help_text="Type of notification, either alert or notification."
    )

    productevent = models.ForeignKey(
        ProductEvent,
        on_delete=models.CASCADE,
        related_name='notifications',
        help_text="The product event that triggered this alert."
    )

    requirement = models.ForeignKey(
        SupplyChainRequirement,
        on_delete=models.CASCADE,
        related_name='notifications',
        help_text="The supply chain requirement that was not met.",
        blank=True,
        null=True,
    )

    order = models.ForeignKey(
        ProductOrder,
        on_delete=models.CASCADE,
        related_name='notifications',
        help_text="The product order associated with this alert."
    )

    timestamp = models.DateTimeField(
        help_text="Timestamp when the producy event occured."
    )

    created_timestamp = models.DateTimeField(
        auto_now_add=True,
        help_text="Timestamp when the notification was created."
    )

    message = models.TextField(
        help_text="Detailed message about the notification or alert.",
        blank=True,
        null=True
    )

    acknowledged_timestamp = models.DateTimeField(
        blank=True,
        null=True,
        help_text="Timestamp when the notification was acknowledged."
    )

    acknowledged_by = models.ForeignKey(
        User,
        on_delete=models.SET_NULL,
        blank=True,
        null=True,
        related_name='acknowledged_productevent_notifications',
        help_text="User who acknowledged the notification."
    )

    class Meta:
        verbose_name = "Product Notification"
        verbose_name_plural = "Product Notifications"
        ordering = ['-created_timestamp']
        indexes = [
            models.Index(fields=['order']),
        ]

    def __str__(self):
        return f"{self.notication_type} for {self.productevent.product.product_key} on {self.created_timestamp}"

    def acknowledge(self, user: User) -> None:
        """
            Mark this notification as acknowledged by a user.
        """

        # Verify that the user is able to acknowledge this notification
        # if not user.is_authenticated:
        #     raise ValueError("User must be authenticated to acknowledge notifications.")
        #
        # if Role.ADMIN not in get_user_roles(user, self.order.owner):
        #     raise ValueError("User must be an admin to acknowledge notifications.")
        #
        # # check that the user is at the company of the order
        # if self.order.owner is None:
        #     raise ValueError("Order must have an owner to acknowledge notifications.")
        #
        # if user.company != self.order.owner:
        #     raise ValueError("User must belong to the company of the order to acknowledge notifications.")
        #
        # if self.acknowledged_by is not None:
        #     raise ValueError("Notification has already been acknowledged.")

        self.acknowledged_by = user
        self.acknowledged_timestamp = timezone.now()
        self.save(update_fields=['acknowledged_by', 'acknowledged_timestamp'])

class TrackerNotification(models.Model):
    """
    Represents a notification that can be sent to users.
    This is a base class for different types of notifications.
    """
    NOTICATION_TYPE_ALERT = 'alert'
    NOTICATION_TYPE_NOTIFICATION = 'notification'
    NOTICATION_TYPE_CHOICES = [
        (NOTICATION_TYPE_ALERT, 'Alert'),
        (NOTICATION_TYPE_NOTIFICATION, 'Notification'),
    ]

    notication_type = models.CharField(
        max_length=20,
        choices=NOTICATION_TYPE_CHOICES,
        default=NOTICATION_TYPE_NOTIFICATION,
        help_text="Type of notification, either alert or notification."
    )

    tracker = models.ForeignKey(
        Tracker,
        on_delete=models.CASCADE,
        related_name='notifications',
        help_text="The tracker associated with this notification."
    )

    timestamp = models.DateTimeField(
        help_text="Timestamp when the tracker event occured."
    )

    created_timestamp = models.DateTimeField(
        auto_now_add=True,
        help_text="Timestamp when the notification was created."
    )

    message = models.TextField(
        help_text="Detailed message about the notification or alert.",
        blank=True,
        null=True
    )

    acknowledged_timestamp = models.DateTimeField(
        blank=True,
        null=True,
        help_text="Timestamp when the notification was acknowledged."
    )

    acknowledged_by = models.ForeignKey(
        User,
        on_delete=models.SET_NULL,
        blank=True,
        null=True,
        related_name='acknowledged_trackerevent_notifications',
        help_text="User who acknowledged the notification."
    )

    class Meta:
        verbose_name = "Tracker Notification"
        verbose_name_plural = "Tracker Notifications"
        ordering = ['-created_timestamp']
        indexes = [
            models.Index(fields=['tracker'])
        ]

    def acknowledge(self, user: User) -> None:
        """
            Mark this notification as acknowledged by a user.
        """

        # Verify that the user is able to acknowledge this notification
        if not user.is_authenticated:
            raise ValueError("User must be authenticated to acknowledge notifications.")

        if Role.ADMIN not in get_user_roles(user, self.order.owner):
            raise ValueError("User must be an admin to acknowledge notifications.")

        if self.acknowledged_by is not None:
            raise ValueError("Notification has already been acknowledged.")

        self.acknowledged_by = user
        self.acknowledged_timestamp = models.DateTimeField(auto_now=True)
        self.save(update_fields=['acknowledged_by', 'acknowledged_timestamp'])
