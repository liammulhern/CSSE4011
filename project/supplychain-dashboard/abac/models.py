"""
abac.models

Defines ABAC core: subject/resource attributes, policies, and policy bindings.
"""

from django.conf import settings
from django.contrib.contenttypes.fields import GenericForeignKey
from django.contrib.contenttypes.models import ContentType
from django.db import models


class SubjectAttribute(models.Model):
    """
    Arbitrary attribute assigned to a User for ABAC decisions.
    e.g. ('region', 'APAC'), ('role_level', '3')
    """
    user = models.ForeignKey(
        settings.AUTH_USER_MODEL,
        on_delete=models.CASCADE,
        related_name="abac_subject_attributes",
    )
    name = models.CharField(
        max_length=100,
        help_text="Attribute name, e.g. 'region', 'clearance_level'."
    )
    value = models.CharField(
        max_length=200,
        help_text="Attribute value as string."
    )

    class Meta:
        unique_together = ("user", "name", "value")
        verbose_name = "Subject Attribute"
        verbose_name_plural = "Subject Attributes"

    def __str__(self):
        return f"{self.user.username}: {self.name}={self.value}"


class ResourceAttribute(models.Model):
    """
    Arbitrary attribute on any resource (model instance) for ABAC.
    Uses a generic foreign key so you can tag events, products, etc.
    """
    content_type = models.ForeignKey(
        ContentType,
        on_delete=models.CASCADE,
        related_name="abac_resource_attributes"
    )
    object_id = models.PositiveIntegerField()
    content_object = GenericForeignKey("content_type", "object_id")
    name = models.CharField(
        max_length=100,
        help_text="Attribute name, e.g. 'sensitivity', 'owner_company'."
    )
    value = models.CharField(
        max_length=200,
        help_text="Attribute value as string."
    )

    class Meta:
        indexes = [
            models.Index(fields=["content_type", "object_id"]),
        ]
        verbose_name = "Resource Attribute"
        verbose_name_plural = "Resource Attributes"

    def __str__(self):
        return (
            f"{self.content_type.app_label}."
            f"{self.content_type.model}[{self.object_id}]: "
            f"{self.name}={self.value}"
        )


class Policy(models.Model):
    """
    A single ABAC policy: if all subject-attrs AND
    all resource-attrs hold, then for any matching action,
    effect is applied (allow or deny).
    """
    EFFECT_CHOICES = [
        ("allow", "Allow"),
        ("deny", "Deny"),
    ]

    name = models.CharField(
        max_length=150,
        unique=True,
        help_text="Human-readable policy name."
    )
    description = models.TextField(
        blank=True,
        help_text="What this policy enforces."
    )
    effect = models.CharField(
        max_length=5,
        choices=EFFECT_CHOICES,
        default="deny",
        help_text="Whether to allow or deny when policy matches."
    )

    class Meta:
        ordering = ["name"]
        verbose_name = "ABAC Policy"
        verbose_name_plural = "ABAC Policies"

    def __str__(self):
        return f"{self.name} ({self.effect})"


class PolicySubjectAttribute(models.Model):
    """
    One required subject-attribute for a policy to match.
    e.g. policy requires ('company_type', 'manufacturer')
    """
    policy = models.ForeignKey(
        Policy,
        on_delete=models.CASCADE,
        related_name="subject_attribute_requirements"
    )
    name = models.CharField(max_length=100)
    value = models.CharField(max_length=200)

    class Meta:
        verbose_name = "Policy Subject Attribute"
        verbose_name_plural = "Policy Subject Attributes"

    def __str__(self):
        return f"{self.policy.name}: subj.{self.name}={self.value}"


class PolicyResourceAttribute(models.Model):
    """
    One required resource-attribute for a policy to match.
    e.g. policy applies only to resources with ('owner_company', 'AcmeCo')
    """
    policy = models.ForeignKey(
        Policy,
        on_delete=models.CASCADE,
        related_name="resource_attribute_requirements"
    )
    name = models.CharField(max_length=100)
    value = models.CharField(max_length=200)

    class Meta:
        verbose_name = "Policy Resource Attribute"
        verbose_name_plural = "Policy Resource Attributes"

    def __str__(self):
        return f"{self.policy.name}: res.{self.name}={self.value}"


class PolicyAction(models.Model):
    """
    An action (string) that this policy covers, e.g. 'view_event',
    'edit_product', 'record_transfer'.
    """
    policy = models.ForeignKey(
        Policy,
        on_delete=models.CASCADE,
        related_name="actions"
    )
    name = models.CharField(
        max_length=100,
        help_text="Action name to be checked in code."
    )

    class Meta:
        verbose_name = "Policy Action"
        verbose_name_plural = "Policy Actions"
        unique_together = ("policy", "name")

    def __str__(self):
        return f"{self.policy.name}: action={self.name}"
