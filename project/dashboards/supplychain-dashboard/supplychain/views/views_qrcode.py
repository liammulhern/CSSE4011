from rest_framework.decorators import api_view, permission_classes
from rest_framework.response import Response
from rest_framework import permissions

from accounts.serialisers import UserSerializer

@api_view(['GET'])
@permission_classes([permissions.IsAuthenticated])
def get_model_qr_code(request):

    return Response(serializer.data)
