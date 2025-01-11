#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Fingerprint.h>
#include <ArduinoJson.h>

// WiFi Settings
#define SSID                          "xxxxxxxxxxx"
#define PASSWORD                      "xxxxxxxxxxx"

// MQTT Settings
#define HOSTNAME                      "fingerprint-sensor"
#define MQTT_SERVER                   "wasinstupid1.local"
#define STATE_TOPIC                   "/fingerprint/mode/status"
#define MODE_LEARNING                 "/fingerprint/mode/learning"
#define MODE_READING                  "/fingerprint/mode/reading"
#define MODE_DELETE                   "/fingerprint/mode/delete"
#define AVAILABILITY_TOPIC            "/fingerprint/available"
#define mqtt_username                 "mqtt-user"
#define mqtt_password                 "xxxxxxxxxxx"

#define MQTT_INTERVAL 5000            //MQTT rate limiting when no finger present, in ms

// UART Pins for the fingerprint sensor
#define SENSOR_TX 17                  // TX pin for the fingerprint sensor
#define SENSOR_RX 16                  // RX pin for the fingerprint sensor

// Hardware serial port for fingerprint sensor
HardwareSerial mySerial(1);           // Use UART1 on ESP32

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

WiFiClient wifiClient;                // Initiate WiFi library
PubSubClient client(wifiClient);      // Initiate PubSubClient library

uint8_t id = 0;                       // Stores the current fingerprint ID
uint8_t lastID = 0;                   // Stores the last matched ID
uint8_t lastConfidenceScore = 0;      // Stores the last matched confidence score
boolean modeLearning = false;
boolean modeReading = true;
boolean modeDelete = false;
unsigned long lastMQTTmsg = 0;	      // Stores millis since last MQTT message

// Declare JSON variables
DynamicJsonDocument mqttMessage(100);
char mqttBuffer[100];

// Function prototypes
uint8_t getFingerprintID();
void enrollFingerprint(uint8_t id, const char* mode);
void deleteFingerprint(uint8_t id, const char* mode);
bool isFingerprintEnrolled(uint8_t id);  // New function to check if fingerprint is already enrolled

void setup() {
  Serial.begin(115200);
  mySerial.begin(57600, SERIAL_8N1, SENSOR_RX, SENSOR_TX);  // Initialize hardware serial for fingerprint sensor
  delay(500);
  Serial.println("\n\nWelcome to the MQTT Fingerprint Sensor program!");

  // Start fingerprint sensor
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) {
      delay(10);
    }
  }

  // Connect to WiFi
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  // Set MQTT server
  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();                // Reconnect if disconnected from MQTT server
  }
  
  if (modeReading && !modeLearning && !modeDelete) {
    Serial.println("Reading mode active.");
    uint8_t result = getFingerprintID();
    handleFingerprintResult(result, "reading");
  }
  
  // Handle learning mode (enrolling)
  else if (modeLearning) {
    Serial.println("Learning mode active.");
    enrollFingerprint(id , "learning");      // Use the ID from the received message
    modeLearning = false;                    // Reset modes
    modeReading = true;
  }

  // Handle delete mode
  else if (modeDelete) {
    Serial.println("Delete mode active.");
    deleteFingerprint(id, "delete");         // Use the ID from the received message
    modeDelete = false;                      // Reset modes
    modeReading = true;
  }

  client.loop();
  delay(1500);
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p == FINGERPRINT_OK) {
    p = finger.image2Tz(1);
    if (p != FINGERPRINT_OK) return p;
    p = finger.fingerFastSearch();
    if (p == FINGERPRINT_OK) {
      lastID = finger.fingerID;
      lastConfidenceScore = finger.confidence;
      return FINGERPRINT_OK;
    } else {
      return FINGERPRINT_NOTFOUND;
    }
  }
  return FINGERPRINT_NOFINGER;
}

void handleFingerprintResult(uint8_t result, const char* mode) {
  if (result == FINGERPRINT_OK) {
    Serial.println("Fingerprint matched.");
    mqttMessage["mode"] = mode;
    mqttMessage["id"] = lastID;
    mqttMessage["state"] = "Matched";
    mqttMessage["confidence"] = lastConfidenceScore;
    serializeJson(mqttMessage, mqttBuffer);
    client.publish(STATE_TOPIC, mqttBuffer);
    Serial.println(mqttBuffer); 
    lastMQTTmsg = millis();
    delay(2000);
  } else if (result == FINGERPRINT_NOTFOUND) {
    Serial.println("Fingerprint not found.");
    mqttMessage["mode"] = mode;
    mqttMessage["match"] = false;            // <=== **แก้ไข** 
    mqttMessage["id"] = id;
    mqttMessage["state"] = "Not matched";
    serializeJson(mqttMessage, mqttBuffer);
    client.publish(STATE_TOPIC, mqttBuffer);
    Serial.println(mqttBuffer); 
    lastMQTTmsg = millis();
    delay(2000);
  } else if (result == FINGERPRINT_NOFINGER) {
    Serial.println("Waiting for a finger...");
    if ((millis() - lastMQTTmsg) > MQTT_INTERVAL) {
      mqttMessage["mode"] = mode;
      mqttMessage["id"] = id;
      mqttMessage["state"] = "Waiting";
      mqttMessage["match"] = false;         // <=== **แก้ไข** 
      serializeJson(mqttMessage, mqttBuffer);
      client.publish(STATE_TOPIC, mqttBuffer);
      Serial.println(mqttBuffer); 
      lastMQTTmsg = millis();
    }
  }
}

// Function to check if the fingerprint is already enrolled
bool isFingerprintEnrolled(uint8_t id) {
  uint8_t p = finger.loadModel(id);
  return (p == FINGERPRINT_OK);
}

// Function to enroll a new fingerprint
void enrollFingerprint(uint8_t id, const char* mode) {
  // Check if the fingerprint is already enrolled
  if (isFingerprintEnrolled(id)) {
    Serial.println("Fingerprint already enrolled.");
    mqttMessage["mode"] = mode;
    mqttMessage["id"] = id;
    mqttMessage["state"] = "Already Enrolled"; // Consistent state message
    serializeJson(mqttMessage, mqttBuffer);
    client.publish(STATE_TOPIC, mqttBuffer);   // Notify Home Assistant
    return;
  }

  Serial.print("Enrolling ID #");
  Serial.println(id);
  
  int p = -1;
  Serial.println("Waiting for valid finger to enroll...");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
      Serial.print(".");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("Communication error");
      return;
    } else if (p == FINGERPRINT_IMAGEFAIL) {
      Serial.println("Imaging error");
      return;
    } else {
      Serial.println("Image taken");
    }
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("Error converting image");
    return;
  }

  Serial.println("Remove finger");
  delay(2000);

  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }

  Serial.println("Place same finger again...");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
      Serial.print(".");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("Communication error");
      return;
    } else if (p == FINGERPRINT_IMAGEFAIL) {
      Serial.println("Imaging error");
      return;
    } else {
      Serial.println("Image taken");
    }
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.println("Error converting image");
    return;
  }

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else {
    Serial.println("Prints did not match");
    return;
  }

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored successfully!");
    mqttMessage["mode"] = mode; // Use the provided mode
    mqttMessage["id"] = id;      // Use the current ID
    mqttMessage["state"] = "Enrolled Successfully"; // Consistent state message
    serializeJson(mqttMessage, mqttBuffer);
    client.publish(STATE_TOPIC, mqttBuffer);   // Notify Home Assistant
  } else {
    Serial.println("Error storing fingerprint");
  }
}

void deleteFingerprint(uint8_t id, const char* mode) {
  if (isFingerprintEnrolled(id)) {
    int p = finger.deleteModel(id);
    if (p == FINGERPRINT_OK) {
      Serial.print("Deleted fingerprint with ID ");
      Serial.println(id);
      mqttMessage["mode"] = mode;
      mqttMessage["id"] = id;
      mqttMessage["state"] = "Deleted Successfully"; // Consistent state message
      serializeJson(mqttMessage, mqttBuffer);
      client.publish(STATE_TOPIC, mqttBuffer);   // Notify Home Assistant
    } else {
      Serial.println("Error deleting fingerprint");
    }
  } else {
    Serial.print("No fingerprint with ID ");
    Serial.println(id);
    mqttMessage["mode"] = mode;
    mqttMessage["id"] = id;
    mqttMessage["state"] = "Not Enrolled";  // Consistent state message
    serializeJson(mqttMessage, mqttBuffer);
    client.publish(STATE_TOPIC, mqttBuffer);   // Notify Home Assistant
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0'; // Null-terminate the payload
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println((char*)payload);

  // Compare the topic with strings using strcmp
  if (strcmp(topic, MODE_LEARNING) == 0) {
    Serial.println("Learning mode activated.");
    modeLearning = true;
    modeReading = false;
    modeDelete = false;
  } else if (strcmp(topic, MODE_READING) == 0) {
    Serial.println("Reading mode activated.");
    modeLearning = false;
    modeReading = true;
    modeDelete = false;
  } else if (strcmp(topic, MODE_DELETE) == 0) {
    Serial.println("Delete mode activated.");
    modeLearning = false;
    modeReading = false;
    modeDelete = true;
  } else {
    Serial.println("Unknown topic");
  }

  // If the topic is a mode topic, parse the ID from the payload
  if (modeLearning || modeDelete) {
    if (payload[0] >= '0' && payload[0] <= '9') { // Check if the first character is a digit
      id = atoi((char*)payload); // Convert payload to ID
      Serial.print("ID received: ");
      Serial.println(id); // Debug the ID received  **<=== เพิ่มการดีบั๊ก** 
    } else {
      Serial.println("Invalid ID received.");
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(HOSTNAME, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.publish(AVAILABILITY_TOPIC, "online");
      client.subscribe(MODE_LEARNING);
      client.subscribe(MODE_READING);
      client.subscribe(MODE_DELETE);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
