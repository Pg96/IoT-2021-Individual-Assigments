import boto3
from datetime import datetime, timedelta
import json

def lambda_handler(event, context):
    ''' Retrieves last hour's readings from the DB and compute max, min, average '''
    TS_LEN = 10
    PAST_LEN = 1 # Used to retrieve the readings from the last our 
    
    #print(datetime.today())
    #print(datetime.today().timestamp())
    last_hour = datetime.today() - timedelta(hours=PAST_LEN)
    #print(last_hour)
    #print(int(last_hour.timestamp()))
    #print(int(last_hour.timestamp())*1000)

    table = boto3.resource("dynamodb").Table("powersaver_readings")
 
    items = table.scan(
        FilterExpression="sample_time >= :date",
        ExpressionAttributeValues={":date": int(last_hour.timestamp())*1000},
        ProjectionExpression="sample_time,device_data"
    )['Items']

    #print(items)
    if (len(items) == 0):
        print("empty")
        return { 'statusCode':200 }
    
    # The last element is the latest reading
    i = sorted(items, key=lambda x: x['sample_time'], reverse=True)
    latest = i[0]
    
    #print(i)


    max_lux = int(max(i, key=lambda x: x['device_data']['lux'])['device_data']['lux'])
    min_lux = int(min(i, key=lambda x: x['device_data']['lux'])['device_data']['lux'])
    
    max_temp = int(max(i, key=lambda x: x['device_data']['lux'])['device_data']['temp'])
    min_temp = int(min(i, key=lambda x: x['device_data']['lux'])['device_data']['temp'])
    
    all_lux = []
    all_temp = []
    
    avg_lux = 0
    avg_temp = 0
    
    for v in i:
        lux = int(v['device_data']['lux'])
        temp = int(v['device_data']['temp'])
        avg_lux += lux
        avg_temp += temp
        all_lux.append(lux)
        all_temp.append(temp)
    avg_lux /= len(i)
    avg_temp /= len(i)
    

    return { 
    'statusCode': 200,
    'latest_lux':int(latest['device_data']['lux']), 
    'latest_temp':int(latest['device_data']['temp']),
    'lux_max': max_lux,
    'lux_min': min_lux,
    'lux_avg': avg_lux,
    'temp_max': max_temp,
    'temp_min': min_temp,
    'temp_avg': avg_temp,
    
    'all_lux': all_lux,
    'all_temp': all_temp
    }
