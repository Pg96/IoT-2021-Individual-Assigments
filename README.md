# IoT-2021-Individual-Assigments - Power Saver - #3
Individual assignments for the IoT 2021 Course @ Sapienza University of Rome

Web dashboard: https://dev867.dyaycgfnuds5z.amplifyapp.com/

## 1. Questions

## 2. Hands-on Walkthrough
### IoT-Lab Setup
In order to set up the FIT/IoT-Lab enviroment, it is enough to follow the Jupyter notebook provided in the _iot-lab_ folder, which explains every step to perform in order to reproduce the experiment.
### TTN Setup
Create an application on **The Things Network** as well as a number of end devices with the following characteristics:
* LoRaWAN Version = MAC V1.0.2
* Regional Parameters Version = PHY V1.0.2 REV B
* Frequency plan = Europe 863-870MHz (SF9 for RX2)
* Activation mode = OTAA

Then follow the [documentation to integrate TTN and AWS IoT](https://www.thethingsindustries.com/docs/integrations/aws-iot/default/), as well as that for [handling messages](https://www.thethingsindustries.com/docs/integrations/aws-iot/default/messages/).
### AWS setup
0) Create an application on *AWS IoT Core*.
1) Set up the *IoT core* rule as in `./iot_core/rules.sql`. Two actions should be linked to the rule: 
    1. sending a message to a lambda function (`./iot_core/lambda.py`).
2) Add the other lambdas that can be found in the `./iot_core` folder to aws lambda and set up an *AWS API Gateway* for each one of them.
3) Create a website on *AWS Amplify* using the code for the web dashboard in `./web_dashboard/index.html`

*Note: some of the steps above may require creating and correctly configuring roles and policies in the *IAM Console* in order for everything to work as intended.*


## Extra: Web Dashboard Example
![alt text](images/1v2.png "Dashboard")
![alt text](images/2v2.png "Dashboard")

