# IoT-2021-Individual-Assigments - Power Saver
Individual assignments for the IoT 2021 Course @ Sapienza University of Rome

Web dashboard: https://dev867.dyaycgfnuds5z.amplifyapp.com/

## Repository structure
This repository is organized on multiple branches in order to separately handle the 3 different assignments for the course. In particular:
### The _assignment1_ branch  
* The [_assignment1_ branch](https://github.com/Pg96/IoT-2021-Individual-Assigments/tree/assignment_1) contains all the code, scripts, etc. generated during the completion of the **first** assignment, which uses a single physical STM-32 nucleo device to sense the environment, send the gathered information to the cloud and receive instructions about how to trigger the actuators accordingly.
### The _assignment2_ branch  
* The [_assignment2_ branch](https://github.com/Pg96/IoT-2021-Individual-Assigments/tree/assignment_2) contains all the code, scripts, etc. generated during the completion of the **second** assignment, which uses an arbitrary number of m3 nodes from the FIT/IoT-Lab real-world testbed communicating with each other via the _6LoWPAN_ protocol over _802.15.4 mesh networking_ technologies.
### The _assignment3_ branch  
* The [_assignment3_ branch](https://github.com/Pg96/IoT-2021-Individual-Assigments/tree/assignment_3) - contains all the code, scripts, etc. generated during the completion of the **third** assignment, which uses ab arbitrary number of st-lrwan1 nodes from the FIT/IoT-Lab real-world testbed communicating via the _LoRAWAN_ technology using _The Things Network_ to reach the AWS cloud infrastructure.
### The _main_ branch  
* The _main_ branch is designed to keep compatibility and interoperability of the device developed for the first assignment with the updated scripts and codes from AWS developed and modified to complete the other 2 assignments. It is mainly used to test & debug the interaction with the AWS facilities without employing the nodes from FIT IoT-Lab, which are generally slow to deploy and not so easy to interact with.


Each branch contains a custom _README.md_ file that addresses the questions and the requests issued by each specific assignment.
