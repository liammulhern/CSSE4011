from rest_framework.views import APIView
from rest_framework.response import Response
from rest_framework_api_key.permissions import HasAPIKey

from telemetry.scripts.ingest import gateway_ingest

class TelemetryView(APIView):
    permission_classes = [HasAPIKey]

    def post(self, request):
        data = request.data

        # Validate the incoming data
        if not isinstance(data, dict):
            return Response({"error": "Invalid data format"}, status=400)

        required_fields = ['message_id', 'gateway_id', 'message_type', 'payload']

        for field in required_fields:
            if field not in data:
                return Response({"error": f"Missing required field: {field}"}, status=400)

        gateway_ingest

        return Response({"status": "ok"})
