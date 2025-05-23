from rest_framework import viewsets, permissions
from rest_framework.decorators import action

from supplychain.serialisers.serialiser_product import ProductSerializer
from supplychain.models import Product
from supplychain.scripts.qr_token import create_model_qr_code

class ProductViewSet(viewsets.ModelViewSet):
    """
        list / retrieve / create / update / delete Products.
    """
    queryset = Product.objects.all()
    serializer_class = ProductSerializer
    permission_classes = [
        permissions.IsAuthenticated,
    ]

    def perform_create(self, serializer):
        serializer.save(recorded_by=self.request.user)

    @action(detail=True, methods=['get'], url_path='qr-code')
    def qr_code(self, request, pk=None):
        """
        GET /products/{pk}/qr-code/
        → returns an SVG QR payload embedding a short‐lived JWT for this product
        """
        product = self.get_object()

        # build a URL to your front‐end or API detail view
        # here we point back at the DRF detail endpoint
        detail_url = request.build_absolute_uri(
            reverse('product-detail', args=[product.pk])
        )

        # generate the SVG bytes
        svg_bytes = create_model_qr_code(detail_url, product)

        # return raw SVG
        return HttpResponse(svg_bytes, content_type='image/svg+xml')
