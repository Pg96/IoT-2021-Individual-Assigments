import boto3
import json

def lambda_handler(event, context):
        client = boto3.client('iot-data', region_name='us-east-1')
        
        enable_lux = 0
        if (int(event['lux']) < 15):
                enable_lux = 1
        led_code = 2 # Green
        if (int(event['temp']) < 10):
                led_code = 1 # Blue
        elif (int(event['temp']) > 30):
                led_code = 0 # Red

        # Change topic, qos and payload
        response = client.publish(
                topic='awsiot_to_localgateway',
                qos=0,
                payload=json.dumps({"acts":"1", "lux":str(enable_lux), "led":str(led_code)})
            )

        return response