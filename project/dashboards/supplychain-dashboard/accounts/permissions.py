from django.conf import settings
from django.contrib.auth.models import AnonymousUser

from rest_framework.authentication import BaseAuthentication
from rest_framework import exceptions
from rest_framework.permissions import BasePermission

import jwt

class IsAuthenticatedOrValidQR(BasePermission):
    """
    Allow access if the user is JWT-authenticated,
    OR if they present a valid QR token matching this model+pk.
    """
    message = "Must be authenticated or provide a valid QR token for this product."

    def has_permission(self, request, view):
        # 1) JWT path
        if request.user and request.user.is_authenticated:
            return True

        # 2) QR-token path
        payload = getattr(request, "qr_payload", None)
        if not payload or payload.get("type") != "qr":
            return False

        model_name = view.get_queryset().model.__name__
        if payload.get("model") != model_name:
            return False

        try:
            token_id = int(payload.get("id"))
            url_id   = int(view.kwargs.get(view.lookup_field))
        except (TypeError, ValueError):
            return False

        return token_id == url_id

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
