from rest_framework import viewsets, permissions
from rest_framework.decorators import action
from rest_framework.response import Response

from supplychain.models import Tracker, TrackerEvent
from supplychain.serialisers.serialiser_tracker import TrackerSerializer
from supplychain.serialisers.serialiser_events import TrackerEventSerializer, EventFilterSerializer

class TrackerViewSet(viewsets.ModelViewSet):
    """
        list / retrieve / create / update / delete Trackers.
    """
    queryset = Tracker.objects.select_related('owner').all()
    serializer_class = TrackerSerializer
    permission_classes = [permissions.IsAuthenticated]

    @action(detail=True, methods=['get'], url_path='events')
    def events(self, request, pk=None):
        """
        GET /trackers/{pk}/events/?start=2025-05-01T00:00:00Z&end=2025-05-10T23:59:59Z
        → all TrackerEvents for this tracker between start and end.
        """
        # 1. validate and parse query params
        filt = EventFilterSerializer(data=request.query_params)
        filt.is_valid(raise_exception=True)
        start, end = filt.validated_data['start'], filt.validated_data['end']

        # 2. filter events (assumes TrackerEvent has a FK 'tracker')
        qs = TrackerEvent.objects.filter(
            tracker_id=pk,
            timestamp__gte=start,
            timestamp__lte=end,
        ).order_by('timestamp')

        # 3. serialize and return
        data = TrackerEventSerializer(qs, many=True).data
        return Response(data)

