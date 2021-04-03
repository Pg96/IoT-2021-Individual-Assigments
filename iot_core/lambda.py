import boto3
import json
from datetime import datetime, timezone

def lambda_handler(event, context):
        ''' Handles the sensors' readings and triggers the actuators '''
        client = boto3.client('iot-data', region_name='us-east-1')
        
        UTC_SKEW = 2
        
        START_HOUR = 8
        END_HOUR = 19
        LUX_THRESHOLD = 15
        
        TEMPHIGH_THRESHOLD = 30
        TEMPLOW_THRESHOLD = 10
        
        WINTER_START_MONTH = -2 # = 2 months before the current year (November)
        WINTER_END_MONTH = 3
        SUMMER_START_MONTH = 5
        SUMMER_END_MONTH = 8
        
        hour_now = int(datetime.now(timezone.utc).time().hour)
        #print(hour_now)
        hour_now += UTC_SKEW
        #print(hour_now)
        if (hour_now >= 24):
                hour_now = hour_now - 24
        print(hour_now)
        
        #enable_lux = 0
        #if (int(event['lux']) <= LUX_THRESHOLD and (hour_now >= START_HOUR and hour_now <= END_HOUR)):
        #        enable_lux = 1
        #print(event)
        lamp_status = int(event['lamp'])
        #print(lamp_status)
        enable_lux = lamp_status
        #if the light is on and we are outside the activity hours
        if (lamp_status == 1 and (int(event['lux']) >= LUX_THRESHOLD or (hour_now <= START_HOUR or hour_now >= END_HOUR))):
                enable_lux = 0
        
        #print(enable_lux)
        
        currentmonth = int(datetime.now().date().month)
        #print(currentmonth)
                
        led_code = 2 # Green
        # This check should only be performed during the summer period
        if ((currentmonth >= SUMMER_START_MONTH and currentmonth <= SUMMER_END_MONTH) and int(event['temp']) < TEMPLOW_THRESHOLD):
                led_code = 1 # Blue
        # This check should only be performed during the winter period
        elif ((currentmonth >= WINTER_START_MONTH and currentmonth <= WINTER_END_MONTH) and int(event['temp']) > TEMPHIGH_THRESHOLD):
                led_code = 0 # Red

        # Change topic, qos and payload
        response = client.publish(
                topic='awsiot_to_localgateway',
                qos=0,
                payload=json.dumps({"acts":"2", "lux":str(enable_lux), "led":str(led_code)})
            )

        return response