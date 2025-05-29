from django.http import HttpResponse, HttpResponseBadRequest

from rest_framework_simplejwt.authentication import JWTAuthentication
from rest_framework import viewsets, permissions, status
from rest_framework.decorators import action
from rest_framework.response import Response

from accounts.permissions import IsAuthenticatedOrValidQR 
from accounts.auth import QRAuthentication

from supplychain.serialisers.serialiser_product import ProductSerializer
from supplychain.serialisers.serialiser_events import ProductEventSerializer
from supplychain.models import Product, ProductOrderItem, ProductEvent
from supplychain.scripts.qr_token import create_model_qr_code

class ProductViewSet(viewsets.ModelViewSet):
    """
        list / retrieve / create / update / delete Products.
    """
    queryset = Product.objects.all()
    serializer_class = ProductSerializer
    permission_classes = [IsAuthenticatedOrValidQR]
    authentication_classes = [JWTAuthentication, QRAuthentication]

    def perform_create(self, serializer):
        serializer.save(recorded_by=self.request.user)

    def get_queryset(self):
        qs = super().get_queryset()
        user = self.request.user

        return qs.filter(
            owner_id__in=user.user_companies.filter(
                is_active=True
            ).values_list('company_id', flat=True)
        )

    def get_serializer_class(self):
        if self.action == "events":
            return ProductEventSerializer

        return ProductSerializer

    @action(detail=True, methods=['get'], url_path='qr-code')
    def qr_code(self, request, pk=None):
        """
        GET /products/{pk}/qr-code/?url=https://app.myfrontendsite.com/view
        Returns an SVG QR payload embedding a short-lived JWT for this product.
        If `url` isn’t provided, falls back to reversing the DRF detail URL.
        """
        product = self.get_object()

        # 1. Grab the `url` query-param
        base_url = request.query_params.get('url')
        if not base_url:
            # fallback to DRF detail route
            try:
                base_url = request.build_absolute_uri(
                    reverse('product-detail', args=[product.pk])
                )
            except Exception:
                return HttpResponseBadRequest("No `url` provided and could not reverse detail route.")

        # 2. Generate the QR SVG
        try:
            svg_bytes = create_model_qr_code(base_url, product)
        except Exception as e:
            return Response(
                {"detail": f"Could not generate QR code: {str(e)}"},
                status=status.HTTP_500_INTERNAL_SERVER_ERROR
            )

        # 3. Return raw SVG
        return HttpResponse(svg_bytes, content_type='image/svg+xml')    

    @action(detail=False, methods=['get'], url_path=r'order/(?P<order_pk>[^/.]+)')
    def by_order(self, request, order_pk=None):
        """
        GET /products/by-order/{order_pk}/
        list all Products that are line‐items on the given ProductOrder.
        """
        # 1. Find all ProductOrderItem entries for this order
        items = ProductOrderItem.objects.filter(order_id=order_pk)
        product_ids = items.values_list('product_id', flat=True)

        # 2. Filter the base queryset
        qs = self.get_queryset().filter(id__in=product_ids)

        # 3. Paginate if requested
        page = self.paginate_queryset(qs)
        if page is not None:
            serializer = self.get_serializer(page, many=True)
            return self.get_paginated_response(serializer.data)

        # 4. Or return full list
        serializer = self.get_serializer(qs, many=True)
        return Response(serializer.data, status=status.HTTP_200_OK)

    @action(detail=True, methods=["get"], url_path="events")
    def events(self, request, pk=None):
        """
        GET /products/{pk}/events/
        List all ProductTrackerEvent instances for this product,
        in reverse‐chronological order.
        """
        product = self.get_object()
        qs = ProductEvent.objects.filter(product=product).order_by("-timestamp")

        page = self.paginate_queryset(qs)
        if page is not None:
            serializer = self.get_serializer(page, many=True)
            return self.get_paginated_response(serializer.data)

        serializer = self.get_serializer(qs, many=True)
        return Response(serializer.data, status=status.HTTP_200_OK)
