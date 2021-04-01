import boto3
import json
from datetime import datetime, timezone

def lambda_handler(event, context):
        client = boto3.client('iot-data', region_name='us-east-1')
        
        UTC_SKEW = 2                    # Rome is UTC+2
        
        START_HOUR = 8                  # After this hour, lights can be turned on
        END_HOUR = 19                   # After this hour, lights should be turned off
        LUX_THRESHOLD = 15
        
        TEMPHIGH_THRESHOLD = 30         # Temperature too high
        TEMPLOW_THRESHOLD = 10          # Temperature too low
        
        
        hour_now = int(datetime.now(timezone.utc).time().hour)
        hour_now += UTC_SKEW
        if (hour_now >= 24):
                hour_now = hour_now - 24
        
        enable_lux = 0
        if (int(event['lux']) < LUX_THRESHOLD and (hour_now >= START_HOUR and hour_now <= END_HOUR)):
                enable_lux = 1
                
        led_code = 2 # Green (== OK)
        if (int(event['temp']) < TEMPLOW_THRESHOLD):
                led_code = 1 # Blue (== Too low)
        elif (int(event['temp']) > TEMPHIGH_THRESHOLD):
                led_code = 0 # Red (== Too high)

        # Change topic, qos and payload
        response = client.publish(
                topic='awsiot_to_localgateway',
                qos=0,
                payload=json.dumps({"acts":"2", "lux":str(enable_lux), "led":str(led_code)})
            )

        return response