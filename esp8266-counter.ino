#include <PubSubClient.h>
#include <ESP8266WiFi.h>

const int gpio = D0;                     // the pin we read from
const char mqtt_server[] = SECRET_HOST;  // dns (not mdns) or ip of mqtt host
const int mqtt_port = 1883;              // port, default 1883

// Variables will change:
int state = HIGH;    // the current reading from the input pin
int lastState = LOW; // the previous reading from the input pin
int counter = 0;     // the counter that will be increased after each trigger

String deviceId; // the deviceId is derived from the hostname

// the following variable are used for mqtt transport
IPAddress mqtt_address;        // the ip address of mqtt_server
char mqtt_message_payload[32]; // the mqtt message payload
char mqtt_topic[32];           // the mqtt topic that will be used, we derivate the topic
                               // from mac adresse and gpio

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
unsigned long debounceDelay = 100;  // the debounce time; increase if the output flickers
unsigned long deadTime = 500;       // deadTime after we rise to high

WiFiClient espClient;
PubSubClient client(espClient);

void setup()
{
  pinMode(gpio, INPUT);

  // initialize serial:
  Serial.begin(115200);

  // ssid and password were previously set by another sketch
  // attempt to connect using WPA2 encryption:
  Serial.println("Attempting to connect to WPA network...");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(50);
    Serial.print(".");
  }

  deviceId = WiFi.hostname();
  deviceId.toLowerCase();

  String topic = deviceId + "/" + gpio;
  topic.toLowerCase();
  topic.toCharArray(mqtt_topic, 32);

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address:   ");
  Serial.println(WiFi.localIP());
  Serial.print("DNS address:  ");
  Serial.println(WiFi.dnsIP());
  Serial.print("Device id:    ");
  Serial.println(deviceId);

  // resolve mqtt address
  WiFi.hostByName(mqtt_server, mqtt_address);

  Serial.println("");
  Serial.println("MQTT");
  Serial.print("address:      ");
  Serial.print(mqtt_server);
  Serial.print(" (");
  Serial.print(mqtt_address);
  Serial.println(")");
  Serial.print("Port:         ");
  Serial.println(mqtt_port);
  Serial.print("Topic:        ");
  Serial.println(mqtt_topic);

  // configure  mqtt client
  client.setServer(mqtt_address, mqtt_port);

  Serial.println();
  Serial.println("Setup completed");
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    // TODO maybe we should use our hostname
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 1 seconds");

      // Wait a seconds before retrying
      delay(1000);
    }
  }
}

void loop()
{
  // read the state of the switch into a local variable:
  int reading = digitalRead(gpio);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastState)
  {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != state)
    {
      state = reading;

      // only toggle the LED if the new button state is HIGH
      if (state == HIGH)
      {
        counter++;

        Serial.print("> ");
        Serial.print(mqtt_topic);
        Serial.print(" ");
        Serial.println(counter);

        if (!client.connected())
        {
          reconnect();
        }

        snprintf(mqtt_message_payload, 32, "%d", counter);
        client.publish(mqtt_topic, mqtt_message_payload);

        long restDeadTime = millis() - lastDebounceTime - deadTime;
        if (restDeadTime > 0)
        {
          Serial.print("deadTime :");
          Serial.println(restDeadTime);
          delay(restDeadTime);
        }
      }
    }
  }

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastState = reading;
}
