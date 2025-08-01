"""
Django management command to generate fake ProductNotification and TrackerNotification data.

Usage:
    python manage.py generate_fake_notifications --product 100 --tracker 100

Ensure you have the Faker library installed:
    pip install Faker
"""
from django.core.management.base import BaseCommand
from django.utils import timezone
from faker import Faker
import random

from notifications.models import (
    ProductNotification,
    TrackerNotification
)

from supplychain.models import (
    ProductEvent,
    ProductOrder,
    SupplyChainRequirement,
    Tracker
)
from accounts.models import User

class Command(BaseCommand):
    help = 'Generate fake ProductNotification and TrackerNotification entries'

    def add_arguments(self, parser):
        parser.add_argument(
            '--product',
            type=int,
            default=100,
            help='Number of ProductNotification objects to create'
        )
        parser.add_argument(
            '--tracker',
            type=int,
            default=100,
            help='Number of TrackerNotification objects to create'
        )

    def handle(self, *args, **options):
        fake = Faker()
        num_product = options['product']
        num_tracker = options['tracker']

        # Fetch related objects
        events = list(ProductEvent.objects.all())
        orders = list(ProductOrder.objects.all())
        reqs = list(SupplyChainRequirement.objects.all())
        trackers = list(Tracker.objects.all())
        users = list(User.objects.filter(is_active=True))

        if not (events and orders and trackers):
            self.stdout.write(self.style.ERROR(
                'Ensure you have ProductEvent, ProductOrder, and Tracker data before generating notifications.'
            ))
            return

        # Create ProductNotifications
        for _ in range(num_product):
            pe = random.choice(events)
            order = random.choice(orders)
            req = random.choice(reqs) if reqs and random.random() < 0.7 else None
            ntype = random.choice([
                ProductNotification.NOTICATION_TYPE_ALERT,
                ProductNotification.NOTICATION_TYPE_NOTIFICATION
            ])
            ts = fake.date_time_between(start_date='-30d', end_date='now', tzinfo=timezone.get_current_timezone())
            msg = fake.sentence(nb_words=12)

            pn = ProductNotification.objects.create(
                notication_type=ntype,
                productevent=pe,
                requirement=req,
                order=order,
                timestamp=ts,
                message=msg
            )

            # Optionally acknowledge some
            if users and random.random() < 0.3:
                ack_user = random.choice(users)
                ack_ts = fake.date_time_between(start_date=ts, end_date='now', tzinfo=timezone.get_current_timezone())
                pn.acknowledged_by = ack_user
                pn.acknowledged_timestamp = ack_ts
                pn.save(update_fields=['acknowledged_by', 'acknowledged_timestamp'])

        self.stdout.write(self.style.SUCCESS(f'Created {num_product} ProductNotifications'))

        # Create TrackerNotifications
        for _ in range(num_tracker):
            tr = random.choice(trackers)
            ntype = random.choice([
                TrackerNotification.NOTICATION_TYPE_ALERT,
                TrackerNotification.NOTICATION_TYPE_NOTIFICATION
            ])
            ts = fake.date_time_between(start_date='-30d', end_date='now', tzinfo=timezone.get_current_timezone())
            msg = fake.sentence(nb_words=12)

            tn = TrackerNotification.objects.create(
                notication_type=ntype,
                tracker=tr,
                timestamp=ts,
                message=msg
            )

            # Optionally acknowledge some
            if users and random.random() < 0.3:
                ack_user = random.choice(users)
                ack_ts = fake.date_time_between(start_date=ts, end_date='now', tzinfo=timezone.get_current_timezone())
                tn.acknowledged_by = ack_user
                tn.acknowledged_timestamp = ack_ts
                tn.save(update_fields=['acknowledged_by', 'acknowledged_timestamp'])

        self.stdout.write(self.style.SUCCESS(f'Created {num_tracker} TrackerNotifications'))

