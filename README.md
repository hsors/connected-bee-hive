# ardbeescale-for-dummies
 
This repo includes the code of a a connected bee hive project together with a 65 pages document explaining the project in all details, including hardware and software 

My implementation is classical (Arduino/LoRawAN/NodeRED/InfluxDB/Grafana) but I also have included a Google Sheet integration that is useful for calibration purposes (see explantion in the pdf document). My code includes a weight temperature compensation with a nonlinear model which has proven to be effective and which is described in the document. My connected bee hives include a small sirup pump allowing to feed the hives remotely. This is both convenient when the apiary is not close by and very effective to develop the hives in the spring or before winter (feeding hives with a small quantity of sugar very often seems to be more effective than a bigger quantity less often). The application includes a smartphone app from which the user can trigger the pumps.

My apiary being remote from any LoRaWAN gateway and from any power source, I have added a gateway which is powered with a 12V battery charged with PV panels. The gateway being quite power angry, it is switched on by an “energy saving device” only when uploads are transmitted by the scales. This "energy saving device" is also described in the pdf document. My code includes therefore a mechanism which synchronizes the time of each scale and the one of the energy saver device with TTN time

