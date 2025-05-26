from django.db import transaction
from iota_sdk import HexStr
from notifications.models import ProductNotification
from supplychain.models import TrackerEvent, ProductEvent, ProductOrderTracker, ProductOrderStatus, Product, ProductOrderRequirement, ProductOrder

from typing import List, Optional

from datetime import datetime

import logging

logger = logging.getLogger(__name__)


def create_productevent_from_trackerevent(trackerevent: TrackerEvent) -> List[ProductEvent]:
    """
        Creates a ProductEvent from a TrackerEvent based on a ProductOrders assigned trackers.

        Args:
            tracker_event (TrackerEvent): The TrackerEvent instance to convert.

        Returns:
            ProductEvent: The created ProductEvent instance.
    """
    productevents: List[ProductEvent] = []
    trackerevent_timestamp = trackerevent.timestamp

    # 1) All assignments whose assigned_timestamp is <= event time
    productordertracker_timestamp_qs = ProductOrderTracker.objects.filter(
        tracker=trackerevent.tracker,
        assigned_timestamp__lte=trackerevent_timestamp
    ).select_related('order')

    with transaction.atomic():
        for productordertracker in productordertracker_timestamp_qs:
            order = productordertracker.order

            # 2) Find the delivered timestamp for this order, if any
            delivered_record = order.status_history.filter(
                status=ProductOrderStatus.STATUS_DELIVERED
            ).order_by(
                '-timestamp'
            ).first()

            delivered_timestamp: Optional[datetime] = delivered_record.timestamp if delivered_record else None

            # 3) Skip if the tracker-event happened *after* delivery
            if delivered_timestamp and trackerevent_timestamp > delivered_timestamp:
                continue

            # 4) Otherwise, create a ProductEvent for each product in the order
            for item in order.items.select_related('product'):
                productevent = ProductEvent.objects.create(
                    message_id=trackerevent.message_id,
                    product=item.product,
                    trackerevent=trackerevent,
                    event_type=ProductEvent.EVENT_TYPE_TELEMETRY,
                    payload=trackerevent.payload,
                    timestamp=trackerevent.timestamp,
                    recorded_by=None,
                )

                # Verify the product hash
                if HexStr(trackerevent.data_hash) != trackerevent.compute_hash():
                    ProductNotification.objects.create(
                        productevent=productevent,
                        message=f"Payload hash mismatch for product event.",
                        timestamp=trackerevent.timestamp,
                        notication_type=ProductNotification.NOTICATION_TYPE_ALERT,
                    )

                    logger.error(
                        f"Data hash mismatch for ProductEvent {productevent.pk}. "
                        f"Expected {HexStr(trackerevent.data_hash)}, got {trackerevent.compute_hash()}"
                    )

                productevents.append(productevent)

    return productevents
