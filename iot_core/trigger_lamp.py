import boto3
import json

def lambda_handler(event, context):
        ''' Toggles the lamp '''
        client = boto3.client('iot-data', region_name='us-east-1')
        
        code = event['lamp']
        
        # Change topic, qos and payload
        response = client.publish(
                topic='awsiot_to_localgateway',
                qos=0,
                payload=json.dumps({"acts":"1", "lux":str(code)})
            )

        return {
            'statusCode': 200,
            'body': json.dumps('Command executed!')
        }
