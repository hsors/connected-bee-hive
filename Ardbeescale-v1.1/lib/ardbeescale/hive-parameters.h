
#define VBATPIN A7 // pin A7 is connected to baterry through a divider 100K-100K. Used to measure battery voltage
#define GPIOPUMP 10 // pin 10 is connected to the pump relay
#define I2CPULLUP 5 // pin 5 is used to pull-up the I2C bus through a resistor
#define PUMPTRIGGERCMD 0 // when 0 is received in the "command" portion of the downlink, this is to trigger the pump
#define FREQCHANGECMD 1 // when 1 is received in the "command" portion of the downlink, this is to change the measure frequency
#define DOWNLINKRESET 2 // when 2 is received in the "command" portion of the downlink, this is to reset the downlink sent after every uplink
#define STOPMEASURES 3 // // when 3 is received in the "command" portion of the downlink, this is to stop measures during a given time (coded in the rest of the downlink)

// HX711 circuit wiring
#define LOADCELL_DOUT_PIN 11
# define LOADCELL_SCK_PIN 12

// ONE wire pin
#define ONE_WIRE_BUS 5

// scale parameters
#define LOADCELL_OFFSET 166450 // value deducted from the raw reading (which is also the raw reading when no load is applied to the scale)
#define LOADCELL_DIVIDER 44  // raw reading minus offset divided by this number will give the weight applied on the scale in grams
#define WEIGHTSLOPE -0.15 // this is the value (in grams) by which the weight measurement varies when the LC temperature increases by 1 degree C
#define TIMECONSTANT 17 // this is the time constant of the LC mounted in its fraMe (see explanation is file ardbeescale.pdf, chapter "temperature compensation")
#define T_EXT_INIT 23 // this is the external temperature at which the scale calibration was done (stabilized temperature, supposed to be also the one of the load cell)


