import azure.functions as func
import datetime
import json
import logging

app = func.FunctionApp()


@app.event_hub_message_trigger(arg_name="azeventhub", event_hub_name="iothub-ehub-pathledger-56059448-519a09d736",
                               connection="IOTHUB_EVENTS_CS") 
def ProcessTelemetryFunction(azeventhub: func.EventHubEvent):
    logging.info('Python EventHub trigger processed an event: %s',
                azeventhub.get_body().decode('utf-8'))
