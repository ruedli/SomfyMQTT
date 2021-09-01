/*  SomfyMQTT      
 */

#define Version_major 2
#define Version_minor 1
 /*
 *  v1.0 - 25 aug 2021
 *    Initial release 
 *
 *  v1.1 - 27 aug 2021
 *    Packaged and Published on github
 *
 *  v1.2 - 28 aug 2021
 *    Buttons released when movement stops
 *
 *  v1.3 - 31 aug 2021
 *    small fix: setting the range and moving in small steps now works better. 
 *	  Moving up would sometime step one step too many.
 *
 *    3D printing files added
 *
 /
 
 /* Libraries used:
 *
 *	SmartRC-CC1101-Driver-Lib-master	https://github.com/LSatan/SmartRC-CC1101-Driver-Lib.git
 *	Somfy_Remote_Lib-0.4.1				https://github.com/Legion2/Somfy_Remote_Lib.git
 *	pubsubclient						https://github.com/knolleary/pubsubclient.git
 *	esp8266								https://github.com/esp8266/Arduino.git
 *	 
 */
// Include configuration
#include "SomfyMQTT.config.h"

#ifndef SomfyMQTT_config_version
#error The version of the config file cannot be determined, ensure you copy SomfyMQTT.config.h from the latest SomfyMQTT.config.h.RELEASE
#endif

#if SomfyMQTT_config_version != Version_major
#error "There are new configuration parameters determined, ensure you copy SomfyMQTT.config.h from the latest SomfyMQTT.config.h.RELEASE"
#endif
 
 // Includes for the CC1101
#include <EEPROM.h>
#include <EEPROMRollingCodeStorage.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <SomfyRemote.h>

#define EEPROM_ADDRESS 0

#include <ESP8266WiFi.h>

//Include for OTA updates
#include <ArduinoOTA.h>

// Includes for MQTT
#include <PubSubClient.h>

EEPROMRollingCodeStorage rollingCodeStorage(EEPROM_ADDRESS);
SomfyRemote somfyRemote(EMITTER_GPIO, Somfy_Remote, &rollingCodeStorage);

WiFiClient espClient;
PubSubClient client(espClient);
#define MSG_BUFFER_SIZE	(80)

enum PosState {
  Positioned,
  MovingUp,
  MovingDown,
  PositioningUp,
  PositioningDown,
  Unknown
};

enum button {
	None,
	Up,
	Down,
	My,
	GotoTarget,
	Prog,
	LProg
};
	
char msg[MSG_BUFFER_SIZE];
int counter=0;		//For counting the keep-alive-messages

// Timer stuff
unsigned long lastMsg = 0;
unsigned long TimerLED = 0;
unsigned long TimePositioned = 0;
unsigned long TimePositionReported = 0;

class SomfyCommand { 
	public:
		Command Somfy;
		char Topic[80];
		char Target[1];

		
	
		SomfyCommand(char *sCommand, char *sLocation, char *sDevice, char *sSubTopic) {
			Somfy = getSomfyCommand(sCommand);
			strcpy(Topic,sLocation);
			strcat(Topic,"/");
			strcat(Topic,sDevice);
			strcat(Topic,"/");
			strcat(Topic,sSubTopic);
			
			strncpy(Target,sCommand,1);
			Target[1]='\0';
				
		}			//constructor
		
	
	void SendRF() {
		#ifdef debug_
			Serial.print("Sending RF ");
			Serial.println(Target);
		#endif
		ELECHOUSE_cc1101.SetTx();
		somfyRemote.sendCommand(Somfy);
		ELECHOUSE_cc1101.setSidle();
	}
		
	void Publish() {
		client.publish(Topic,Target);
	}
		
	void Reset() {
		client.publish(Topic,"-");
		
	}
	
};

// The state of the info LED
#ifdef PinLED
int LED_count;
bool LED_on = false;
#endif

PosState State = Unknown;
PosState LastState = Unknown;
button BPressed = None;

//Somfy commands
SomfyCommand SomfyUp("Up",mqtt_Location,mqtt_Device,mqtt_ButtonTopic);
SomfyCommand SomfyDown("Down",mqtt_Location,mqtt_Device,mqtt_ButtonTopic);
SomfyCommand SomfyMy("My",mqtt_Location,mqtt_Device,mqtt_ButtonTopic);
SomfyCommand SomfyProg("Prog",mqtt_Location,mqtt_Device,mqtt_ButtonTopic);

//Positioning stuff
int Position = 0;
int Target = 0;
int StartPos = 0;
int LastPositionReported = 0;	//ensures the position is initialized FROM mqtt.

//	These allow the bridge to publish a button position to reflect a GUI without operating the sunshade
bool IgnoreOneDown = false;
bool IgnoreOneUp = false;
bool IgnoreOneMy = true;		// No "stop" beforde movement.
bool IgnoreOneTarget = true; 	// Avoid problems during initialization of position, when target arrives before position.

bool TryToInitializePosition = true; //Try to subscribe and get the position once.

#ifdef debug_
void logger(String log) {
	Serial.println(log);
	client.publish(mqtt_ESPstate,log.c_str());
}
#endif

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi networkhttps://www.w3schools.com/cpp/cpp_function_overloading.asp
  #ifdef debug_
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WiFissid);
  #endif

  // Update these with values suitable for your network (in the SomfyMQTT.config.h file).
  WiFi.mode(WIFI_STA);
  WiFi.begin(WiFissid, WiFipassword);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
	#ifdef debug_
    Serial.print(".");
	#endif
  }

  //OTA init: Set the hostname
  ArduinoOTA.setHostname(WiFihostname);
  #ifdef OTApass
  ArduinoOTA.setPassword(OTApass);
  #endif

  randomSeed(micros());

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
	
	#ifdef PinLED
	// switch off all the PWMs during upgrade
	analogWrite(PinLED, 0);
	LED_count = 0;
	LED_on = false;
	digitalWrite(PinLED, LED_on ? LOW: HIGH);  // The LED is off...
	#endif
	
  });
  
  ArduinoOTA.onEnd([]() {
	Serial.println("\nEnd");
	#ifdef PinLED
	LED_on = false;
	digitalWrite(PinLED, LED_on ? LOW: HIGH);  // The LED is off...
	#endif
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	#ifdef PinLED
		if ((LED_count++ % 2) == 0) LED_on = not LED_on;
		digitalWrite(PinLED, LED_on ? LOW: HIGH);  // The LED is blinks during uploading...
	#endif
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
	Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();  

//Initial state and timers for OctoPrintPlugout

	#ifdef PinLED
	// do a fancy thing with our board led when starting up
	for (int i = 0; i < 30; i++) {
	  analogWrite(PinLED, (i * 100) % 1001);
	  delay(50);
	}
	digitalWrite(PinLED, HIGH);  // The LED is off...

	// Indicate the major and minor version
	const int delays[] = { Version_major , Version_minor };
	for (int i = 0; i <= 1; i++) {
		delay(500);
		for (int j = 1; j <= delays[i]; j++) {
			delay(300);
			digitalWrite(PinLED, LOW);  // The LED is on...
			delay(50);
			digitalWrite(PinLED, HIGH);  // The LED is off...
		}
	}
	delay(2000);
	#endif

	// This is the initial state
	//State = InitialState;
	#ifdef debug_
	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
	#endif
}

void callback(char* topic, byte* payload, unsigned int length) {
	#ifdef debug_
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (int i = 0; i < length; i++) {
	Serial.print((char)payload[i]);
	}
	Serial.println();
	#endif

	// Set BPressed folowing the mesage received.
	if (strcmp(topic, mqtt_RolloButtonTopic) == 0) {
		if ((char)payload[0] == 'U') {
				if (IgnoreOneUp) {
				IgnoreOneUp=false;
			} else {
				BPressed = Up;
			}				
		} else if ((char)payload[0] == 'D'){
			if (IgnoreOneDown) {
				IgnoreOneDown=false;
			} else {
				BPressed = Down;
			}				
		} else if ((char)payload[0] == 'M'){
			if (IgnoreOneMy) {
				// do nothing
			} else {
				BPressed = My;
			}
		} else if ((char)payload[0] == 'P'){
			BPressed = Prog; 
		} else if ((char)payload[0] == 'L'){
			BPressed = LProg; 
		}
	} else if (strcmp(topic, mqtt_RolloTargetTopic) == 0) {
		if (IgnoreOneTarget) {
			IgnoreOneTarget = false;
		} else {
			payload[sizeof(payload)] = '\0'; // NULL terminate the array
			if ((char)payload[0] != '-') {
				Target = atoi((char *)payload);
				if (Target != Position) {
					BPressed = GotoTarget;
					#ifdef debug_
					snprintf (msg, MSG_BUFFER_SIZE, "Target obtained: %1d \n",Target);
					logger(msg); 
					#endif
				}
			}
		}

	} else if (strcmp(topic, mqtt_RolloPositionTopic) == 0) {
		if (TryToInitializePosition) {
			payload[sizeof(payload)] = '\0'; // NULL terminate the array
			Position = atoi((char *)payload);
			StartPos = Position;
			Target = Position;
			State = Positioned;
			IgnoreOneTarget = false; //Now it is safe to receive the target.
			#ifdef debug_
			snprintf (msg, MSG_BUFFER_SIZE, "Position initialized: %1d \n",Position);
			logger(msg);			
			#endif
		}			
		TryToInitializePosition = false;
		client.unsubscribe(mqtt_RolloPositionTopic);	
	}
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    #ifdef debug_
	Serial.print("Attempting MQTT connection...");
	#endif
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),mqtt_user,mqtt_pass)) {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe(mqtt_RolloTargetTopic);
	  client.subscribe(mqtt_RolloButtonTopic);
	  if (TryToInitializePosition) client.subscribe(mqtt_RolloPositionTopic);
    } else {
	  #ifdef debug_
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
	  #endif
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



void setup() {
	#ifdef PinLED
	pinMode(PinLED, OUTPUT);
	digitalWrite(PinLED, HIGH);  // The LED is off...
	#endif
	Serial.begin(115200);
	delay(1000);
	Serial.flush();

	somfyRemote.setup();

	ELECHOUSE_cc1101.Init();
	ELECHOUSE_cc1101.setMHZ(CC1101_FREQUENCY);

#if defined(ESP32)
	if (!EEPROM.begin(4)) {
		#ifdef debug_
		Serial.println("failed to initialise EEPROM");
		#endif
		delay(1000);
	}
#elif defined(ESP8266)
	EEPROM.begin(4);
#endif

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  

}

/*
void sendSomfyCc1101Command(char *command) {
	const Command Scommand = getSomfyCommand(command);
	ELECHOUSE_cc1101.SetTx();
	somfyRemote.sendCommand(Scommand);
	ELECHOUSE_cc1101.setSidle();
}
*/

void loop() {
	ArduinoOTA.handle();
	if (!client.connected()) {
		reconnect();
	}
	
	client.loop();

	unsigned long now = millis();
	
/* Here is all movement and positioning calculated, it is a state machine going through states, issuing remote commands and updating the mqtt topics
*/

//	React to buttons pressed
	if ((BPressed==Up) or ((BPressed==GotoTarget) and (Target==0))){
		SomfyUp.SendRF();
		TimePositioned = now;
		StartPos=Position;
		
		IgnoreOneMy=false;	// Stopping is allowed now. 
		
		// Reflect the button pressed
		if (BPressed==GotoTarget) {
			IgnoreOneUp = true;
			SomfyUp.Publish();
		}

		BPressed=None;
		State = MovingUp;
		
		#ifdef debug_
		logger("Up event scheduled");
		#endif

	} else if ((BPressed==Down) or ((BPressed==GotoTarget) and (Target==100))) {
		SomfyDown.SendRF();
		TimePositioned = now;		
		StartPos=Position;

		IgnoreOneMy=false;	// Stopping is allowed now. 
		
		// Reflect the button pressed
		if (BPressed==GotoTarget) {
			IgnoreOneDown = true;
			SomfyDown.Publish();
		}
		
		BPressed=None;
		State = MovingDown;
		
		#ifdef debug_
		logger("Down event scheduled");
		#endif
		
	} else if (BPressed==My) {
		SomfyMy.SendRF();
		IgnoreOneMy=true;	// Stopping is NOT allowed now. 
		
		#ifdef debug_
		logger("My event scheduled");
		#endif
		
		SomfyMy.Reset();
	} else if (BPressed==Prog) {
		SomfyProg.SendRF();
		BPressed=None;
		#ifdef debug_		
		logger("Prog event sent");
		#endif
	} else if (BPressed==LProg) {
		while ((millis()-now) < Somfy_LongPressTime) {
			SomfyProg.SendRF();
		}
		BPressed=None;
		#ifdef debug_		
		logger("Long Prog event sent");
		#endif
	} else if (BPressed==GotoTarget) {
		BPressed=None;
		TimePositioned = now;
		StartPos=Position;
		if (Position>Target) {
			SomfyUp.SendRF();
			State = PositioningUp;
			StartPos=Position;
			
			IgnoreOneUp = true;
			SomfyUp.Publish();
			IgnoreOneMy=false;	// Stopping is allowed now. 
			
			#ifdef debug_
			snprintf (msg, MSG_BUFFER_SIZE, "Position UP from %1d to %2d \n",StartPos,Target);
			logger(msg);
			#endif
			
		} else if (Position<Target) {
			SomfyDown.SendRF();	
			State = PositioningDown;
			StartPos=Position;	

			IgnoreOneDown = true;
			SomfyDown.Publish();
			IgnoreOneMy=false;	// Stopping is allowed now. 	
			
			#ifdef debug_
			snprintf (msg, MSG_BUFFER_SIZE, "Position DOWN from %1d to %2d \n",StartPos,Target);
			logger(msg);
			#endif
		}
	}


//	position calculation
	if ((State == MovingUp) or (State == PositioningUp)) Position = StartPos - 1 - 100.0 * (now-TimePositioned) / (1000.0 * Somfy_Time2MoveFullyUp);
	
	if ((State == MovingDown) or (State == PositioningDown)) Position = StartPos + 100.0 * (now-TimePositioned) / (1000.0 * Somfy_Time2MoveFullyDown);	

/*
	#ifdef debug_
	if ((Position != LastPos)) {
		Serial.print("Calculate position, Position: ");
		Serial.print(Position);
		Serial.print(" State: ");
		Serial.println(ShowState(State));

		LastPos = Position;
	}
	#endif
*/
	
//	Fix out-of-bound positions
	if (Position < 0) {
		Position = 0;
		State = Positioned;
	} else if (Position > 100) {
		Position = 100;
		State = Positioned;
	} 

	if (BPressed==My) {
		BPressed=None;
		State = Positioned;
	}
		
	if (((State == PositioningDown) and (Position>=Target)) or ((State == PositioningUp) and (Position<=Target))) {
		SomfyMy.SendRF();

		#ifdef debug_
		if (State == PositioningDown) snprintf (msg, MSG_BUFFER_SIZE, "Stopped moving down at Position: %1d, Target %2d \n",Position,Target);
		if (State == PositioningUp)   snprintf (msg, MSG_BUFFER_SIZE, "Stopped moving up at Position: %1d, Target %2d \n",Position,Target);
		logger(msg);
		#endif
		State = Positioned;
	}		

// Report the position through mqtt
	if (now-TimePositionReported > mqtt_PublishPositionInterval) {
		TimePositionReported = now;
		if (Position!=LastPositionReported) {
			LastPositionReported=Position;
			#ifdef debug_
			snprintf (msg, MSG_BUFFER_SIZE, "Position is %ld",Position);
			logger(msg);
			#endif			
			snprintf (msg, MSG_BUFFER_SIZE, "%ld",Position);
			client.publish(mqtt_RolloPositionTopic, msg, true);
		}
	}
	
	if (LastState!=State) {
		LastState = State;
		if (State==Positioned) {
			SomfyMy.Reset();;
			if (!IgnoreOneTarget) {
				//Set target on server, without acting
				IgnoreOneTarget	= true;
				snprintf (msg, MSG_BUFFER_SIZE, "%ld",Position);
				client.publish(mqtt_RolloTargetTopic, msg, true);
			}
		}

	}		

//	Cosmetic stuff, keep-alive, LED blinking etc.
	if (now - lastMsg > mqtt_PublishInterval) {
		lastMsg = now;
		snprintf (msg, MSG_BUFFER_SIZE, "%s v%ld.%ld #%ld P%ld",WiFihostname,Version_major,Version_minor, ++counter,Position);
		#ifdef debug_
		Serial.print("Publish message: ");
		Serial.println(msg);
		#endif
		client.publish(mqtt_ESPstate, msg);
		#ifdef PinLED
			TimerLED = now;
			LED_on = true;
		#endif
	}
	
	#ifdef PinLED
	if (now - TimerLED > 100) {
		LED_on = false;
	}
	
	digitalWrite(PinLED, LED_on ? LOW: HIGH);  // The LED is blinks once during publishing...
	#endif
}
