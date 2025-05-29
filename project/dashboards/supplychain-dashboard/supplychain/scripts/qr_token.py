from django.utils import timezone
from django.db.models import Model
from django.conf import settings

from datetime import timedelta

import jwt
import qrcode
import qrcode.image.svg

def create_model_jwt(model: Model) -> str:
    """
        Create a JWT token for a given model instance.

        Args:
            model (Model): The Django model instance to encode.

        Returns:
            str: The JWT token.
    """
    payload = {
        'id': model.id,
        'type': 'qr',
        'model': type(model).__name__,
        'exp': timezone.now() + timedelta(hours=1),
    }

    return jwt.encode(payload, settings.SECRET_KEY, algorithm='HS256')

def create_model_qr_code(url: str, model: Model) -> bytes:
    """
        Create a QR code for a given model instance.

        Args:
            url (str): The base URL to encode in the QR code.
            model (Model): The Django model instance to encode.

        Returns:
            bytes: The SVG representation of the QR code.
    """
    token: str = create_model_jwt(model)

    qr_payload = f"{url}?token={token}"

    factory = qrcode.image.svg.SvgPathImage
    qr_code = qrcode.make(qr_payload, image_factory=factory)

    return qr_code.to_string()
