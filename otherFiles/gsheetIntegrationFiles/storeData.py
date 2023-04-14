#!/usr/bin/python
# -*- coding: utf-8 -*-
# listen directory /home/pi/Documents/hiveData/application_id. 
# If a new file is created in it, this script will process it
# See: http://github.com/seb-m/pyinotify/wiki/Tutorial and file tutorial_notifier.py
import pyinotify
import pickle
import gsheetLib
import sys
import os
import logging
import importlib
import datetime
import gspread
import time

apiaryName = sys.argv[1] # uncomment when script is launched from command line with argument
#apiaryName ="ardbeescale-af01" # comment when script is launched from command line with argument
confFile = "config_"+apiaryName
print(confFile)
modImport = importlib.import_module(confFile)

def debugInfo(apiaryName, text):
    confFile = "config_"+apiaryName
    modImport = importlib.import_module(confFile)
    logging.basicConfig(filename=modImport.logFile, level=logging.INFO)
    logOutput = modImport.logOutput
    if logOutput != "none":
        if logOutput == "print":
            print(text)
        if logOutput == "log":
            logging.info(text)
    return

wm = pyinotify.WatchManager()  # Watch Manager
mask = pyinotify.IN_DELETE | pyinotify.IN_CREATE | pyinotify.IN_CLOSE_WRITE # watched events
global gsheetBlocked # Flag enabling to block access to gsheet file while it's being accessed
gsheetBlocked = False
class EventHandler(pyinotify.ProcessEvent):
    def process_IN_CLOSE_WRITE(self, event):
        #print("File ",event.pathname," just closed")
        global gsheetBlocked
        global apiaryName
        # get device_id & date/time from file name:
        path = event.pathname
        device_id=path[path.index("#")+1:] # device_id is the last portion of the file name
        year =  path[path.index("T")-10: path.index("T")-10+4]
        month = path[path.index("T")-5: path.index("T")-5+2]
        day = path[path.index("T")-2: path.index("T")-2+2]
        hour = path[path.index("T")+1: path.index("T")+1+2]
        mn = path[path.index("T")+4: path.index("T")+4+2]
        # retreive corresponding uplink data
        try:
            myFile = open(path,'rb')
        except:
            print("the file ",path," doesn't exist")
            return
            
        uplinkList = pickle.load(myFile)
        print("File:",event.pathname," uplinkList:",uplinkList)
        myFile.close()
        # check if this measure correspond to a new line in the gsheet file or if it just needs to be added to an existing line
        # This is done by checking if the time of last row of the gsheet file and the uplink received time are the same, modulo
        # the time interval between 2 measures for this apiary (all nodes of an apiary must have the same time interval)
        credentialsFileName = apiaryName+"_secret.json"
        from oauth2client.service_account import ServiceAccountCredentials
        scope = ['https://spreadsheets.google.com/feeds', 'https://www.googleapis.com/auth/drive']
        credentials = ServiceAccountCredentials.from_json_keyfile_name(credentialsFileName, scope)
        gc = gspread.authorize(credentials)
        # open the file:
        gsheetFile = gc.open(apiaryName)
        # select ST sheet:
        trySetCurrentSheetOK, currentSheet = gsheetLib.trySetCurrentSheet(apiaryName,gsheetFile,"ST")
        print("trySetCurrentSheetOK:",trySetCurrentSheetOK)
        # get the date time of last line
        tryCellValueOK, lastLineDateTime = gsheetLib.tryCellValue(apiaryName,currentSheet,modImport.lastSTrow, 1)
        # extract day from lastTimeDateTime which form is DD/MM/YYYY HH:MM:SS
        print("tryCellValueOK:",tryCellValueOK)
        lastLineDay = lastLineDateTime[0:2]
        # extract the time (HH:MM) from lastTimeDateTime which form is DD/MM/YYYY HH:MM:SS
        lastLineTime = lastLineDateTime[lastLineDateTime.index(" ")+1:lastLineDateTime.index(" ")+6]
        # calculate the time in nb of minutes since midnight
        lastLineMn = int(lastLineTime[0:2])*60+int(lastLineTime[3:5])
        # modulo measureInterval
        lastLineMnRounded = lastLineMn - lastLineMn % modImport.measureIntervalMn
        # calculate the time of last uplink received in mn since midnigh
        lastUplinkMn = 60*int(hour)+int(mn)
        # modulo measureInterval
        lastUplinkMnRounded = lastUplinkMn - lastUplinkMn % modImport.measureIntervalMn
        logText = "file time:"+str(year)+" "+str(month)+" "+str(day)+" "+str(hour)+"H"+str(mn)+"MN; lastUplinkMnRounded:"+str(lastUplinkMnRounded)+" lastLineMnRounded:"+str(lastLineMnRounded)
        debugInfo(apiaryName,logText)
        # update gsheet file:
        while gsheetBlocked == True: # wait until the file is released from other accesses
            pass
        time.sleep(5) # wait 5s to make sure the writing process is complete
        # block the gsheet file so that another process cannot access it
        gsheetBlocked = True
        if lastUplinkMnRounded != lastLineMnRounded: # new line in gsheet file
            # calculate the XLdate corresponding to the rounded dateTime of the uplink
            hour = str(int(lastUplinkMnRounded/60))
            mn = str(round(60 * (lastUplinkMnRounded/60 - int(lastUplinkMnRounded/60))))
            if int(mn) <10:
                mn = "0"+mn
            dateTimeString = day+"/"+month+"/"+year+" "+hour+":"+mn
            dateTimeObj = datetime.datetime.strptime(dateTimeString, '%d/%m/%Y %H:%M')
            delta = dateTimeObj - datetime.datetime(1899,12,30)
            XLdate = float(delta.days) + (float(delta.seconds) / 86400)
            # delete row #2 of ST sheet
            gsheetLib.tryDeleteRow(apiaryName,currentSheet,2)
            # Insert a new row with only the date/time of the uplink 
            rowList=[XLdate] 
            tryInsertRowOK = gsheetLib.tryInsertRow(apiaryName,currentSheet,rowList,modImport.lastSTrow) 
            # check if this is a new day (ignore last row which may not be completely filled-in yet)
            tryCellValueOK, lineBeforeLastDateTime = gsheetLib.tryCellValue(apiaryName,currentSheet,modImport.lastSTrow-2, 1)
            lastBeforeLastLineDay = lineBeforeLastDateTime[0:2]
            if lastLineDay != lastBeforeLastLineDay: # this is a new day, need to insert a new line in the long term sheet
                # get the line to be copied in the LT sheet
                tryReadRowOK, rowToBeCopied = gsheetLib.tryReadRow(apiaryName,currentSheet,modImport.lastSTrow-1)
                # select LT sheet:
                trySetCurrentSheetOK, currentSheet = gsheetLib.trySetCurrentSheet(apiaryName,gsheetFile,"LT")
                # Delete the second row at the top of the sheet
                tryDeleteRowOK = gsheetLib.tryDeleteRow(apiaryName,currentSheet,2)
                # Insert the new row at the bottom
                tryInsertRowOK = gsheetLib.tryInsertRow(apiaryName,currentSheet,rowToBeCopied,modImport.lastLTrow)
                
        # get the hive number from the device_id
        hiveNb = modImport.hiveList.index(device_id)+1
        # Update last line of ST sheet with the information received in the uplink
        # select ST sheet again
        trySetCurrentSheetOK, currentSheet = gsheetLib.trySetCurrentSheet(apiaryName,gsheetFile,"ST")
        # Load weight
        tryUpdateCellOK= gsheetLib.tryUpdateCell(apiaryName,currentSheet,modImport.lastSTrow,hiveNb+1, uplinkList[0]) # weight
        # load voltage
        tryUpdateCellOK= gsheetLib.tryUpdateCell(apiaryName,currentSheet,modImport.lastSTrow,hiveNb+13, uplinkList[4]) # Voltage
        # Depending on the hive #, load ext. temp. humidity and pressure
        if hiveNb ==   modImport.hiveSelectedForTHP: # need to update external T, H and P in gsheet file
            tryUpdateCellOK= gsheetLib.tryUpdateCell(apiaryName,currentSheet,modImport.lastSTrow,20, round(uplinkList[1],2)) # Temp
            tryUpdateCellOK= gsheetLib.tryUpdateCell(apiaryName,currentSheet,modImport.lastSTrow,21, uplinkList[3]) # Pressure
            tryUpdateCellOK= gsheetLib.tryUpdateCell(apiaryName,currentSheet,modImport.lastSTrow,22, uplinkList[2]) # Humidity
        gsheetBlocked = False # release the access to gsheet file
        os.system("rm "+path) # delete the uplink file
        
       
#    def process_IN_CREATE(self, event):
#        print ("File created:", event.pathname)

dirToWatch = modImport.hiveDataRootDir+"/"+apiaryName
handler = EventHandler()
notifier = pyinotify.Notifier(wm, handler)
wdd = wm.add_watch(dirToWatch, mask, rec=True)

notifier.loop()




