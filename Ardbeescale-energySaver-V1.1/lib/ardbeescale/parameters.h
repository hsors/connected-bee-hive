#define GPIOGATEWAY 10 // GPIO10 is used to trigger the relay powering the gateway
#define GPIO4GKEY 11   // GPIO11 is used to trigger the relay powering the 4G key
#define VBAT12V A2 // A2 is the analog input on which the 12V supply is connected (through a resistors divider)


// List of possible commands passed to the node
#define TIMEON 0 // when 0 is received in the "command" portion of the downlink, the rest of the downlink will define how many minutes before "round time" the gateway 
                 // and the 4G key must be switched on and how minutes after this "round time" they will stay ON.
#define FREQCHANGECMD 1 // when 1 is received in the "command" portion of the downlink, this is to change the measure frequency
#define DOWNLINKRESET 2 // when 2 is received in the "command" portion of the downlink, this is to reset the downlink value sent after every uplink
#define ALWAYSON 3 // when 3 is received in the "command" portion on the downlink, the rest of the downlink will define which device (gateway or 4G key) has to be permanently
                   // switched ON (even when the energy saver is in sleep mode). Other values stops the "permanent ON " statuts. See documentation

// default values 
#define EARLYON 3      // default value for earlyON variable, defining how much early (versus round time) the processor must wake-up
#define PRONLONGEDON 2 // default value for prolongedON variable, defining how long the device must stay awaken (and the gateways / 4G Key ON) after uplink transmission

// 12V hysteresis values for blocking or unblocking gateway powering
#define VOLTAGE_STOP_ALL 12 // when 12V main battery voltage goes below this value, the gateway will not be power-up anymore until it reaches back a voltage greater or equal to VOLTAGE_RESUME
#define VOLTAGE_RESUME 12.5





