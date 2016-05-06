#include "MQTT/MQTT.h"

// This #include statement was automatically added by the Particle IDE.
#include "Adafruit_DHT/Adafruit_DHT.h"

// This #include statement was automatically added by the Particle IDE.
#include "SparkJson/SparkJson.h"

#define WATER_SENSOR 0
#define BUZZER 3
#define DHTPIN 2

//What type of temp/hum sensor are we using?
#define DHTTYPE DHT11		// DHT 22 (AM2302)

//define your mqtt credentials
#define DEVICE_ID "814ebb96-320d-4a93-93c1-6b21c9afe801"
#define MQTT_USER "814ebb96-320d-4a93-93c1-6b21c9afe801"
#define MQTT_PASSWORD "8htaGLrOHcvG"
#define MQTT_CLIENTID "TgU67ljINSpOTwWshya/oAQ" //can be anything else
#define MQTT_TOPIC "/v1/814ebb96-320d-4a93-93c1-6b21c9afe801/"
#define MQTT_SERVER "mqtt.relayr.io"
//Some definitions, including the publishing period
//Some definitions, including the publishing period
const int led = D7;
int ledState = LOW;
unsigned long lastPublishTime = 0;
unsigned long lastBlinkTime = 0;
int publishingPeriod = 400;     //Value given in ms

//The values of humidity and temperature will be stored here
float h;
float t;

//And this is for the temperature and humidity sensor
DHT dht(DHTPIN, DHTTYPE);

//Initializing some stuff...
void setup()
{
	pins_init();
	
    RGB.control(true);
    Serial.begin(9600);
    Serial.println("Hello there, I'm your Photon!");
    //Setup our LED pin
    pinMode(led, OUTPUT);
    //200ms is the minimum publishing period
    publishingPeriod = publishingPeriod > 200 ? publishingPeriod : 200;
    //This initializes the sensor
    dht.begin();
    mqtt_connect();
}

//This one is called up there; it initializes some pins
void pins_init()
{
	pinMode(WATER_SENSOR, INPUT);
	pinMode(BUZZER, OUTPUT);
}

//This function makes the buzzer beep shortly
void soundAlarm()
{
	for(uint8_t i = 0;i < 4;i ++)
	{
		digitalWrite(BUZZER, HIGH);
		delay(50);
		digitalWrite(BUZZER, LOW);
		delay(50);
	}
}

//This function checks if there's water in the sensor
boolean isExposedToWater()
{
	if(digitalRead(WATER_SENSOR) == LOW)
		return true;
            		    
	else return false;
}

//This is the callback function, necessary for the MQTT communication
void callback(char* topic, byte* payload, unsigned int length);
//Create our instance of MQTT object
MQTT client(MQTT_SERVER, 1883, callback);
//Implement our callback method that's called on receiving data from a subscribed topic
void callback(char* topic, byte* payload, unsigned int length) {
  //Store the received payload and convert it to string
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  //Print the topic and the received payload
  Serial.println("topic: " + String(topic));
  Serial.println("payload: " + String(p));
  //Call our method to parse and use the received payload
  handlePayload(p);
}

void handlePayload(char* payload) {
  StaticJsonBuffer<200> jsonBuffer;
  //Convert payload to json
  JsonObject& json = jsonBuffer.parseObject(payload);
  if (!json.success()) {
    Serial.println("json parsing failed");
    return;
  }
  //Get the value of the key "command", aka. listen to incoming commands
  const char* command = json["command"];
  Serial.println("parsed command: " + String(command));
  
  //We can send commands to change the color of the RGB
  if (String(command).equals("color"))
  {
    const char* color = json["value"];
    Serial.println("parsed color: " + String(color));
    String s(color);
    if (s.equals("red")){
      RGB.color(255, 0, 0);
    }
    else if (s.equals("blue"))
      RGB.color(0, 0, 255);
    else if (s.equals("green"))
      RGB.color(0, 255, 0);
    else if (s.equals("orange"))
      RGB.color(255, 70, 0);
    else if (s.equals("yellow"))
      RGB.color(255, 150, 0);
  }
}

//This function establishes the connection with the MQTT server
void mqtt_connect() {
  Serial.println("Connecting to MQTT server...");
  if (client.connect(MQTT_CLIENTID, MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("Connection successful! Subscribing to topic...");
    //This one subscribes to the topic "cmd", so we can listen to commands
    client.subscribe("/v1/"DEVICE_ID"/cmd");
  }
  else {
    Serial.println("Connection failed! Check your credentials or WiFi network");
  }
}

//This is for the LED to blink
void blink(int interval) {
  if (millis() - lastBlinkTime > interval) {
    //Save the last time you blinked the LED
    lastBlinkTime = millis();
    if (ledState == LOW)
      ledState = HIGH;
    else
      ledState = LOW;
    //Set the LED with the ledState of the variable:
    digitalWrite(led, ledState);
  }
}

//This is the main loop, it's repeated until the end of time :)
void loop()
{
	
	//If we're connected, we can send data...
	if (client.isConnected()) {

    client.loop();
    //Publish within the defined publishing period
        if (millis() - lastPublishTime > publishingPeriod) {
            
            lastPublishTime = millis();
            
            //Finally publishing...
            publish();
            
            //Is there water in the sensor?
            if(isExposedToWater()){
             	
             	soundAlarm();   
            }
            
        }
            
        //Blink LED  
        blink(publishingPeriod / 2);
    }
    else {
    //If connection is lost, then reconnect
        Serial.println("Retrying...");
        mqtt_connect();
    }
        
    // Wait a few seconds between measurements
	delay(2000);
        
    // Reading temperature or humidity takes about 250 ms!
    // Sensor readings may also be up to 2s 'old' (it's a very slow sensor!)
	h = dht.getHumidity();
	Serial.print(h);
    // Read temperature as Celsius
	t = dht.getTempCelcius();
	Serial.print(t);
    // Read temperature as Farenheit
	float f = dht.getTempFarenheit();
        
    // Check if any reads failed and exit early (to try again)
	if (isnan(h) || isnan(t) || isnan(f)) {
		Serial.println("Failed to read from DHT sensor!");
		return;
	}
        
    // Compute heat index
    // Must send in temp in Fahrenheit!
	float hi = dht.getHeatIndex();
	float dp = dht.getDewPoint();
	float k = dht.getTempKelvin();
        
}

//Publish function; here we add what we want to send to the cloud
void publish() {
  //Create our JsonArray
  StaticJsonBuffer<300> pubJsonBuffer;
  JsonArray& root = pubJsonBuffer.createArray();

//-------------------------------------------------
//   //First object: Water sensor
//   JsonObject& leaf1 = root.createNestedObject();
//   //This is how we name what we are sending
//   leaf1["meaning"] = "w";
//   //This contains the readings of the sensor
//   leaf1["value"] = isExposedToWater();
//-------------------------------------------------
  
//-------------------------------------------------  
  //Second object: Humidity sensor
  JsonObject& leaf2 = root.createNestedObject();
  //This is how we name what we are sending
  leaf2["meaning"] = "h";
  //This contains the readings of the sensor
  leaf2["value"] = h;
//-------------------------------------------------
  
//-------------------------------------------------  
  //Third object: Temperature sensor
  JsonObject& leaf3 = root.createNestedObject();
  //This is how we name what we are sending
  leaf3["meaning"] = "t";
  //This contains the readings of the sensor
  leaf3["value"] = t;
//-------------------------------------------------

  char message_buff[128];
  root.printTo(message_buff, sizeof(message_buff));
  client.publish("/v1/"DEVICE_ID"/data", message_buff);
  Serial.println("Publishing " + String(message_buff));
}