import boto3
import json

def lambda_handler(event, context):
        ''' Toggles the RGB led '''
        client = boto3.client('iot-data', region_name='us-east-1')
        
        dev = event['id'][-1]
        code = event['led']

        # Change topic, qos and payload
        response = client.publish(
                topic='awsiot_to_localgateway',
                qos=0,
                payload=json.dumps({"id":dev, "acts":"1", "led":str(code)})
            )

        return {
            'statusCode': 200,
            'body': json.dumps('Command executed!')
        }