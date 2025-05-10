from rest_framework import viewsets, status
from rest_framework.response import Response

from abac.constants import Action

from supplychain.models import Product

from supplychain.serialisers import ProductSerializer
from supplychain.permissions import ABACPermission

class ProductViewSet(viewsets.ModelViewSet):
    """
    CRUD API for Products with ABAC-enforced permissions.
    """
    queryset = Product.objects.all()
    serializer_class = ProductSerializer
    permission_classes = [ABACPermission]

    def get_queryset(self):
        user = self.request.user
        # Filter objects by ABAC view permission
        return [p for p in super().get_queryset() if user.has_perm(Action.VIEW_PRODUCT.value, p)]

    def perform_create(self, serializer):
        user = self.request.user
        # Global create permission (object=None)

        if not user.has_perm(Action.CREATE_PRODUCT.value, None):
            raise PermissionDenied("You do not have permission to create products.")

        serializer.save()

    def perform_update(self, serializer):
        instance = self.get_object()
        user = self.request.user

        if not user.has_perm(Action.CHANGE_PRODUCT.value, instance):
            raise PermissionDenied("You do not have permission to update this product.")

        serializer.save()

    def destroy(self, request, *args, **kwargs):
        instance = self.get_object()
        user = request.user

        if not user.has_perm(Action.DELETE_PRODUCT.value, instance):
            raise PermissionDenied("You do not have permission to delete this product.")

        return super().destroy(request, *args, **kwargs)
