# IoT-2021-Individual-Assigments - Power Saver - #2
Individual assignments for the IoT 2021 Course @ Sapienza University of Rome

Web dashboard: https://dev867.dyaycgfnuds5z.amplifyapp.com/

## 1. Questions
### 1.1. How is the deployment of multiple sensors going to affect the IoT platform?
For this kind of application, it is enough to deploy 1 single device per room, as the light intensity and temperature measures will hardly differ in the case multiple devices are placed in the same ambient, if the device is placed in a central position in the room.  
The benefit of deploying multiple sensors in this case may help in terms of the analysis of the data sensed in multiple rooms to adjust the thresholds that lead the cloud system to trigger the actuators and possibly find common ones for all the rooms on the same floor inside a building.

The main limitation of a multi-hop wireless network in this case is that it will take a while to send the data to the cloud and receive a response, which may also get lost while travelling, thus leading to power wastes until the next iteration of the sensing. 

### 1.2. What are the connected components, the protocols to connect them and the overall IoT architecture?
* **Network diagram**
![alt text](images/net_diagram2.png "Network diagram")

The system used for the experiment uses 2 kinds of nodes from the testbed:
* 2+ [IoT-Lab M3](https://www.iot-lab.info/docs/boards/iot-lab-m3/) nodes, one of which acts as border router of the mesh while the remaining ones run the application developed for the 1st assignment, adequately modified to use the new communication technology and the sensors from the m3 nodes instead of the STM32 nucleo board.
* 1 [IoT-LAB A8-M3](https://www.iot-lab.info/docs/boards/iot-lab-a8-m3/) node, which hosts both the MQTT/SN broker and the MQTT transparent bridge towards the AWS cloud infrastructure.

* **Software components** -
The cloud components remain unchanged with respect to the first assignment, except for a few adjustments to the code of the lambdas to handle multiple devices. Two additional software components have been employed for this assignment, both at the testbed level:
    - The [generic border router provided by RIOT](https://github.com/RIOT-OS/RIOT/tree/master/examples/gnrc_border_router) in order to allow the m3 nodes to communicate with the a8 node, which bridges the communication to the clound infrastructure via MQTT.
    - A *Jupyter* notebook is employed to interact with the real-world testbed in order to submit and run the experiments. 
* **Architecture diagram**
![alt text](images/diagram2.png "Architecture diagram")
The _FIT/IoT-Lab Testbed_ node in the diagram includes the m3 nodes running the application, the m3 node running the generic border router firmware and the a8 node running the MQTT broker plus the transparent bridge.

### 1.3. How do you measure the performance of the system?
The performance of the system was measured using the consumption and radio monitoring tools offered by the FIT/IoT-Lab facility.

Three different tests were performed.
All tests were run using channel 22. Concerning the topologies' pictures, the blue dots represent the used nodes.  
In order to perform the tests in a limited time window (20-25 minutes), the devices' sleep time between sensing phases was reduced to 3 minutes.  
Additionally, to prevent multiple devices to send data at the same time, the firmware was flashed with a small delay for each node.

#### **1) Test 1:**
Duration: 20 minutes  
Nodes: 3 (1 router (red circle) + 2 devices)  
Topology: line (border router in the middle).

![alt text](images/performance/tst1/tst1.png "test1 topology")

**Consumptions**:  
Router (m3_8):

![alt text](images/performance/tst1/m8/power.png)
![alt text](images/performance/tst1/m8/current.png)
![alt text](images/performance/tst1/m8/voltage.png)
![alt text](images/performance/tst1/m8/rssi.png)

Node #1 (m3_7):

![alt text](images/performance/tst1/m7/power.png)
![alt text](images/performance/tst1/m7/current.png)
![alt text](images/performance/tst1/m7/voltage.png)
![alt text](images/performance/tst1/m7/rssi.png)

Node #2 (m3_9):  

![alt text](images/performance/tst1/m9/power.png)
![alt text](images/performance/tst1/m9/current.png)
![alt text](images/performance/tst1/m9/voltage.png)
![alt text](images/performance/tst1/m9/rssi.png)


#### **2) Test 2:**
Duration: 25 minutes  
Nodes: 4 (1 router (red circle) + 3 devices)  
Topology: diamond.

![alt text](images/performance/tst2/tst2.png "test2 topology")

**Consumptions**:  
Router (m3_8):
![alt text](images/performance/tst2/m8/power.png)
![alt text](images/performance/tst2/m8/current.png)
![alt text](images/performance/tst2/m8/voltage.png)
![alt text](images/performance/tst2/m8/rssi.png)

Node #1 (m3_2):
![alt text](images/performance/tst2/m2/power.png)
![alt text](images/performance/tst2/m2/current.png)
![alt text](images/performance/tst2/m2/voltage.png)
![alt text](images/performance/tst2/m2/rssi.png)

Node #2 (m3_4):  
![alt text](images/performance/tst2/m4/power.png)
![alt text](images/performance/tst2/m4/current.png)
![alt text](images/performance/tst2/m4/voltage.png)
![alt text](images/performance/tst2/m4/rssi.png)

Node #3 (m3_6):  
![alt text](images/performance/tst2/m6/power.png)
![alt text](images/performance/tst2/m6/current.png)
![alt text](images/performance/tst2/m6/voltage.png)
![alt text](images/performance/tst2/m6/rssi.png)

#### **3) Test 3:**
Duration: 25 minutes  
Nodes: 4 (1 router (red circle) + 3 devices)  
Topology: larger diameter.

![alt text](images/performance/tst3/tst3.png "test3 topology")

**Consumptions**:  
Router (m3_6):
![alt text](images/performance/tst3/m6/power.png)
![alt text](images/performance/tst3/m6/current.png)
![alt text](images/performance/tst3/m6/voltage.png)
![alt text](images/performance/tst3/m6/rssi.png)

Node #1 (m3_1):
![alt text](images/performance/tst3/m1/power.png)
![alt text](images/performance/tst3/m1/current.png)
![alt text](images/performance/tst3/m1/voltage.png)
![alt text](images/performance/tst3/m1/rssi.png)

Node #2 (m3_7):  
![alt text](images/performance/tst3/m7/power.png)
![alt text](images/performance/tst3/m7/current.png)
![alt text](images/performance/tst3/m7/voltage.png)
![alt text](images/performance/tst3/m7/rssi.png)

Node #3 (m3_12):  
![alt text](images/performance/tst3/m12/power.png)
![alt text](images/performance/tst3/m12/current.png)
![alt text](images/performance/tst3/m12/voltage.png)
![alt text](images/performance/tst3/m12/rssi.png)


### **Conclusions**
#### **Wireless performance**
As expected from and stated in the [iot-lab docs](https://www.iot-lab.info/docs/tools/radio-monitoring/) the measured RSSI for all the node in all the experiments is always near -91dBm, meaning there was not too much noise nor interference while performing the tests.  

Nevertheless, by analyzing the nodes from the inside (via *nc*) and the logs from the border router and the **MQTT** broker on the A8 node, when the topology becomes more sparse and the number of nodes increases (test 2 and test 3), also the network's **latency icreases**: sometimes the messages from the devices did not manage to correctly arrive to the router and thus to the broker, as a consequence they were not published and bridged to AWS IoT. This may be either due to interferences generated by other experiments on other channels or to possible collisions of the messages from the different m3 nodes (despite the attempt, as stated above, to differentiate the time when each sensor was supposed to send messages by starting the nodes not at the same time).

#### **Power consumption**
The power, current and voltage consumptions are equivalent among the routers belonging to the different tests, which is expected as the firmware is the same for all of them.

What stated above is generally true also for the devices, where the first major spike most likely correspond to the flashing of the firmware, while the other minor spikes (generally 5, which is expected as the devices are supposed to sample every 3 minutes) probably correspond to the devices waking up for the sensing phase.

The overall consumption of devices running the *power saver* firmware could be significantly reduced by introducing deep sleep for the devices and waking them only when they need to sense the ambient. This was not done in this case because it would have prevented the leds from staying on all the time.

## 2. Hands-on Walkthrough
### IoT-Lab Setup
In order to set up the FIT/IoT-Lab enviroment, it is enough to follow the Jupyter notebook provided in the _iot-lab_ folder, which explains every step to perform in order to reproduce the experiment, flash the firmwares and run the MQTT/SN broker plus the transparent bridge. 
### Remote setup
0) Create an application on *AWS IoT Core* & download the certificates and private keys from the IoT core to be used by the `mosquitto` instance on your local machine.
1) Set up the *IoT core* rule as in `./iot_core/rules.sql`. Two actions should be linked to the rule: 
    1. putting the data in a *DynamoDB* table (using `${timestamp()}` as primary key and `${id}` as sort key) in the `device_data` column; 
    2. sending a message to a lambda function (`./lambda.py`).
2) Add the other lambdas that can be found in the `./iot_core` folder to aws lambda and set up an *AWS API Gateway* for each one of them.
3) Create a website on *AWS Amplify* using the code for the web dashboard in `./web_dashboard/index.html`

*Note: some of the steps above may require creating and correctly configuring roles and policies in the *IAM Console* in order for everything to work as intended.*


## Extra: Web Dashboard Example
![alt text](images/1v2.png "Dashboard")
![alt text](images/2v2.png "Dashboard")

