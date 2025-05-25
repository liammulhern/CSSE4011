from rest_framework.decorators import api_view
from rest_framework import viewsets, permissions, status
from rest_framework.decorators import action, permission_classes
from rest_framework.response import Response

from supplychain.models import TrackerEvent, ProductEvent
from supplychain.serialisers.serialiser_events import (
    TrackerEventSerializer, 
    ProductEventSerializer,
    VerifyHashInputSerializer,
    VerifyHashResultSerializer
)

class TrackerEventViewSet(viewsets.ModelViewSet):
    queryset = TrackerEvent.objects.all()
    serializer_class = TrackerEventSerializer

    @action(detail=True, methods=['post'], url_path='verify')
    def verify(self, request, pk=None):
        """
        POST /tracker-events/{pk}/verify/
        Calls TrackerEvent.verify_block_hash() and returns pass/fail.
        """
        tracker = self.get_object()

        try:
            ok = tracker.verify_block_hash()
            return Response({'verified': ok})
        except Exception as exc:
            return Response(
                {'verified': False, 'error': str(exc)},
                status=status.HTTP_500_INTERNAL_SERVER_ERROR
            )

class ProductEventViewSet(viewsets.ModelViewSet):
    queryset = ProductEvent.objects.select_related('product', 'trackerevent', 'recorded_by')
    serializer_class = ProductEventSerializer

    def perform_create(self, serializer):
        serializer.save(recorded_by=self.request.user)

    @action(detail=True, methods=['post'], url_path='verify')
    def verify(self, request, pk=None):
        """
        POST /product-events/{pk}/verify/
        Calls ProductEvent.verify_block_hash() and returns pass/fail.
        """
        product_event = self.get_object()
        try:
            ok = product_event.verify_block_hash()
            return Response({'verified': ok})
        except Exception as exc:
            return Response(
                {'verified': False, 'error': str(exc)},
                status=status.HTTP_500_INTERNAL_SERVER_ERROR
            )

    @action(detail=False, methods=['get'], url_path=r'product/(?P<product_id>[^/.]+)')
    def by_product(self, request, product_id=None):
        """
        GET /api/product-events/by-product/{product_id}/
        list all ProductEvent instances for the given product.
        """
        # Filter events by product foreign key
        qs = self.get_queryset().filter(product=product_id).order_by('-timestamp')

        # Apply pagination if requested
        page = self.paginate_queryset(qs)
        if page is not None:
            serializer = self.get_serializer(page, many=True)
            return self.get_paginated_response(serializer.data)

        serializer = self.get_serializer(qs, many=True)
        return Response(serializer.data, status=status.HTTP_200_OK)

@api_view(['POST'])
def verify_event_hashes(request):
    """
    POST /events/verify-block-hashes/
    {
      "message_ids": ["uuid1", "uuid2", ...]
    }
    returns a list of {message_id, event_type, verified, [error]}.
    """
    inp = VerifyHashInputSerializer(data=request.data)
    inp.is_valid(raise_exception=True)

    results = []

    for mid in inp.validated_data['message_ids']:
        # try as TrackerEvent, then as ProductEvent
        for model, etype in ((TrackerEvent, 'tracker'), (ProductEvent, 'product')):
            try:
                ev = model.objects.get(message_id=mid)
            except model.DoesNotExist:
                continue

            try:
                ok = ev.verify_block_hash()
                results.append({'message_id': mid, 'event_type': etype, 'verified': ok})
            except Exception as e:
                results.append({
                    'message_id': mid,
                    'event_type': etype,
                    'verified': False,
                    'error': str(e)
                })
            break
        else:
            # neither model found
            results.append({
                'message_id': mid,
                'event_type': None,
                'verified': False,
                'error': 'Event not found'
            })

    # validate output structure
    out = VerifyHashResultSerializer(results, many=True)

    return Response(out.data)
