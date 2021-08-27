// COPY this file and name it "SomfyMQTT.config.h" This ensures I never overwrite your config file when updating.
// Version information
#define SomfyMQTT_config_version 1

//=========================================================================================================
// Wifi settings
#define WiFissid "----"         		// your network SSID (name)
#define WiFipassword "--------" 		// your network password

// Somfy remote / cc1101 settings
#define Somfy_Remote 0x5184c8			// The ID you choose for this Somfy remote
#define Somfy_LongPressTime 2500		// The time a longpress will repeat a button (in ms)
#define EMITTER_GPIO 5					// The ESP pin to select the CC1101, D2 is GPIO5 on a ESP D1 mini, hence pin 5.
#define CC1101_FREQUENCY 433.42			// Not the regular 433 band frequency, but this is what Somfy uses.


// The characteristics of your rollo, edit this to sync the speed of the position calculation with the actual movement
#define Somfy_Time2MoveFullyUp	 43.1	// Number of seconds for moving the sunshade "Up"   (position   0)
#define Somfy_Time2MoveFullyDown 44.3	// Number of seconds for moving the sunshade "Down" (position 100)

// MQTT server to use
#define mqtt_server "xxx.yyy.zzz.nnn"	// The IP address (or name) of your MQTT server
#define mqtt_port 1883					// The port for accessing your MQTT server, change it if you use portforwarding
#define mqtt_user "mqttuser"			// The user for accessing mqtt topics on your server"		
#define mqtt_pass "mqttpass"			// The password for accessing your MQTT server

// Topics to be used========================================================================================
#define mqtt_ESPstate 			"test/scherm/ESP"			// Status messages and debug info (when enabled)		
#define mqtt_RolloTargetTopic	"test/scherm/target"		// (WRITE) The position to go to
#define mqtt_RolloPositionTopic	"test/scherm/position"		// (READ) Actual position
#define mqtt_RolloButtonTopic	"test/scherm/button"		// (WRITE) press button Up,Down,My,Prog,LongProg (U,D,M,P,L)
#define mqtt_PublishInterval 	60000    					// Interval (ms) for updating status messages
#define mqtt_PublishPositionInterval 1000   				// Interval (ms) for updating the position
                 
//=========================================================================================================
// Note that your platformio OTA upload (or arduino IDE OTA upgrade), must specify (or type in when prompted)
// the same password the NEXT time you upload an image.
//
//Set the password, for future Over The Air (WiFi) updates here.
// If the define is not set, no OTA password is required.
#define OTApass "1234"

// A hostname helps identifying the plug, not all operating systems will see it configured in this manner, 
// it might need the "bonjour" service installed.
#define WiFihostname "SomfyMQTT"

// Set this debug define to write information to the serial port, will NOT be visible over Wifi, only to the serial port
// When set, debug lines are also written to the [mqtt_ESPstate] topic.
#define debug_

//The Led behavior (for flashing/state LED), when not defined, no LED will be used.
//#define PinLED 16

