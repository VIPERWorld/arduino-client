#include <Ethernet.h>			//for loading components required by the iot device object.
#include <PubSubClient.h>

#include <iot_att_min.h>
#include <SPI.h>                //required to have support for signed/unsigned long type.

/*
  AllThingsTalk Makers Arduino Example 

  ### Instructions

  1. Setup the Arduino hardware
    - USB2Serial
    - Grove kit shield
    - Potentiometer to A0
    - Led light to D8
  2. Add 'iot_att_min' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. fill in the missing strings (deviceId, clientId, mac) and optionally change/add the sensor & actuator ids
     For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Upload the sketch

  ### Troubleshooting

  1. 'Device' type is reported to be missing. 
  - Make sure to properly add the 'iot_att' library

*/

char deviceId[] = ""; // Your device id comes here
char clientId[] = ""; // Your client id comes here";

ATTDevice Device(deviceId, clientId);            //create the object that provides the connection to the cloud to manager the device.

byte mqttServer[] = { 188, 64, 53, 92 };                   

byte mac[] = {  0x90, 0xA2, 0xDA, 0x0D, 0xE1, 0x3E }; 	    // Adapt to your Arduino MAC Address  

String sensorId = "1";										// uniquely identify this asset. Don't use spaces in the id.
String actuatorId = "2";									// uniquely identify this asset. Don't use spaces in the id.

int ValueIn = 0;                                            // Analog 0 is the input pin
unsigned int prevVal = 0;                                   //so we only send the value if it was different from prev value.	
int ledPin = 8;                                             // Pin 8 is the LED output pin 

//required for the device
void callback(char* topic, byte* payload, unsigned int length);
EthernetClient ethClient;
PubSubClient pubSub(mqttServer, 1883, callback, ethClient);

void setup()
{
  pinMode(ledPin, OUTPUT);                              // initialize the digital pin as an output.         
  Serial.begin(9600);                                   // init serial link for debugging
  
  if (Ethernet.begin(mac) == 0)                         // Initialize the Ethernet connection: for pub-sub client
    Serial.println(F("DHCP failed,end"));
  delay(1000);							                // give the Ethernet shield a second to initialize:
  
  Device.Subscribe(pubSub);						        // make certain that we can receive message from the iot platform (activate mqtt)
  Serial.println("init done");
}

void loop()
{
  unsigned int lightRead = analogRead(ValueIn);			        // read from light sensor (photocell)
  if(lightRead != prevVal)
  {
    Device.Send(String(lightRead), sensorId);
	prevVal = lightRead;
  }
  Device.Process(); 
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(char* topic, byte* payload, unsigned int length) 
{ 
  String msgString; 
  {	                                                    //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
	char message_buff[length + 1];						//need to copy over the payload so that we can add a /0 terminator, this can then be wrapped inside a string for easy manipulation.
	int i = 0;
	for(; i < length; i++) 							//create character buffer with ending null terminator (string)
	  message_buff[i] = payload[i];
	message_buff[i] = '\0';							//make certain that it ends with a null			
		  
	msgString = String(message_buff);
	msgString.toLowerCase();							//to make certain that our comparison later on works ok (it could be that a 'True' or 'False' was sent)
  }
  String* idOut = NULL;
  {	                                                    //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
	String topicStr = topic;							//we convert the topic to a string so we can easily work with it (use 'endsWith')
	
	Serial.print("Payload: ");			                //show some debugging.
	Serial.println(msgString);
	Serial.print("topic: ");
	Serial.println(topicStr);
	
	if (topicStr.endsWith(actuatorId)) 				                //warning: the topic will always be lowercase. This allows us to work with multiple actuators: the name of the actuator to use is at the end of the topic.
	{
	  if (msgString == "false") {
		digitalWrite(ledPin, LOW);					        //change the led	
		idOut = &actuatorId;		                        
	  }
	  else if (msgString == "true") {
		digitalWrite(ledPin, HIGH);
		idOut = &actuatorId;
	  }
	}
  }
  if(idOut != NULL)                //also let the iot platform know that the operation was succesful: give it some feedback. This also allows the iot to update the GUI's correctly & run scenarios.
    Device.Send(msgString, *idOut);    
}