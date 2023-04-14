# logOutput can have 3 values:
#  1) "print" to get  debug information on screen (stdout)
#  2) "log" to get debug information in file logFile.log
#  3) "none" for no debuf information
logOutput = "print"
logFile = "/home/pi/Documents/logFiles/ardbeescale-af01.log"


# list of nodes (deived_id) of each apiary
hiveList = ["hso-scale01","hso-scale02","hso-scale03","hso-scale04","hso-scale05","hso-scale06"] # pb: l'appId est ardbeescale-af01

# All hives have a temperature/humidity/pressure sensor but only one is stored in gsheet. This is how to specify it (by apiary):
hiveSelectedForTHP = 2

# root directory in which all data from different applications (= different apiaries) will be stored
# typically for apiary "sthugues" data will be stored in /home/pi/Documents/hiveData/sthugues
hiveDataRootDir = "/home/pi/Documents/hiveData"

# Time between two consecutive measures (a downlink with this information is pushed to the node when script getdataxx
# starts in order to configure the node
measureIntervalMn = 15 # all nodes of an apiary must have the same time interval

# Last line number of gSheet file (ST sheet and LT sheet)
lastSTrow = 10080 #1 week @ 1 measure/mn (column A of this line and column A of previous line must both contain de valid date)
lastLTrow = 366
