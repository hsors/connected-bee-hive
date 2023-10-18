/****************************************************************************
 *
 *  File:          LMIC-node.cpp
 * 
 *  Function:      LMIC-node main application file.
 * 
 *  Copyright:     Copyright (c) 2021 Leonel Lopes Parente
 *                 Copyright (c) 2018 Terry Moore, MCCI
 *                 Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 *
 *                 Permission is hereby granted, free of charge, to anyone 
 *                 obtaining a copy of this document and accompanying files to do, 
 *                 whatever they want with them without any restriction, including,
 *                 but not limited to, copying, modification and redistribution.
 *                 The above copyright notice and this permission notice shall be 
 *                 included in all copies or substantial portions of the Software.
 * 
 *                 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT ANY WARRANTY.
 * 
 *  License:       MIT License. See accompanying LICENSE file.
 * 
 *  Author:        Leonel Lopes Parente
 * 
 *  Description:   To get LMIC-node up and running no changes need to be made
 *                 to any source code. Only configuration is required
 *                 in platform-io.ini and lorawan-keys.h.
 * 
 *                 If you want to modify the code e.g. to add your own sensors,
 *                 that can be done in the two area's that start with
 *                 USER CODE BEGIN and end with USER CODE END. There's no need
 *                 to change code in other locations (unless you have a reason).
 *                 See README.md for documentation and how to use LMIC-node.
 * 
 *                 LMIC-node uses the concepts from the original ttn-otaa.ino 
 *                 and ttn-abp.ino examples provided with the LMIC libraries.
 *                 LMIC-node combines both OTAA and ABP support in a single example,
 *                 supports multiple LMIC libraries, contains several improvements
 *                 and enhancements like display support, support for downlinks,
 *                 separates LoRaWAN keys from source code into a separate keyfile,
 *                 provides formatted output to serial port and display
 *                 and supports many popular development boards out of the box.
 *                 To get a working node up and running only requires some configuration.
 *                 No programming or customization of source code required.
 * 
 *  Dependencies:  External libraries:
 *                 MCCI LoRaWAN LMIC library  https://github.com/mcci-catena/arduino-lmic
 *                 IBM LMIC framework         https://github.com/matthijskooijman/arduino-lmic  
 *                 U8g2                       https://github.com/olikraus/u8g2
 *                 EasyLed                    https://github.com/lnlp/EasyLed
 *
 ******************************************************************************/

#include "LMIC-node.h"


//  █ █ █▀▀ █▀▀ █▀▄   █▀▀ █▀█ █▀▄ █▀▀   █▀▄ █▀▀ █▀▀ ▀█▀ █▀█
//  █ █ ▀▀█ █▀▀ █▀▄   █   █ █ █ █ █▀▀   █▀▄ █▀▀ █ █  █  █ █
//  ▀▀▀ ▀▀▀ ▀▀▀ ▀ ▀   ▀▀▀ ▀▀▀ ▀▀  ▀▀▀   ▀▀  ▀▀▀ ▀▀▀ ▀▀▀ ▀ ▀

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "SparkFunBME280.h"
#include "DHT.h"
#include <RTCZero.h>
#include "HX711.h"
#include "hive-parameters.h"
#include <OneWire.h> 
#include <DallasTemperature.h>
#include "Arduino.h"
#include <WDTZero.h>

WDTZero MyWatchDoggy; // Define WDT (watchdog timer)

// init. BME280
BME280 myBME280;

/* Create an rtc object */
RTCZero rtc;

/* create an HX711 object */
HX711 scale;

/* Setup a oneWire instance to communicate with any OneWire devices   */
OneWire oneWire(ONE_WIRE_BUS); // ONE_WIRE_BUS is 5
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

uint8_t lastDownlink = 0; // is sent with each uplink for akn purpose

const uint8_t payloadBufferLength = 9;    // Adjust to fit max payload length

/* Change these values to set the current initial time - will be done using a downlink */
byte seconds = 0;
byte minutes = 00;
byte hours = 00;
/* Change these values to set the current initial date */
const byte day = 01;
const byte month =01;
const byte year = 00;
/* stopMeasureTimeMn is the time during which no measure will be taken (as specified by the corresponding downlink when/if received) */
int stopMeasureTimeMn = 0;

/* Temperature compensation variables here below: code explanation can be found in document rasbeescale.docx, chapter "Temperature compensation" 
parameters used for temperature compensation are stored in file scale-parameters.h (/lib/ardbeescale directory) */

double deltaText[5];
double deltaTlc[5];
double Text[6] = {T_EXT_INIT,T_EXT_INIT,T_EXT_INIT,T_EXT_INIT,T_EXT_INIT,T_EXT_INIT};


//  █ █ █▀▀ █▀▀ █▀▄   █▀▀ █▀█ █▀▄ █▀▀   █▀▀ █▀█ █▀▄
//  █ █ ▀▀█ █▀▀ █▀▄   █   █ █ █ █ █▀▀   █▀▀ █ █ █ █
//  ▀▀▀ ▀▀▀ ▀▀▀ ▀ ▀   ▀▀▀ ▀▀▀ ▀▀  ▀▀▀   ▀▀▀ ▀ ▀ ▀▀ 


uint8_t payloadBuffer[payloadBufferLength];
static osjob_t doWorkJob;
uint32_t doWorkIntervalSeconds = DO_WORK_INTERVAL_SECONDS;  // Change DO_WORK_INTERVAL_SECONDS value in platformio.ini

// Note: LoRa module pin mappings are defined in the Board Support Files.

// Set LoRaWAN keys defined in lorawan-keys.h.
#ifdef OTAA_ACTIVATION
    static const u1_t PROGMEM DEVEUI[8]  = { OTAA_DEVEUI } ;
    static const u1_t PROGMEM APPEUI[8]  = { OTAA_APPEUI };
    static const u1_t PROGMEM APPKEY[16] = { OTAA_APPKEY };
    // Below callbacks are used by LMIC for reading above values.
    void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8); }
    void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8); }
    void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16); }    
#else
    // ABP activation
    static const u4_t DEVADDR = ABP_DEVADDR ;
    static const PROGMEM u1_t NWKSKEY[16] = { ABP_NWKSKEY };
    static const u1_t PROGMEM APPSKEY[16] = { ABP_APPSKEY };
    // Below callbacks are not used be they must be defined.
    void os_getDevEui (u1_t* buf) { }
    void os_getArtEui (u1_t* buf) { }
    void os_getDevKey (u1_t* buf) { }
#endif


int16_t getSnrTenfold()
{
    // Returns ten times the SNR (dB) value of the last received packet.
    // Ten times to prevent the use of float but keep 1 decimal digit accuracy.
    // Calculation per SX1276 datasheet rev.7 §6.4, SX1276 datasheet rev.4 §6.4.
    // LMIC.snr contains value of PacketSnr, which is 4 times the actual SNR value.
    return (LMIC.snr * 10) / 4;
}


int16_t getRssi(int8_t snr)
{
    // Returns correct RSSI (dBm) value of the last received packet.
    // Calculation per SX1276 datasheet rev.7 §5.5.5, SX1272 datasheet rev.4 §5.5.5.

    #define RSSI_OFFSET            64
    #define SX1276_FREQ_LF_MAX     525000000     // per datasheet 6.3
    #define SX1272_RSSI_ADJUST     -139
    #define SX1276_RSSI_ADJUST_LF  -164
    #define SX1276_RSSI_ADJUST_HF  -157

    int16_t rssi;

    #ifdef MCCI_LMIC

        rssi = LMIC.rssi - RSSI_OFFSET;

    #else
        int16_t rssiAdjust;
        #ifdef CFG_sx1276_radio
            if (LMIC.freq > SX1276_FREQ_LF_MAX)
            {
                rssiAdjust = SX1276_RSSI_ADJUST_HF;
            }
            else
            {
                rssiAdjust = SX1276_RSSI_ADJUST_LF;   
            }
        #else
            // CFG_sx1272_radio    
            rssiAdjust = SX1272_RSSI_ADJUST;
        #endif    
        
        // Revert modification (applied in lmic/radio.c) to get PacketRssi.
        int16_t packetRssi = LMIC.rssi + 125 - RSSI_OFFSET;
        if (snr < 0)
        {
            rssi = rssiAdjust + packetRssi + snr;
        }
        else
        {
            rssi = rssiAdjust + (16 * packetRssi) / 15;
        }
    #endif

    return rssi;
}


void printEvent(ostime_t timestamp, 
                const char * const message, 
                PrintTarget target = PrintTarget::All,
                bool clearDisplayStatusRow = true,
                bool eventLabel = false)
{
    #ifdef USE_DISPLAY 
        if (target == PrintTarget::All || target == PrintTarget::Display)
        {
            display.clearLine(TIME_ROW);
            display.setCursor(COL_0, TIME_ROW);
            display.print(F("Time:"));                 
            display.print(timestamp); 
            display.clearLine(EVENT_ROW);
            if (clearDisplayStatusRow)
            {
                display.clearLine(STATUS_ROW);    
            }
            display.setCursor(COL_0, EVENT_ROW);               
            display.print(message);
        }
    #endif  
    
    #ifdef USE_SERIAL
        // Create padded/indented output without using printf().
        // printf() is not default supported/enabled in each Arduino core. 
        // Not using printf() will save memory for memory constrainted devices.
        String timeString(timestamp);
        uint8_t len = timeString.length();
        uint8_t zerosCount = TIMESTAMP_WIDTH > len ? TIMESTAMP_WIDTH - len : 0;

        if (target == PrintTarget::All || target == PrintTarget::Serial)
        {
            printChars(serial, '0', zerosCount);
            serial.print(timeString);
            serial.print(":  ");
            if (eventLabel)
            {
                serial.print(F("Event: "));
            }
            serial.println(message);
        }
    #endif   
}           

void printEvent(ostime_t timestamp, 
                ev_t ev, 
                PrintTarget target = PrintTarget::All, 
                bool clearDisplayStatusRow = true)
{
    #if defined(USE_DISPLAY) || defined(USE_SERIAL)
        printEvent(timestamp, lmicEventNames[ev], target, clearDisplayStatusRow, true);
    #endif
}


void printFrameCounters(PrintTarget target = PrintTarget::All)
{
    #ifdef USE_DISPLAY
        if (target == PrintTarget::Display || target == PrintTarget::All)
        {
            display.clearLine(FRMCNTRS_ROW);
            display.setCursor(COL_0, FRMCNTRS_ROW);
            display.print(F("Up:"));
            display.print(LMIC.seqnoUp);
            display.print(F(" Dn:"));
            display.print(LMIC.seqnoDn);        
        }
    #endif

    #ifdef USE_SERIAL
        if (target == PrintTarget::Serial || target == PrintTarget::All)
        {
            printSpaces(serial, MESSAGE_INDENT);
            serial.print(F("Up: "));
            serial.print(LMIC.seqnoUp);
            serial.print(F(",  Down: "));
            serial.println(LMIC.seqnoDn);        
        }
    #endif        
}      


void printSessionKeys()
{    
    #if defined(USE_SERIAL) && defined(MCCI_LMIC)
        u4_t networkId = 0;
        devaddr_t deviceAddress = 0;
        u1_t networkSessionKey[16];
        u1_t applicationSessionKey[16];
        LMIC_getSessionKeys(&networkId, &deviceAddress, 
                            networkSessionKey, applicationSessionKey);

        printSpaces(serial, MESSAGE_INDENT);    
        serial.print(F("Network Id: "));
        serial.println(networkId, DEC);

        printSpaces(serial, MESSAGE_INDENT);    
        serial.print(F("Device Address: "));
        serial.println(deviceAddress, HEX);

        printSpaces(serial, MESSAGE_INDENT);    
        serial.print(F("Application Session Key: "));
        printHex(serial, applicationSessionKey, 16, true, '-');

        printSpaces(serial, MESSAGE_INDENT);    
        serial.print(F("Network Session Key:     "));
        printHex(serial, networkSessionKey, 16, true, '-');
    #endif
}


void printDownlinkInfo(void)
{
    #if defined(USE_SERIAL) || defined(USE_DISPLAY)

        uint8_t dataLength = LMIC.dataLen;
        // bool ackReceived = LMIC.txrxFlags & TXRX_ACK;

        int16_t snrTenfold = getSnrTenfold();
        int8_t snr = snrTenfold / 10;
        int8_t snrDecimalFraction = snrTenfold % 10;
        int16_t rssi = getRssi(snr);

        uint8_t fPort = 0;        
        if (LMIC.txrxFlags & TXRX_PORT)
        {
            fPort = LMIC.frame[LMIC.dataBeg -1];
        }        

        #ifdef USE_DISPLAY
            display.clearLine(EVENT_ROW);        
            display.setCursor(COL_0, EVENT_ROW);
            display.print(F("RX P:"));
            display.print(fPort);
            if (dataLength != 0)
            {
                display.print(" Len:");
                display.print(LMIC.dataLen);                       
            }
            display.clearLine(STATUS_ROW);        
            display.setCursor(COL_0, STATUS_ROW);
            display.print(F("RSSI"));
            display.print(rssi);
            display.print(F(" SNR"));
            display.print(snr);                
            display.print(".");                
            display.print(snrDecimalFraction);                      
        #endif

        #ifdef USE_SERIAL
            printSpaces(serial, MESSAGE_INDENT);    
            serial.println(F("Downlink received"));

            printSpaces(serial, MESSAGE_INDENT);
            serial.print(F("RSSI: "));
            serial.print(rssi);
            serial.print(F(" dBm,  SNR: "));
            serial.print(snr);                        
            serial.print(".");                        
            serial.print(snrDecimalFraction);                        
            serial.println(F(" dB"));

            printSpaces(serial, MESSAGE_INDENT);    
            serial.print(F("Port: "));
            serial.println(fPort);
   
            if (dataLength != 0)
            {
                printSpaces(serial, MESSAGE_INDENT);
                serial.print(F("Length: "));
                serial.println(LMIC.dataLen);                   
                printSpaces(serial, MESSAGE_INDENT);    
                serial.print(F("Data: "));
                printHex(serial, LMIC.frame+LMIC.dataBeg, LMIC.dataLen, true, ' ');
                serial.println();
            }
        #endif
    #endif
} 


void printHeader(void)
{
    #ifdef USE_DISPLAY
        display.clear();
        display.setCursor(COL_0, HEADER_ROW);
        display.print(F("LMIC-node"));
        #ifdef ABP_ACTIVATION
            display.drawString(ABPMODE_COL, HEADER_ROW, "ABP");
        #endif
        #ifdef CLASSIC_LMIC
            display.drawString(CLMICSYMBOL_COL, HEADER_ROW, "*");
        #endif
        display.drawString(COL_0, DEVICEID_ROW, deviceId);
        display.setCursor(COL_0, INTERVAL_ROW);
        display.print(F("sleep time:"));
        display.print(doWorkIntervalSeconds);
        display.print("s");
    #endif

    #ifdef USE_SERIAL
        serial.println(F("\n\nLMIC-node\n"));
        serial.print(F("Device-id:     "));
        serial.println(deviceId);            
        serial.print(F("LMIC library:  "));
        #ifdef MCCI_LMIC  
            serial.println(F("MCCI"));
        #else
            serial.println(F("Classic [Deprecated]")); 
        #endif
        serial.print(F("Activation:    "));
        #ifdef OTAA_ACTIVATION  
            serial.println(F("OTAA"));
        #else
            serial.println(F("ABP")); 
        #endif
        #if defined(LMIC_DEBUG_LEVEL) && LMIC_DEBUG_LEVEL > 0
            serial.print(F("LMIC debug:    "));  
            serial.println(LMIC_DEBUG_LEVEL);
        #endif
        serial.print(F("Sleep time:      "));
        serial.print(doWorkIntervalSeconds);
        serial.println(F(" seconds"));
        if (activationMode == ActivationMode::OTAA)
        {
            serial.println();
        }
    #endif
}     


#ifdef ABP_ACTIVATION
    void setAbpParameters(dr_t dataRate = DefaultABPDataRate, s1_t txPower = DefaultABPTxPower) 
    {
        // Set static session parameters. Instead of dynamically establishing a session
        // by joining the network, precomputed session parameters are be provided.
        #ifdef PROGMEM
            // On AVR, these values are stored in flash and only copied to RAM
            // once. Copy them to a temporary buffer here, LMIC_setSession will
            // copy them into a buffer of its own again.
            uint8_t appskey[sizeof(APPSKEY)];
            uint8_t nwkskey[sizeof(NWKSKEY)];
            memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
            memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
            LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
        #else
            // If not running an AVR with PROGMEM, just use the arrays directly
            LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
        #endif

        #if defined(CFG_eu868)
            // Set up the channels used by the Things Network, which corresponds
            // to the defaults of most gateways. Without this, only three base
            // channels from the LoRaWAN specification are used, which certainly
            // works, so it is good for debugging, but can overload those
            // frequencies, so be sure to configure the full frequency range of
            // your network here (unless your network autoconfigures them).
            // Setting up channels should happen after LMIC_setSession, as that
            // configures the minimal channel set. The LMIC doesn't let you change
            // the three basic settings, but we show them here.
            LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
            LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
            LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
            LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
            LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
            LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
            LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
            LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
            LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
            // TTN defines an additional channel at 869.525Mhz using SF9 for class B
            // devices' ping slots. LMIC does not have an easy way to define set this
            // frequency and support for class B is spotty and untested, so this
            // frequency is not configured here.
        #elif defined(CFG_us915) || defined(CFG_au915)
            // NA-US and AU channels 0-71 are configured automatically
            // but only one group of 8 should (a subband) should be active
            // TTN recommends the second sub band, 1 in a zero based count.
            // https://github.com/TheThingsNetwork/gateway-conf/blob/master/US-global_conf.json
            LMIC_selectSubBand(1);
        #elif defined(CFG_as923)
            // Set up the channels used in your country. Only two are defined by default,
            // and they cannot be changed.  Use BAND_CENTI to indicate 1% duty cycle.
            // LMIC_setupChannel(0, 923200000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
            // LMIC_setupChannel(1, 923400000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);

            // ... extra definitions for channels 2..n here
        #elif defined(CFG_kr920)
            // Set up the channels used in your country. Three are defined by default,
            // and they cannot be changed. Duty cycle doesn't matter, but is conventionally
            // BAND_MILLI.
            // LMIC_setupChannel(0, 922100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);
            // LMIC_setupChannel(1, 922300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);
            // LMIC_setupChannel(2, 922500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);

            // ... extra definitions for channels 3..n here.
        #elif defined(CFG_in866)
            // Set up the channels used in your country. Three are defined by default,
            // and they cannot be changed. Duty cycle doesn't matter, but is conventionally
            // BAND_MILLI.
            // LMIC_setupChannel(0, 865062500, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);
            // LMIC_setupChannel(1, 865402500, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);
            // LMIC_setupChannel(2, 865985000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);

            // ... extra definitions for channels 3..n here.
        #endif

        // Disable link check validation
        LMIC_setLinkCheckMode(0);

        // TTN uses SF9 for its RX2 window.
        LMIC.dn2Dr = DR_SF9;

        // Set data rate and transmit power (note: txpow is possibly ignored by the library)
        LMIC_setDrTxpow(dataRate, txPower);    
    }
#endif //ABP_ACTIVATION


void initLmic(bit_t adrEnabled = 1,
              dr_t abpDataRate = DefaultABPDataRate, 
              s1_t abpTxPower = DefaultABPTxPower) 
{
    // ostime_t timestamp = os_getTime();

    // Initialize LMIC runtime environment
    os_init();
    // Reset MAC state
    LMIC_reset();

    #ifdef ABP_ACTIVATION
        setAbpParameters(abpDataRate, abpTxPower);
    #endif

    // Enable or disable ADR (data rate adaptation). 
    // Should be turned off if the device is not stationary (mobile).
    // 1 is on, 0 is off.
    LMIC_setAdrMode(adrEnabled);

    if (activationMode == ActivationMode::OTAA)
    {
        #if defined(CFG_us915) || defined(CFG_au915)
            // NA-US and AU channels 0-71 are configured automatically
            // but only one group of 8 should (a subband) should be active
            // TTN recommends the second sub band, 1 in a zero based count.
            // https://github.com/TheThingsNetwork/gateway-conf/blob/master/US-global_conf.json
            LMIC_selectSubBand(1); 
        #endif
    }

    // Relax LMIC timing if defined
    #if defined(LMIC_CLOCK_ERROR_PPM)
        uint32_t clockError = 0;
        #if LMIC_CLOCK_ERROR_PPM > 0
            #if defined(MCCI_LMIC) && LMIC_CLOCK_ERROR_PPM > 4000
                // Allow clock error percentage to be > 0.4%
                #define LMIC_ENABLE_arbitrary_clock_error 1
            #endif    
            clockError = (LMIC_CLOCK_ERROR_PPM / 100) * (MAX_CLOCK_ERROR / 100) / 100;
            LMIC_setClockError(clockError);
        #endif

        #ifdef USE_SERIAL
            serial.print(F("Clock Error:   "));
            serial.print(LMIC_CLOCK_ERROR_PPM);
            serial.print(" ppm (");
            serial.print(clockError);
            serial.println(")");            
        #endif
    #endif

    #ifdef MCCI_LMIC
        // Register a custom eventhandler and don't use default onEvent() to enable
        // additional features (e.g. make EV_RXSTART available). User data pointer is omitted.
        LMIC_registerEventCb(&onLmicEvent, nullptr);
    #endif
}

//interrupt service routine (ISR), called when interrupt is triggered 
//executes after MCU wakes up
void ISR()
{
  // do nothing
}

void goToSleepDuration(int sleepTimeSec)  // this routine is called when a command is received to hold measures during a certain time 
{
  int sleepHours = (int) sleepTimeSec/3600;
  int sleepMn = (int) (sleepTimeSec - sleepHours * 3600)/60;
  int sleepSec = (int) sleepTimeSec - sleepHours * 3600 - sleepMn * 60;
  int currentHour = rtc.getHours();
  int currentMn = rtc.getMinutes();
  int currentSec = rtc.getSeconds();
  int alarmSec = 0;
  int alarmMn = (currentMn + sleepMn + (currentSec+sleepSec)/60) % 60;
  int alarmHour = (currentHour +sleepHours + ((currentMn+sleepMn)+(currentSec+sleepSec)/60)/60) % 24;
  Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
  serial.print("Holding measure until.... ");serial.print(alarmHour);serial.print("H");serial.print(alarmMn);serial.print("MN=");serial.print(alarmSec);serial.println("sec=");
  rtc.enableAlarm(rtc.MATCH_HHMMSS);
  rtc.attachInterrupt(ISR);
  rtc.setAlarmTime(alarmHour, alarmMn, alarmSec);
  rtc.standbyMode();
}

void gotoSleep() // this routine is called after each cycle in order to put the process in sleep mode until next measure.
// The processor will wake-up at "round" time, i.e xHH00MN00EC if intervalMn= 60, or at xxH00MN00SEC and xxH30MN00SEC is intervalMn=30...
// the next "round" time is the wake-up time (ex: if currentMn=13 and intervalMn=5, the wake-up time will be at xxH15MN) 

// explanation on the way the code works: at any given time, the calculated wake-up time should be the next "round time", i.e 0mn, 15mn, 30mn, 45mn
// in the case intervalMn=15. The code calculates the interger portion of current time (in mn: cuurentMn) divided by timeInterval. It gives 0 if currentMn<15,
// 1 if currentMn is betwen 15 and 29, 2 if currentMn is between 30 and 44 and 3 if currentMn is between 45 and 59. This result is multiplied by 15 and 15 is added to
// the result. The end result is 15 if currentMn is between 0 and 14, 30 is currentMn is between 15 and 29, 45 is currentMn is between 30 and 44 and 60 if currentMn is
// betwwen 45 and 59. We take this result modulus 60 because we want 60 to become 0 and in this case, alarmHour is incremented by 1.

{
  int currentHour = rtc.getHours();
  int currentMn = rtc.getMinutes();
  int currentSec = rtc.getSeconds();
  int alarmSec = currentSec;
  int alarmMn = 0;
  int alarmHour = 0;
  int intervalMn = doWorkIntervalSeconds/60;
  int intervalHour = doWorkIntervalSeconds/3600;
  switch (intervalMn) // calculate the wake-up time based on the measure interval
  {
    case 1: // 1mn between two consecutive measures
        alarmMn = (currentMn+1) % 60;
        if (currentMn == 59) {alarmHour=(currentHour+1)%24;} else {alarmHour=currentHour;}
        Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
        Serial.print("1mn interval. wake-up at ");Serial.print(alarmHour);Serial.print(":");Serial.print(alarmMn);Serial.print("MN");Serial.print(alarmSec);Serial.println("SEC");
    break;
    case 2: // 2mn between two consecutive measures
        alarmMn = (intervalMn * (currentMn/intervalMn) + intervalMn) % 60; // converts - for instance - 13 into 6 + 2 in case currentMn = 13
        if (alarmMn < intervalMn) {alarmHour =(currentHour+1)%24;} else {alarmHour = currentHour;}
        Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
        Serial.print("2mn interval. wake-up at ");Serial.print(alarmHour);Serial.print(":");Serial.println(alarmMn);
    break;
    case 5: // 5mn between two consecutive measures
        alarmMn = (intervalMn * (currentMn/intervalMn) + intervalMn) % 60; // converts - for instance - 13 into 10 + 5 in case currentMn = 13
        if (alarmMn < intervalMn) {alarmHour = (currentHour+1)%24;} else {alarmHour = currentHour;}
        Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
        Serial.print("5mn interval. wake-up at ");Serial.print(alarmHour);Serial.print(":");Serial.println(alarmMn);
    break;
    case 10: // 10mn between two consecutive measures
        alarmMn = (intervalMn * (currentMn/intervalMn) + intervalMn) % 60; // converts - for instance - 13 into 10 + 10 in case currentMn = 13
        if (alarmMn < intervalMn) {alarmHour = (currentHour+1)%24;} else {alarmHour = currentHour;}
        Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
        Serial.print("10mn interval. wake-up at ");Serial.print(alarmHour);Serial.print(":");Serial.println(alarmMn);
    break;
    case 15: // 15mn between two consecutive measures
        alarmMn = (intervalMn * (currentMn/intervalMn) + intervalMn) % 60; // converts - for instance - 13 into 0 + 15 in case currentMn = 13
        if (alarmMn < intervalMn) {alarmHour = (currentHour+1)%24;} else {alarmHour = currentHour;}
        Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
        Serial.print("15mn interval. wake-up at ");Serial.print(alarmHour);Serial.print(":");Serial.println(alarmMn);
    break;
    case 30: // 30mn between two consecutive measures
        alarmMn = (intervalMn * (currentMn/intervalMn) + intervalMn) % 60; // converts - for instance - 13 into 0 + 30 in case currentMn = 13
        if (alarmMn < intervalMn) {alarmHour = (currentHour+1)%24;} else {alarmHour = currentHour;}
        Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
        Serial.print("30mn interval. wake-up at ");Serial.print(alarmHour);Serial.print(":");Serial.println(alarmMn);

    break;
    case 60: // 1H between two consecutive measures
        alarmMn = 0;
        alarmHour = (currentHour + intervalHour) % 24;
        Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
        Serial.print("1H interval. wake-up at ");Serial.print(alarmHour);Serial.print(":");Serial.println(alarmMn);
    break;
    case 6*60: // 6H between two consecutive measures
        alarmMn = 0;
        alarmHour = (currentHour + intervalHour) % 24;
        Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
        Serial.print("6H interval. wake-up at ");Serial.print(alarmHour);Serial.print(":");Serial.println(alarmMn);
    break;
    case 12*60: // 12H between two consecutive measures
        alarmMn = 0;
        alarmHour = (currentHour + intervalHour) % 24;
        Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
        Serial.print("12H interval. wake-up at ");Serial.print(alarmHour);Serial.print(":");Serial.println(alarmMn);

    break;
    case 24*60: // 24H between two consecutive measures
        alarmMn = 0;
        alarmHour = 0;
        Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
        Serial.print("24H interval. wake-up at ");Serial.print(alarmHour);Serial.print(":");Serial.println(alarmMn);

    break;
  }
  rtc.enableAlarm(rtc.MATCH_HHMMSS);
  rtc.attachInterrupt(ISR);
  rtc.setAlarmTime(alarmHour, alarmMn, alarmSec);
  rtc.standbyMode();
}

#ifdef MCCI_LMIC 
void onLmicEvent(void *pUserData, ev_t ev)
#else
void onEvent(ev_t ev) 
#endif
{
    // LMIC event handler
    ostime_t timestamp = os_getTime(); 

    switch (ev) 
    {
#ifdef MCCI_LMIC
        // Only supported in MCCI LMIC library:
        case EV_RXSTART:
            // Do not print anything for this event or it will mess up timing.
            break;

        case EV_TXSTART:
            // setTxIndicatorsOn();
            printEvent(timestamp, ev);            
            break;               

        case EV_JOIN_TXCOMPLETE:
        case EV_TXCANCELED:
            setTxIndicatorsOn(false);
            printEvent(timestamp, ev);
            break;               
#endif
        case EV_JOINED:
            setTxIndicatorsOn(false);
            printEvent(timestamp, ev);
            printSessionKeys();

            // Disable link check validation.
            // Link check validation is automatically enabled
            // during join, but because slow data rates change
            // max TX size, it is not used in this example.                    
            LMIC_setLinkCheckMode(0);

            // The doWork job has probably run already (while
            // the node was still joining) and have rescheduled itself.
            // Cancel the next scheduled doWork job and re-schedule
            // for immediate execution to prevent that any uplink will
            // have to wait until the current doWork interval ends.
            os_clearCallback(&doWorkJob);
            os_setCallback(&doWorkJob, doWorkCallback);
            break;

        case EV_TXCOMPLETE:
            // Transmit completed, includes waiting for RX windows.
            setTxIndicatorsOn(false);   
            printEvent(timestamp, ev);
            printFrameCounters();

            // Check if downlink was received
            Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
            serial.print("LMIC.dataLen=");
            serial.print(LMIC.dataLen);
            serial.print(" - LMIC.dataBeg=");
            serial.println(LMIC.dataBeg);
            if (LMIC.dataLen != 0 || LMIC.dataBeg != 0)
            {
                uint8_t fPort = 0;
                if (LMIC.txrxFlags & TXRX_PORT)
                {
                    fPort = LMIC.frame[LMIC.dataBeg -1];
                }
                printDownlinkInfo();
                processDownlink(timestamp, fPort, LMIC.frame + LMIC.dataBeg, LMIC.dataLen);                
            }
            // HSO 09/02/2022 - once the downlink has been processed, put the controller in sleep mode
            Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
            Serial.println("Now going to sleep...");
            digitalWrite(LED_BUILTIN,LOW); // make sure built-in LED is OFF
            gotoSleep(); // go to sleep mode until it's time to take next measure
            Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
            Serial.println("Now awaking...");
            os_setCallback(&doWorkJob, doWorkCallback); // then launch the callBack function at wake-up time 
            
            break;     

        // Below events are printed only.
        case EV_SCAN_TIMEOUT:
        case EV_BEACON_FOUND:
        case EV_BEACON_MISSED:
        case EV_BEACON_TRACKED:
        case EV_RFU1:                    // This event is defined but not used in code
        case EV_JOINING:        
        case EV_JOIN_FAILED:           
        case EV_REJOIN_FAILED:
        case EV_LOST_TSYNC:
        case EV_RESET:
        case EV_RXCOMPLETE:
        case EV_LINK_DEAD:
        case EV_LINK_ALIVE:
#ifdef MCCI_LMIC
        // Only supported in MCCI LMIC library:
        case EV_SCAN_FOUND:              // This event is defined but not used in code 
#endif
            printEvent(timestamp, ev);    
            break;

        default: 
            printEvent(timestamp, "Unknown Event");    
            break;
    }
}

static void doWorkCallback(osjob_t* job)
{
    // Event hander for doWorkJob. Gets called by the LMIC scheduler.
    // The actual work is performed in function processWork() which is called below.

    ostime_t timestamp = os_getTime();
    #ifdef USE_SERIAL
        serial.println();
        printEvent(timestamp, "doWork job started", PrintTarget::Serial);
    #endif    

    // Do the work that needs to be performed, except if a downlink with a "skip n measures" has been received (in this case, the global variable
    // nbCyclesNoMeasure has been set to the number of time the process work must be skipped).
    processWork(timestamp);  
}

lmic_tx_error_t scheduleUplink(uint8_t fPort, uint8_t* data, uint8_t dataLength, bool confirmed = false)
{
    // This function is called from the processWork() function to schedule
    // transmission of an uplink message that was prepared by processWork().
    // Transmission will be performed at the next possible time

    ostime_t timestamp = os_getTime();
    printEvent(timestamp, "Packet queued");

    lmic_tx_error_t retval = LMIC_setTxData2(fPort, data, dataLength, confirmed ? 1 : 0);
    timestamp = os_getTime();

    if (retval == LMIC_ERROR_SUCCESS)
    {
        #ifdef CLASSIC_LMIC
            // For MCCI_LMIC this will be handled in EV_TXSTART        
            setTxIndicatorsOn();  
        #endif        
    }
    else
    {
        String errmsg; 
        #ifdef USE_SERIAL
            errmsg = "LMIC Error: ";
            #ifdef MCCI_LMIC
                errmsg.concat(lmicErrorNames[abs(retval)]);
            #else
                errmsg.concat(retval);
            #endif
            printEvent(timestamp, errmsg.c_str(), PrintTarget::Serial);
        #endif
        #ifdef USE_DISPLAY
            errmsg = "LMIC Err: ";
            errmsg.concat(retval);
            printEvent(timestamp, errmsg.c_str(), PrintTarget::Display);
        #endif         
    }
    return retval;    
}


//  █ █ █▀▀ █▀▀ █▀▄   █▀▀ █▀█ █▀▄ █▀▀   █▀▄ █▀▀ █▀▀ ▀█▀ █▀█
//  █ █ ▀▀█ █▀▀ █▀▄   █   █ █ █ █ █▀▀   █▀▄ █▀▀ █ █  █  █ █
//  ▀▀▀ ▀▀▀ ▀▀▀ ▀ ▀   ▀▀▀ ▀▀▀ ▀▀  ▀▀▀   ▀▀  ▀▀▀ ▀▀▀ ▀▀▀ ▀ ▀


void processWork(ostime_t doWorkJobTimeStamp)
{
    // This function is called from the doWorkCallback() 
    // callback function when the doWork job is executed.

    // Uses globals: payloadBuffer and LMIC data structure.

    // This is where the main work is performed like
    // reading sensor and GPS data and schedule uplink
    // messages if anything needs to be transmitted.
    
    // Skip processWork if using OTAA and still joining.
    if (LMIC.devaddr != 0)
    {
        // Collect input data.

        ostime_t timestamp = os_getTime();

        #ifdef USE_DISPLAY
            // Interval and Counter values are combined on a single row.
            // This allows to keep the 3rd row empty which makes the
            // information better readable on the small display.
            display.clearLine(INTERVAL_ROW);
            display.setCursor(COL_0, INTERVAL_ROW);
            display.print("I:");
            display.print(doWorkIntervalSeconds);
            display.print("s");        
            display.print(" Ctr:");
        #endif
        #ifdef USE_SERIAL
            printEvent(timestamp, "Input data collected", PrintTarget::Serial);
            printSpaces(serial, MESSAGE_INDENT);
        #endif    

        // For simplicity LMIC-node will try to send an uplink
        // message every time processWork() is executed.

        // Schedule uplink message if possible
        if (LMIC.opmode & OP_TXRXPEND)
        {
            // TxRx is currently pending, do not send.
            #ifdef USE_SERIAL
                printEvent(timestamp, "Uplink not scheduled because TxRx pending", PrintTarget::Serial);
            #endif    
            #ifdef USE_DISPLAY
                printEvent(timestamp, "UL not scheduled", PrintTarget::Display);
            #endif
        }
        else
        {   // Prepare uplink payload.
            uint8_t fPort = 10;
            uint8_t payloadLength = 9;

            // Put 3.3V on pull-up resistors
            digitalWrite(I2CPULLUP,HIGH);
            delay(250);

            MyWatchDoggy.setup(WDT_SOFTCYCLE32S);  // initialize WDT-softcounter refesh cycle on 32sec interval
            Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
            Serial.println(" Watchdog timer initiated");

            // read the temperature from the BME280 and convert in a float between -1 and +1
            float temperature = myBME280.readTempC();
            float newTemp = temperature; // need to keep temperature before it is coded
            Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
            Serial.print("Temperature: "); Serial.print(temperature);
            Serial.print(" *C   ");
            // adjust for the f2sflt16 range (-1 to 1)
            temperature = temperature / 100;
            // float -> int
            // note: this uses the sflt16 datum (https://github.com/mcci-catena/arduino-lmic#sflt16)
            uint16_t codedTemp = LMIC_f2sflt16(temperature);

            // read the humidity from the BME280 and code the value on 1 byte
            float rHumidity = myBME280.readFloatHumidity();
            uint8_t codedHumidity = (uint8_t) round(rHumidity);
            Serial.print(" --- %RH "); Serial.print(rHumidity); Serial.print(" codedHumidity: "); Serial.print(codedHumidity);
            digitalWrite(I2CPULLUP,LOW); // save power by 

            float pressure = myBME280.readFloatPressure()/100;
            uint16_t codedPressure = (uint16_t) pressure - 900;
            Serial.print(" --- Pressure: "); Serial.print(pressure); Serial.print(" Coded pressure: "); Serial.println(codedPressure);

            // take a weight measure
            // float weight = 0;
            // comment the line above and uncomment the block of lines below before adding a load cell
            scale.power_up();
            float weight = scale.get_units(5);

            scale.power_down();

            // below is the temperature compensation code. See explanation in file ardbeescale.docx
            int TextLength = sizeof(Text)/sizeof(Text[0]); // Text is an array (global variable) containing the last 6 external temperature measures
            for( int index = 0; index < TextLength; index = index + 1 ){ // right shift Text array by 1 and update Text[0]
            Text[TextLength-index-1]=Text[TextLength-index-2];
            }
            Text[0] = newTemp; // newTemp should contain the latest temperature measure

            int deltaTextLength = sizeof(deltaText)/sizeof(deltaText[0]); // deltaText is an array (global variable) containing the last 5 temperature changes
            for( int index = 0; index < deltaTextLength; index = index + 1 ){ // right shift deltaText array by 1 and update deltaText[0] 
            deltaText[deltaTextLength-index-1]=deltaText[deltaTextLength-index-2];
            }
            deltaText[0] = newTemp - Text[1];

            float totalDeltaTlc = 0; // will contain the sum of loaf cell temprature changes, after calculation of them here below
            float expFactor = (float) (doWorkIntervalSeconds+8)/60 / (float) TIMECONSTANT; // needed for calculating load cell temperature (4 lines below)
            int deltaTlcLength = sizeof(deltaTlc)/sizeof(deltaTlc[0]);
            for( int index = 0; index < deltaTlcLength; index = index + 1 ){ // recalculate all the load cell temperature changes of the past
            deltaTlc[index] = deltaText[index] * (1-exp(-(expFactor*(index+1)))); // load cell temperature change resulting from external temperature step
            totalDeltaTlc += deltaTlc[index];
            }
            float Tlc = Text[5] + totalDeltaTlc; // now we are getting the real load cell temperature
            
            float wCompensation = 0; // variable initialisation
            if (doWorkIntervalSeconds <= 60 *30) // temperature compensation is implemented only time interval between measurements is less than 1H
            {
                Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
                Serial.println(" Temperature compensation: non linear model is applied");
                wCompensation = (Tlc - T_EXT_INIT) * WEIGHTSLOPE; // and we can therefore calculate the compensation that need to be substracted from the measured weight

            }
            else
            {
                Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
                Serial.println(" Temprature compensation: linear model based on external temperature only");
                wCompensation = (newTemp - T_EXT_INIT) * WEIGHTSLOPE;
            }
            weight -= wCompensation; // this is the temperature compensated weight
            if (weight <0) // weight is transmitted as a non signed integer. This can happen when no load is applied on the scale
            {
                weight=0;
            }

            uint16_t codedWeight = (uint16_t) round(weight/2);
            Serial.print(" --- Weight: "); Serial.print(weight); Serial.print(" Coded weight: "); Serial.print(codedWeight);

            // Battery voltage: get adc reading (pin A7) which is connected to baterry through a divider 100K-100K
            float measuredvbat = analogRead(VBATPIN);
            measuredvbat *= 2; // we divided by 2, so multiply back
            measuredvbat *= 3.3; // Multiply by 3.3V, our reference voltage
            measuredvbat /= 1024; // convert to voltage
            measuredvbat *= 25;  // multiply by 25 and convert to byte
            uint8_t codedVoltage = (uint8_t) measuredvbat; // 
            Serial.print(" --- Voltage: "); Serial.print(measuredvbat); Serial.print(" Coded voltage: "); Serial.print(codedVoltage);

            // Get hive internal temperature (DS18B20 sensor)
            sensors.requestTemperatures(); // Send the command to get temperature readings 
            float internalTemp=sensors.getTempCByIndex(0); // Why "byIndex"?  You can have more than one DS18B20 on the same bus. 0 refers to the first IC on the wire
            float codedInternalTemp_f = floor(10*(internalTemp-15) + 0.5);
            u_int8_t codedInternalTemp = codedInternalTemp_f;
            Serial.print(" --- Internal Temp: "); Serial.print(internalTemp); Serial.print(" Coded internal Temp: "); Serial.println(codedInternalTemp);

            // int -> bytes
            byte weightLow = lowByte(codedWeight);
            byte weightHigh = highByte(codedWeight);
            byte tempLow = lowByte(codedTemp);
            byte tempHigh = highByte(codedTemp);
            // place the bytes into the payload
            payloadBuffer[0] = weightLow;
            payloadBuffer[1] = weightHigh;
            payloadBuffer[2] = tempLow;
            payloadBuffer[3] = tempHigh;
            payloadBuffer[4] = codedHumidity;
            payloadBuffer[5] = codedPressure;
            payloadBuffer[6] = codedVoltage;
            payloadBuffer[7] = lastDownlink;
            payloadBuffer[8] = codedInternalTemp;

            scheduleUplink(fPort, payloadBuffer, payloadLength);

            // disable watchdog
            Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
            Serial.println(" Watchdog timer disabled");
            MyWatchDoggy.setup(0);


            
        }
    }
}    

void processDownlink(ostime_t txCompleteTimestamp, uint8_t fPort, uint8_t* data, uint8_t dataLength)
{
    // This function is called from the onEvent() event handler
    // on EV_TXCOMPLETE when a downlink message was received.

    const uint8_t cmdPort = 100;
    uint32_t interval=doWorkIntervalSeconds;
    uint8_t command = data[0]; 

    if (fPort == cmdPort && dataLength == 1)
    {
        #ifdef USE_SERIAL
            printSpaces(serial, MESSAGE_INDENT);
        #endif
        command >>= 6; // the 2 msb (bits) form the command
        uint8_t value = data[0]; 
        value &=63; // the 6 lsb (bits) form the value
        Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
        serial.print("Downlink received:");
        serial.print(data[0]);
        serial.print(" - command portion: ");
        serial.print(command);
        serial.print(" - Value portion: ");
        serial.println(value);

        if (command == STOPMEASURES)
        {
            stopMeasureTimeMn = 10*value;
            Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
            serial.print("STOPMEASURE command received. Exceptional sleep time of ");
            serial.print(stopMeasureTimeMn);
            serial.println(" minutes, NOW !");
            goToSleepDuration(60*stopMeasureTimeMn);
        } 

        if (command == DOWNLINKRESET)
        {
            lastDownlink = 0;
            Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
            serial.println("last Downlink reset to 0");
        }

        if (command == PUMPTRIGGERCMD)
        {
            uint8_t pumpOnDuration = 10 * value; // pump ON for value seconds (10 times the value transmitted in the downlink, bit 0-5)
            Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
            serial.print("Switching the pump to ON for ");serial.print(pumpOnDuration);serial.println("s ");
            digitalWrite(GPIOPUMP, HIGH);
            delay(1000*pumpOnDuration);
            digitalWrite(GPIOPUMP, LOW);
            Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
            serial.println("Switching the pump OFF");
        }
        
        if (command == FREQCHANGECMD)
        {
            switch (value)
            {
                case 0: // 1mn between two consecutive measures
                    interval = 60;
                    break;
                case 1: //  2mn between two consecutive measures
                    interval = 60 * 2;
                    break;
                case 2: // 5mn between two consecutive measures
                    interval = 60 * 5;
                    break;
                case 3: // 10mn between two consecutive measures
                    interval = 60 * 10;
                    break;
                case 4: //  15mn between two consecutive measures
                    interval = 60 * 15;
                    break;
                case 5: //  30mn between two consecutive measures
                    interval = 60 * 30;
                    break;
                case 6: // 1H between two consecutive measures
                    interval = 60 * 60;
                    break;
                case 7: // 2H between two consecutive measures
                    interval = 60 * 60 * 2;
                    break;
                case 8: //  6H between two consecutive measures
                    interval = 60 * 60 * 6;
                    break;
                case 9: //  12H between two consecutive measures
                    interval = 60 * 60 * 12;
                    break;
                case 10: // 24 Hbetween two consecutive measures
                    interval = 60 * 60 * 24;
                    break;
            }
            Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
            serial.print("Frequency change: Downlink value=");serial.print(value);serial.print(" Measure frequency set to ");serial.print(interval);serial.println("s");
        }
        doWorkIntervalSeconds = interval;
        if (command != DOWNLINKRESET) // in the case a DOWNLINKRESET command has been received we want last downlink to be 0, not the actual last DL received
        {
            lastDownlink = data[0];
        }
        Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
        Serial.print("Last downlink, bayte 0 was ");Serial.println(data[0]);
    }
 if (fPort == cmdPort && dataLength == 3)
 {
     seconds = data[2];
     minutes = data[1];
     hours = data[0];
     rtc.setSeconds(seconds);
     rtc.setMinutes(minutes);
     rtc.setHours(hours);
     Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
     Serial.print("Time set to ");Serial.print(hours);Serial.print("H");Serial.print(minutes);Serial.print("MN");Serial.print(seconds);Serial.println("s");
     lastDownlink = data[1];
 }
}


//  █ █ █▀▀ █▀▀ █▀▄   █▀▀ █▀█ █▀▄ █▀▀   █▀▀ █▀█ █▀▄
//  █ █ ▀▀█ █▀▀ █▀▄   █   █ █ █ █ █▀▀   █▀▀ █ █ █ █
//  ▀▀▀ ▀▀▀ ▀▀▀ ▀ ▀   ▀▀▀ ▀▀▀ ▀▀  ▀▀▀   ▀▀▀ ▀ ▀ ▀▀ 


void setup() 
{
    Serial.print(rtc.getHours());Serial.print(":");Serial.print(rtc.getMinutes());Serial.print(":");Serial.print(rtc.getSeconds());Serial.print(" - ");
    Serial.println("Starting a 10s delay (allowing to load new code before controller goes to sleep");
    delay(5000); 
    // boardInit(InitType::Hardware) must be called at start of setup() before anything else.
    bool hardwareInitSucceeded = boardInit(InitType::Hardware);

    #ifdef USE_DISPLAY 
        initDisplay();
    #endif

    #ifdef USE_SERIAL
        initSerial(MONITOR_SPEED, WAITFOR_SERIAL_S);
    #endif    

    boardInit(InitType::PostInitSerial);

    #if defined(USE_SERIAL) || defined(USE_DISPLAY)
        printHeader();
    #endif

    if (!hardwareInitSucceeded)
    {   
        #ifdef USE_SERIAL
            serial.println(F("Error: hardware init failed."));
            serial.flush();            
        #endif
        #ifdef USE_DISPLAY
            // Following mesage shown only if failure was unrelated to I2C.
            display.setCursor(COL_0, FRMCNTRS_ROW);
            display.print(F("HW init failed"));
        #endif
        abort();
    }

    initLmic();

//  █ █ █▀▀ █▀▀ █▀▄   █▀▀ █▀█ █▀▄ █▀▀   █▀▄ █▀▀ █▀▀ ▀█▀ █▀█
//  █ █ ▀▀█ █▀▀ █▀▄   █   █ █ █ █ █▀▀   █▀▄ █▀▀ █ █  █  █ █
//  ▀▀▀ ▀▀▀ ▀▀▀ ▀ ▀   ▀▀▀ ▀▀▀ ▀▀  ▀▀▀   ▀▀  ▀▀▀ ▀▀▀ ▀▀▀ ▀ ▀

    // Place code for initializing sensors etc. here.
    // scale initialisation

    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.set_scale(LOADCELL_DIVIDER);
    scale.set_offset(LOADCELL_OFFSET); 

    sensors.begin(); // one wire 

    pinMode(GPIOPUMP,OUTPUT);
    pinMode(I2CPULLUP, OUTPUT);

    rtc.begin();
    rtc.setTime(hours, minutes, seconds); // hours, minutes & seconds are global variables
    rtc.setDate(day, month, year); // same

    // Initialise BME280 
    Wire.begin();
    delay(10); // wait at least 2ms before taking any reading
    Serial.println("BME280 I2C address setting");
    myBME280.setI2CAddress(0x76);
    delay(10);
    if (myBME280.beginI2C() == false) //Begin communication over I2C
    {
        Serial.println("The sensor did not respond. Please check wiring.");
        while(1); //Freeze
    }


    
//  █ █ █▀▀ █▀▀ █▀▄   █▀▀ █▀█ █▀▄ █▀▀   █▀▀ █▀█ █▀▄
//  █ █ ▀▀█ █▀▀ █▀▄   █   █ █ █ █ █▀▀   █▀▀ █ █ █ █
//  ▀▀▀ ▀▀▀ ▀▀▀ ▀ ▀   ▀▀▀ ▀▀▀ ▀▀  ▀▀▀   ▀▀▀ ▀ ▀ ▀▀ 

    if (activationMode == ActivationMode::OTAA)
    {
        LMIC_startJoining();
    }

    // Schedule initial doWork job for immediate execution.
    os_setCallback(&doWorkJob, doWorkCallback);
}

void loop() 
{
    os_runloop_once();
}