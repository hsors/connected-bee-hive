# ardbeescale-for-dummies
![photo - 1](https://user-images.githubusercontent.com/40210696/232249468-a80b5a2e-59b0-420f-b47d-cf51f5ccfbb2.jpeg)

This repo includes the code of a a connected bee hive project together with a 65 pages document explaining the project in all details, including hardware and software 



My implementation is classical (Arduino/LoRaWan/NodeRED/InfluxDB/Grafana) but I also have included a Google Sheet integration retrieving the hive data in a google sheet, which is convenient for calibration purposes. The data being transmitted are weight, external temperature, humidity and pressure plus internal brood temperature. The data are transmitted every 15mn by default but this is configurable by sending an ad hoc downlink to the hive (a python script is provided to send downlinks to the hives). 

My code includes a weight temperature compensation algorithm using a nonlinear model which has proven to be effective and which is described precisely in the document. 

My connected bee hives include a small sirup pump allowing to feed the hives remotely. This is both convenient when the apiary is not close by and very effective to develop the hives in the spring or before winter (feeding with a small quantity of sugar very often seems to be more effective than a bigger quantity less often). The application includes a smartphone app from which the user can trigger the pumps (the app is communicating with a set of python scripts which send some uplink information to the app and get input from the app to send downlinks to the hives). From the same app, it’s possible to send a downlink which prevent the hive from sending data during a certain pre-determined time (when hives are open, we want to avoid to plot a wrong point on the hive weight curve). 

My apiary being too far away from the closest LoRaWAN gateway I had to add one (with the 4G option), which is powered from a 12V battery charged with PV panels. The gateway being quite power angry, it is switched on by an “energy saving device” only when uploads are transmitted by the scales. My code includes therefore a mechanism which synchronizes the time of each scale and the one of the energy saver device with TTN time. The  time synchronization logic is controlled by two python scripts which can be launched either manually or at regular time intervals such as once a week, to set the time of the nodes: the first script is listening to the next uplink sent by a node. The second one is triggered when this next uplink is received, at which time it sends a “set time” downlink to the node with the time of this last received uplink to which the duration between two measures is added.
