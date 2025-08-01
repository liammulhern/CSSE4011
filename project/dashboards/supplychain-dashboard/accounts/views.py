from django.utils import timezone

from rest_framework.decorators import api_view, permission_classes
from rest_framework.response import Response

from accounts.serialisers import UserSerializer

from accounts.models import User, UserTemporary, Company, UserCompany, Role, UserRole

from faker import Faker

import uuid
import secrets

from datetime import timedelta

@api_view(['GET'])
@permission_classes([])
def current_user(request):
    serializer = UserSerializer(request.user)
    return Response(serializer.data)


@api_view(['POST'])
@permission_classes([])
def create_temporary_user(request):
    fake = Faker()

    username = fake.name()

    password = secrets.token_urlsafe(16)
    expires_at = timezone.now() + timedelta(hours=8)

    user = User.objects.create_user(username=username, password=password)
    UserTemporary.objects.create(user=user, expires_at=expires_at)

    company, _ = Company.objects.get_or_create(
        name="Kiosk Co.",
        type="distributor",
    )

    company_viewer, _ = Role.objects.get_or_create(
        name="viewer",
        company=company
    )

    UserCompany.objects.create(
        user=user,
        company=company
    )

    UserRole.objects.create(
        user=user,
        role=company_viewer
    )

    return Response({"username": username, "password": password, "expires_at": expires_at})

