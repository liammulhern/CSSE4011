"""
Defines Company and a custom User model, linking each user to exactly one company.
"""

from django.conf import settings
from django.contrib.auth.models import AbstractUser
from django.db import models


class Company(models.Model):
    """
    Represents a stakeholder organization in the supply chain.
    """
    TYPE_CHOICES = [
        ('supplier', 'Supplier'),
        ('manufacturer', 'Manufacturer'),
        ('distributor', 'Distributor'),
        ('retailer', 'Retailer'),
        ('consumer', 'Consumer'),
    ]

    name = models.CharField(
        max_length=255,
        unique=True,
        help_text="Official company name."
    )
    type = models.CharField(
        max_length=20,
        choices=TYPE_CHOICES,
        help_text="Role of this company in the supply chain."
    )
    created_at = models.DateTimeField(
        auto_now_add=True,
        help_text="Timestamp when the company was registered."
    )

    class Meta:
        ordering = ['name']
        verbose_name = "Company"
        verbose_name_plural = "Companies"

    def __str__(self):
        return f"{self.name} ({self.get_type_display()})"


class User(AbstractUser):
    """
    Custom user with a link to Company.
    """
    company = models.ForeignKey(
        Company,
        on_delete=models.CASCADE,
        related_name='users',
        null=True,
        blank=True,
        help_text="Company this user belongs to."
    )

    class Meta:
        verbose_name = "User"
        verbose_name_plural = "Users"

    def __str__(self):
        return self.username


class Role(models.Model):
    """
    Represents a high-level permission set (e.g., admin, viewer).
    """
    name = models.CharField(
        max_length=50,
        unique=True,
        help_text="Name of the role, e.g. 'admin', 'viewer'."
    )

    class Meta:
        ordering = ['name']
        verbose_name = "Role"
        verbose_name_plural = "Roles"

    def __str__(self):
        return self.name


class UserRole(models.Model):
    """
    Links Users to Roles.
    """
    user = models.ForeignKey(
        settings.AUTH_USER_MODEL,
        on_delete=models.CASCADE,
        related_name='user_roles',
        help_text="User assigned this role."
    )
    role = models.ForeignKey(
        Role,
        on_delete=models.CASCADE,
        related_name='role_users',
        help_text="Role assigned to the user."
    )
    assigned_at = models.DateTimeField(
        auto_now_add=True,
        help_text="When this role was granted."
    )

    class Meta:
        unique_together = ('user', 'role')
        verbose_name = "User Role"
        verbose_name_plural = "User Roles"

    def __str__(self):
        return f"{self.user} → {self.role}"


