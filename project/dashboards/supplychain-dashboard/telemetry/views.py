from rest_framework.compat import requests
from rest_framework.permissions import AllowAny
from rest_framework.views import APIView
from rest_framework.response import Response
from rest_framework_api_key.permissions import HasAPIKey

from telemetry.scripts.ingest import gateway_ingest

import logging

logger = logging.getLogger(__name__)

class TelemetryView(APIView):
    permission_classes = [HasAPIKey]

    def post(self, request):
        data = request.data

        if not isinstance(data, dict):
            return Response({"error": "Invalid data format"}, status=400)

        required_fields = ['header', 'payload', 'signature']

        for field in required_fields:
            if field not in data:
                return Response({"error": f"Missing required field: {field}"}, status=400)

        try:
            gateway_ingest.gateway_raw_data_ingest(data)
        except ValueError as e:
            logger.info(f"{e}")
            return Response({"error": str(e)}, status=400)

        return Response({"status": "ok"})
