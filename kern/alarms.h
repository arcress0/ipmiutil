/*
 * alarms.h
 * 
 * Description of what the alarm panel bits mean.  
 * 
 * Note that bit = 0 means on, and bit = 1 means off for these LEDs 
 * (e.g. 0xFF is everything off).
 * The low nibble (bits 0x0F) is for LEDs, and the high nibble (bits 0x30) is
 * for the relays.  The relays are triggered from the LED settings so the high
 * nibble doesn't really matter when setting alarms (usu 0xF* in TAM though).
 *
 * The TAM API is available with the TAM rpm, for your convenience.
 * See the bmcpanic.patch for an example of how to set an LED,
 * at http://panicsel.sourceforge.net
 */

//
// masks used to set LED states
//
#define AP_SET_NO_LED_MASK      0x0F    // bits 0-3 = 1
#define AP_SET_POWER_MASK       0x0E    // bit0 = 0, others = 1
#define AP_SET_CRITICAL_MASK    0x0D    // bit1 = 0
#define AP_SET_MAJOR_MASK       0x0B    // bit2 = 0 
#define AP_SET_MINOR_MASK       0x07    // bit3 = 0

//
// masks used to get LED states
//
#define AP_NO_LED_SET           0x0E
#define AP_POWER_SET            0x01
#define AP_CRITICAL_SET         0x0C
#define AP_MAJOR_SET            0x0A
#define AP_MINOR_SET            0x06

//
// masks used to get relay state
//
#define AP_ALL_RELAY_MASK       0x00
#define AP_MINOR_RELAY_MASK     0x10
#define AP_MAJOR_RELAY_MASK     0x20
#define AP_NO_RELAY_MASK        0x30

/* end alarms.h */
