import boto3
import json
import base64
from datetime import datetime, timezone

def lambda_handler(event, context):
        ''' Handles the sensors' readings and triggers the actuators '''
        client = boto3.client('iot-data', region_name='us-east-1')
        
        UTC_SKEW = 2
        
        START_HOUR = 8
        END_HOUR = 19
        LUX_THRESHOLD = 25
        
        TEMPHIGH_THRESHOLD = 30
        TEMPLOW_THRESHOLD = 10
        
        WINTER_START_MONTH = -2 # = 2 months before the current year (November)
        WINTER_END_MONTH = 3
        SUMMER_START_MONTH = 5
        SUMMER_END_MONTH = 8
        
        payload = event['payload']
        dec = base64.b64decode(payload)
        json_dec = json.loads(dec)
        
        
        print(dec)
        id = json_dec['id'][-1]
        print(id)
        
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
        lamp_status = int(json_dec['lamp'])
        #print(lamp_status)
        enable_lux = lamp_status
        #if the light is on and we are outside the activity hours
        print("HI")
        print(hour_now, START_HOUR, END_HOUR)
        if (lamp_status == 1 and (int(json_dec['lux']) >= LUX_THRESHOLD or (hour_now <= START_HOUR or hour_now >= END_HOUR))):
                enable_lux = 0
        
        #print(enable_lux)
        
        currentmonth = int(datetime.now().date().month)
        #print(currentmonth)
                
        led_code = 2 # Green
        # This check should only be performed during the summer period
        if ((currentmonth >= SUMMER_START_MONTH and currentmonth <= SUMMER_END_MONTH) and int(json_dec['temp']) < TEMPLOW_THRESHOLD):
                led_code = 1 # Blue
        # This check should only be performed during the winter period
        elif ((currentmonth >= WINTER_START_MONTH and currentmonth <= WINTER_END_MONTH) and int(json_dec['temp']) > TEMPHIGH_THRESHOLD):
                led_code = 0 # Red

#topic='awsiot_to_localgateway',

        reply_payload = json.dumps({"id":str(id), "acts":"2", "lux":str(enable_lux), "led":str(led_code)})
        reply_payload_b = reply_payload.encode("utf-8")
        reply_payload_dec = base64.b64encode(reply_payload_b)
        repl_p = reply_payload_dec.decode("utf-8")

        print(reply_payload, reply_payload_b, reply_payload_dec)

        final_payload = json.dumps({"thingName": str(event['devEUI']), "bytes": repl_p})
        
        print(final_payload)
        
        data = dict()
        data['sample_time'] = int(datetime.today().timestamp()*1000)
        data['device_id'] = json_dec['id']
        data['device_data'] = dict()
        data['device_data']['id'] = json_dec['id']
        data['device_data']['lux'] = json_dec['lux']
        data['device_data']['temp'] = json_dec['temp']
        data['device_data']['lamp'] = json_dec['lamp']
        data['device_data']['led'] = json_dec['led']
        
        print("-----")
        print(data)
        
        
        # ADD INFO TO DB
        table = boto3.resource("dynamodb").Table("powersaver_readings")
        table.put_item(Item=data)
        
        # Change topic, qos and payload
        response = client.publish(
                topic='lorawan/downlink',
                qos=0,
                payload=final_payload
            )

        return response