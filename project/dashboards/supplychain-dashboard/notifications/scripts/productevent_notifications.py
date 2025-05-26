"""
    Create notifications and alerts based on the product order requirement.
"""

from django.db import transaction

from supplychain.models import ProductEvent, ProductOrder, SupplyChainRequirement
from supplychain.scripts import osm_reverse_geocode

from notifications.models import ProductNotification

from typing import List, Dict, Any

import math


def create_notifications_from_productevent(productevent: ProductEvent) -> List[ProductNotification]:
    """
    Create Notification or Alert objects for each order+requirement
    that applies to this product event and whose timing matches.
    """
    prod = productevent.product
    event_ts = productevent.timestamp

    # 1) Find all orders that include this product and are already placed
    orders = ProductOrder.objects.filter(
        items__product=prod,
        order_timestamp__lte=event_ts
    ).distinct()

    notifications = []
    with transaction.atomic():
        for order in orders:
            # 2) For each requirement attached to this order
            for productorder_requirement in order.order_requirements.select_related('requirement'):
                requirement: SupplyChainRequirement = productorder_requirement.requirement

                if requirement.attribute_type == SupplyChainRequirement.ATTRIBUTE_NUMBER:
                    notification = get_value_based_notifications(productevent, requirement, order)

                    if not notification:
                        continue

                    notifications.append(notification)

                elif requirement.attribute_type == SupplyChainRequirement.ATTRIBUTE_LOCATION:
                    notification = get_location_based_notifactions(productevent, requirement, order)

                    if not notification:
                        continue

                    notifications.append(notification)

    return notifications


def get_value_based_notifications(
        productevent: ProductEvent,
        requirement: SupplyChainRequirement,
        order: ProductOrder
    ) -> ProductNotification | None:
    """
        Check if the product event meets the values-based requirement.

        Args:
            productevent (ProductEvent): The product event to check.
            requirement (SupplyChainRequirement): The requirement to validate against.

        Returns:
            bool: True if the requirement is met, False otherwise.
    """
    details = requirement.details  # e.g. {"min":1.0,"nominal":4.0,"max":8.0}
    val = productevent.payload.get(requirement.unit)

    if val is None:
        return None 

    low = details.get('min')
    high = details.get('max')

    if (low is not None and val < low) or (high is not None and val > high):
        notification = ProductNotification.objects.create(
            notication_type=ProductNotification.NOTICATION_TYPE_ALERT,
            productevent=productevent,
            requirement=requirement,
            order=order,
            message=(
                f"{requirement.name} out of bounds "
                f"({val} not in [{low}, {high}])"
            ),
            timestamp=productevent.timestamp,
        )

        return notification

    notification = ProductNotification.objects.create(
        notication_type=ProductNotification.NOTICATION_TYPE_NOTIFICATION,
        productevent=productevent,
        requirement=requirement,
        order=order,
        message=(
            f"{requirement.name} OK: {val} within "
            f"[{low}, {high}]"
        ),
        timestamp=productevent.timestamp,
    )

    return notification


def get_location_based_notifactions(
        productevent: ProductEvent,
        requirement: SupplyChainRequirement,
        order: ProductOrder
    ) -> ProductNotification | None:
    """
        Check if the product event meets the location-based requirement.

        Args:
            productevent (ProductEvent): The product event to check.
            requirement (SupplyChainRequirement): The requirement to validate against.

        Returns:
            bool: True if the requirement is met, False otherwise.
    """
    details = requirement.details  # e.g. {"type":"exclude", "location":{"country":"AU", "city":"Brisbane", "latitude": -27.470125, "longitude": 153.021072}}

    lat = productevent.payload.get('latitude')
    lon = productevent.payload.get('longitude')

    # Get address
    reverse_geocode = osm_reverse_geocode(lat, lon)

    has_include = any(r["type"] == "include" for r in details)

    matches: List[tuple[int, Dict[str, Any]]] = []

    for detail in details:
        location = detail.get("location", {})
        specificity = 0
        did_match = False

        # 1) position-level?
        pos = location.get("position")
        if pos:
            specificity = 4
            d = _haversine_distance(lat, lon, pos["latitude"], pos["longitude"])
            did_match = (d <= pos["threshold_m"])

        # 2) city-level?
        elif location.get("city"):
            specificity = 3
            # Nominatim returns e.g. reverse_geocode["city"] = "Brisbane"
            did_match = (reverse_geocode.get("city", "").lower() == location["city"].lower())

        # 3) state-level?
        elif location.get("state"):
            specificity = 2
            # Nominatim returns e.g. reverse_geocode["state"] = "Queensland"
            did_match = (reverse_geocode.get("state", "").lower() == location["state"].lower())

        # 4) country-level?
        elif location.get("country"):
            specificity = 1
            # Nominatim returns e.g. reverse_geocode["country_code"] = "AU"
            did_match = (reverse_geocode.get("country_code", "").upper() == location["country"].upper())

        if did_match:
            matches.append((specificity, detail))

    # pick the rule with highest specificity
    if matches:
        best_spec = max(spec for spec, _ in matches)

        _, chosen = next(m for m in matches if m[0] == best_spec)

        if chosen["type"] == "include":
            notification = ProductNotification.objects.create(
                notication_type=ProductNotification.NOTICATION_TYPE_NOTIFICATION,
                productevent=productevent,
                requirement=requirement,
                order=order,
                message=(
                    f"{requirement.name} OK: {reverse_geocode.get('display_name', 'Unknown location')} "
                    f"({chosen['location'].get('city', 'Unknown city')}, "
                ),
                timestamp=productevent.timestamp,
            )

            return notification

        notification = ProductNotification.objects.create(
            notication_type=ProductNotification.NOTICATION_TYPE_ALERT,
            productevent=productevent,
            requirement=requirement,
            order=order,
            message=(
                f"{requirement.name} out of bounds: "
                f"{reverse_geocode.get('display_name', 'Unknown location')} "
                f"({chosen['location'].get('city', 'Unknown city')})"
            ),
            timestamp=productevent.timestamp,
        )

        return notification

    # no rule matched → deny if any include-only rules exist
    if has_include:
        notification = ProductNotification.objects.create(
            notication_type=ProductNotification.NOTICATION_TYPE_ALERT,
            productevent=productevent,
            requirement=requirement,
            order=order,
            message=(
                    f"{requirement.name} out of bounds: "
                    f"{reverse_geocode.get('display_name', 'Unknown location')} "
                    f"({reverse_geocode.get('city', 'Unknown city')}, "
            ),
            timestamp=productevent.timestamp,
        )

        return notification

    return None


def _haversine_distance(
    lat1: float,
    lon1: float,
    lat2: float,
    lon2: float
) -> float:
    """
        Calculate the great-circle distance between two points
        on Earth specified in decimal degrees using the Haversine formula.
        Returns distance in meters.

        Args:
            lat1 (float): Latitude of the first point.
            lon1 (float): Longitude of the first point.
            lat2 (float): Latitude of the second point.
            lon2 (float): Longitude of the second point.

        Returns:
            float: Distance in meters between the two points.
    """
    earth_radius_m = 6_371_000  # meters

    # 1) Convert degrees to radians
    lat1_rad = math.radians(lat1)
    lon1_rad = math.radians(lon1)
    lat2_rad = math.radians(lat2)
    lon2_rad = math.radians(lon2)

    # 2) Compute coordinate differences
    delta_lat_rad = lat2_rad - lat1_rad
    delta_lon_rad = lon2_rad - lon1_rad

    # 3) Apply Haversine formula
    sin_half_delta_lat = math.sin(delta_lat_rad / 2)
    sin_half_delta_lon = math.sin(delta_lon_rad / 2)

    a = (
        sin_half_delta_lat ** 2
        + math.cos(lat1_rad) * math.cos(lat2_rad) * sin_half_delta_lon ** 2
    )
    central_angle = 2 * math.asin(math.sqrt(a))

    # 4) Distance = Earth radius × central angle
    return earth_radius_m * central_angle
