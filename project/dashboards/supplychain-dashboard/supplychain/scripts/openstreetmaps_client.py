import requests
import os

from dotenv import load_dotenv

load_dotenv()

def reverse_geocode(lat: float, lon: float) -> dict:
    """
    Reverse geocode a latitude and longitude using OpenStreetMap's Nominatim service.

    Args:
        lat (float): Latitude of the location.
        lon (float): Longitude of the location.

    Returns:
        dict: A dictionary containing the reverse geocoded address and other details.
        In format:
        {
            "place_id": "123456",
            "osm_type": "node",
            "osm_id": "123456789",
            "lat": "12.345678",
            "lon": "-98.765432",
            "display_name": "Some Place, Some City, Some Country",
            ...
        }
    """
    url = os.getenv('OSM_REVERSE_GEOCODE', "https://nominatim.openstreetmap.org/reverse")
    params = {
        "format": "jsonv2",
        "lat": lat,
        "lon": lon,
        "accept-language": "en",
        "zoom": 10,
        "email": os.getenv('OSM_EMAIL', 'admin@pathledger.live'),
    }

    headers = {
        "User-Agent": f"my-geofencing-app/1.0 ({os.getenv('OSM_EMAIL', 'admin@pathledger.live')})"
    }

    resp = requests.get(url, params=params, headers=headers)
    resp.raise_for_status()

    return resp.json()
