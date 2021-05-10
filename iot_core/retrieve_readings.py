import boto3
from datetime import datetime, timedelta
import json

def lambda_handler(event, context):
    ''' Retrieves last hour's readings from the DB and compute max, min, average '''
    TS_LEN = 10
    PAST_LEN = 15 # Used to retrieve the readings from the last our 
    
    #print(datetime.today())
    #print(datetime.today().timestamp())
    last_hour = datetime.today() - timedelta(hours=PAST_LEN)
    #print(last_hour)
    #print(int(last_hour.timestamp()))
    #print(int(last_hour.timestamp())*1000)

    table = boto3.resource("dynamodb").Table("powersaver_readings")
 
    all_items = table.scan(ProjectionExpression="sample_time,device_data")['Items']
    
    ll = sorted(all_items, key=lambda x: x['sample_time'], reverse=True)
    latest = ll[0]
    #print(latest)
 
    items = table.scan(
        FilterExpression="sample_time >= :date",
        ExpressionAttributeValues={":date": int(last_hour.timestamp())*1000},
        ProjectionExpression="sample_time,device_data"
    )['Items']

    print(items)
    

    if (len(items) == 0):
        print("empty")
        return { 'statusCode':200,
        'latest_lux':int(latest['device_data']['lux']), 
        'latest_temp':int(latest['device_data']['temp'])       
        }
    
    # The last element is the latest reading
    i = sorted(items, key=lambda x: x['sample_time'], reverse=True)
    #latest = i[0]
    
    #print(i)


    max_lux = int(max(i, key=lambda x: x['device_data']['lux'])['device_data']['lux'])
    min_lux = int(min(i, key=lambda x: x['device_data']['lux'])['device_data']['lux'])
    
    max_temp = int(max(i, key=lambda x: x['device_data']['temp'])['device_data']['temp'])
    min_temp = int(min(i, key=lambda x: x['device_data']['temp'])['device_data']['temp'])
    
    all_lux = []
    all_temp = []
    
    avg_lux = 0
    avg_temp = 0
    
    #4
    for v in i:
        lux = int(v['device_data']['lux'])
        temp = int(v['device_data']['temp'])
        avg_lux += lux
        avg_temp += temp
        #all_lux.append(lux)
        #all_temp.append(temp)
    avg_lux /= len(i)
    avg_temp /= len(i)
    
    devs = set(s['device_data']['id'] for s in items)
    devs = sorted(list(devs))
    print(devs)
    dev_to_data = dict()
    for s in devs:
        dev_to_data[s] = dict()
        lxs = [int(t['device_data']['lux']) for t in items if t['device_data']['id'] == s]
        tmps = [int(t['device_data']['temp']) for t in items if t['device_data']['id'] == s]
        #print(lxs)
        
        #1 - the latest values received from all the sensors of a specified device.
        dev_to_data[s]['latest_lux'] = [t['device_data']['lux'] for t in ll if t['device_data']['id'] == s][0]
        dev_to_data[s]['latest_temp'] = [t['device_data']['temp'] for t in ll if t['device_data']['id'] == s][0]
        
        #2 the aggregated values (e.g., average, minimum and maximum) for each sensor of a specified device, during the last hour.
        dev_to_data[s]['lux_avg'] = sum(lxs)/len(lxs)
        dev_to_data[s]['lux_max'] = max(lxs)
        dev_to_data[s]['lux_min'] = min(lxs)
        
        dev_to_data[s]['tmp_avg'] = sum(tmps)/len(tmps)
        dev_to_data[s]['tmp_max'] = max(tmps)
        dev_to_data[s]['tmp_min'] = min(tmps)
        
        #3 - Prob: lxs & tmps
        dev_to_data[s]['all_lux'] = lxs
        dev_to_data[s]['all_temp'] = tmps
        
        dev_to_data[s]['lamp'] = [t['device_data']['lamp'] for t in ll if t['device_data']['id'] == s][0]
        dev_to_data[s]['led'] = [t['device_data']['led'] for t in ll if t['device_data']['id'] == s][0]
    #print(dev_to_data)    
    

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
    
    'devs':list(devs),
    'dev_data': dev_to_data,
    # 'all_lux': all_lux,
    # 'all_temp': all_temp,
    
    #'lamp': int(latest['device_data']['lamp']),
    #'led': int(latest['device_data']['led'])
    }