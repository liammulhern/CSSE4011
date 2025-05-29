from rest_framework.decorators import api_view, permission_classes
from rest_framework.response import Response

from accounts.serialisers import UserSerializer

@api_view(['GET'])
@permission_classes([])
def current_user(request):
    serializer = UserSerializer(request.user)
    return Response(serializer.data)
