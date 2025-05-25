from django.conf import settings

from django.contrib.auth.models import AnonymousUser
from rest_framework.authentication import BaseAuthentication
from rest_framework import exceptions

import jwt

class QRAuthentication(BaseAuthentication):
    """
    Look for `?token=` in the querystring, decode it, and stash it on request.qr_payload.
    """
    def authenticate(self, request):
        token = request.query_params.get("token")
        if not token:
            return None

        try:
            payload = jwt.decode(token, settings.SECRET_KEY, algorithms=["HS256"])
        except jwt.ExpiredSignatureError:
            raise exceptions.AuthenticationFailed("QR token expired")
        except jwt.InvalidTokenError:
            raise exceptions.AuthenticationFailed("Invalid QR token")

        if payload.get("type") != "qr":
            raise exceptions.AuthenticationFailed("Invalid QR token type")

        request.qr_payload = payload
        return (AnonymousUser(), None)
