#!/usr/bin/python3
# -*- coding: utf-8 -*-

application_id = "ardbeescale-af01"
USER = "ardbeescale-af01@ttn"
PASSWORD = "XXXXXXXXXXXXXX"
PUBLIC_ADDRESS = "eu1.cloud.thethings.network"
PUBLIC_ADDRESS_PORT = 1883

# ----------- parameters needed by getStoreUplinks.py program:
# the scale and the energy devices do not send the same uplinks. They will be treated different by getStoreUplinks
# which therefore need to know from which device each uplink is coming
listOfScales = ["hso-scale01","hso-scale02","hso-scale03","hso-scale04","hso-scale05","hso-scale06"] # full list, even if some are not present
# need to define here below the scales that are providing valid weather information (Temperature, humidity & pressure). Corresponding device_id are 
# the keys of a dictionnary which values are also dictionnaries, containing the last and the previous data. This format is chosen to make programming easier
validScalesForWeatherData = {
#    "hso-scale01":{"lastT":0,"lastH":0,"lastP":0,"prevT":0,"prevH":0,"prevP":0},
#    "hso-scale02":{"lastT":0,"lastH":0,"lastP":0,"prevT":0,"prevH":0,"prevP":0},
#    "hso-scale04":{"lastT":0,"lastH":0,"lastP":0,"prevT":0,"prevH":0,"prevP":0},
    "hso-scale05":{"lastT":0,"lastH":0,"lastP":0,"prevT":0,"prevH":0,"prevP":0}
#    "hso-scale06":{"lastT":0,"lastH":0,"lastP":0,"prevT":0,"prevH":0,"prevP":0}    
    }

otherDevices = ["hso-energysaver"]
fileName = "lastUplink.dat" # the file in which the uplink content will be stored
rootDirectory = "/media/pi/PI4HOME_HDD/uplinks/" # the directory in which last uplink (of each noce) will be saved

# ---------- downlinks lists. DL orders are transmetted with a parameter specifying which downlink must be sent to which device
# Following dictionnary defines which string corresponds to which downlink to be sent on which device
DL_Dic = {
"feedScale01":["08",1], # means send 09 to device #1 (hso-scale01)
"feedScale02":["08",2],
"feedScale03":["08",3],
"feedScale04":["08",4],
"feedScale05":["10",5],
"feedScale06":["08",6],
"resetLastDL01":["80",1], # means send 80 to device #1 
"resetLastDL02":["80",2],
"resetLastDL03":["80",3],
"resetLastDL04":["80",4],
"resetLastDL05":["80",5],
"resetLastDL06":["80",6],
"holdMeasures01-1H":["C6",1], # means send C6 to device #1. CF is 6 x 10mn
"holdMeasures02-1H":["C6",2],
"holdMeasures03-1H":["C6",3],
"holdMeasures04-1H":["C6",4],
"holdMeasures05-1H":["C6",5],
"holdMeasures06-1H":["C6",6],
"holdMeasures01-2H":["CC",1], # means send CC to device #1. CC is 12 x 10mn
"holdMeasures02-2H":["CC",2],
"holdMeasures03-2H":["CC",3],
"holdMeasures04-2H":["CC",4],
"holdMeasures05-2H":["CC",5],
"holdMeasures06-2H":["CC",6],
"holdMeasures01-3H":["D2",1], # means send D2 to device #1. D2 is 18 x 10mn
"holdMeasures02-3H":["D2",2],
"holdMeasures03-3H":["D2",3],
"holdMeasures04-3H":["D2",4],
"holdMeasures05-3H":["D2",5],
"holdMeasures06-3H":["D2",6]
}

