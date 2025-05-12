"""
Management command to seed ABAC models based on the definitions in abac/constants.py.
"""

from django.core.management.base import BaseCommand

from abac.constants import POLICY_DEFINITIONS

from abac.models import (
    Policy,
    PolicySubjectAttribute,
    PolicyResourceAttribute,
    PolicyAction,
)

class Command(BaseCommand):
    """Seeds ABAC policies, subject/resource attributes, and actions."""

    help = "Seed ABAC tables from abac/constants.POLICY_DEFINITIONS"

    def handle(self, *args, **options):
        for policy_name, definition in POLICY_DEFINITIONS.items():
            # Create or update the Policy
            policy, created = Policy.objects.update_or_create(
                name=policy_name.value,
                defaults={
                    'description': definition.get('description', ''),
                    'effect': definition.get('effect').value,
                }
            )

            # Clear existing related entries
            policy.subject_attribute_requirements.all().delete()
            policy.resource_attribute_requirements.all().delete()
            policy.actions.all().delete()

            # Seed subject attributes
            for attr_enum, value in definition.get('subject_attributes', []):
                PolicySubjectAttribute.objects.create(
                    policy=policy,
                    name=attr_enum.value,
                    value=value,
                )

            # Seed resource attributes
            for attr_enum, value in definition.get('resource_attributes', []):
                PolicyResourceAttribute.objects.create(
                    policy=policy,
                    name=attr_enum.value,
                    value=value,
                )

            # Seed actions
            for action_enum in definition.get('actions', []):
                PolicyAction.objects.create(
                    policy=policy,
                    name=action_enum.value,
                )

            status = "Created" if created else "Updated"
            self.stdout.write(f"{status} policy: {policy_name.value}")

        self.stdout.write(self.style.SUCCESS("All ABAC policies seeded successfully."))
