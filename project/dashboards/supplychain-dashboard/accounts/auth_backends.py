from django.conf import settings

from rest_framework.authentication import BaseAuthentication
from rest_framework import exceptions
from rest_framework.permissions import BasePermission

import jwt

class IsQRForThisModel(BasePermission):
    def has_permission(self, request, view):
        token = request.auth

        return bool(
            token
            and token.get('type') == 'qr'
            and int(token.get('id')) == int(view.kwargs['pk']) # Make sure the token id matches the model id
            and str(token.get('model')) == str(view.kwargs['model']) # Make sure the token model matches the model
        )

class QueryParamJWTAuthentication(BaseAuthentication):
    def authenticate(self, request):
        """
            Authenticate the user using a JWT token passed as a query parameter.
        """

        raw = request.query_params.get('token')

        if not raw:
            return None

        try:
            payload = jwt.decode(raw, settings.SECRET_KEY, algorithms=['HS256'])
        except jwt.ExpiredSignatureError:
            raise exceptions.AuthenticationFailed('QR token expired')
        except jwt.InvalidTokenError:
            raise exceptions.AuthenticationFailed('Invalid QR token')

        return (None, payload)
