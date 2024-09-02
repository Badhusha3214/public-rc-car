#include <WiFi.h>
#include <HTTPClient.h>

// WiFi credentials
#define MAX_NETWORKS 4
const char* ssids[MAX_NETWORKS] = { "KCMT-ENG-FLR-A", "KCMT-LAB", "master", "Chacko Mash" };
const char* passwords[MAX_NETWORKS] = { "1ab2ac3ad4ae5af", "1ab2ac3ad4ae5af", "123456789", "marannupoi" };

// API endpoint
const char* apiEndpoint = "https://public-rc-car.onrender.com/status";

// Define the motor control pins for L298N
#define MOTOR_LEFT_IN1 26
#define MOTOR_LEFT_IN2 25
#define MOTOR_RIGHT_IN1 33
#define MOTOR_RIGHT_IN2 32

// Define LED pins
#define WIFI_LED_PIN 13 // Using built-in LED on most ESP32 boards
#define ERROR_LED_PIN 12
#define FORWARD_LED_PIN 14
#define REVERSE_LED_PIN 27
#define LEFT_LED_PIN 16
#define RIGHT_LED_PIN 17
#define STOP_LED_PIN 5

unsigned long lastFetchTime = 0;
const unsigned long fetchInterval = 1000; // Fetch every 1 second

void printLoader(const char* message) {
  static const char loader[] = {'|', '/', '-', '\\'};
  static int loaderIndex = 0;
  Serial.printf("\r%s %c", message, loader[loaderIndex]);
  loaderIndex = (loaderIndex + 1) % 4;
}

bool connectToWiFi() {
  digitalWrite(WIFI_LED_PIN, LOW); // Turn off LED while attempting to connect
  
  for (int i = 0; i < MAX_NETWORKS; i++) {
    if (strlen(ssids[i]) == 0) break; // Skip empty SSID
    Serial.printf("\nAttempting to connect to %s\n", ssids[i]);
    WiFi.begin(ssids[i], passwords[i]);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      printLoader("Connecting to WiFi");
      delay(500);
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to WiFi successfully");
      Serial.printf("Connected to: %s\n", ssids[i]);
      Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
      digitalWrite(WIFI_LED_PIN, HIGH); // Turn on LED when connected

      // Set static DNS to Google's public DNS
      WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, IPAddress(8,8,8,8));
      return true;
    } else {
      Serial.printf("\nFailed to connect to %s\n", ssids[i]);
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32 RC Car Control - Initializing...");
  
  // Set motor control pins as outputs
  pinMode(MOTOR_LEFT_IN1, OUTPUT);
  pinMode(MOTOR_LEFT_IN2, OUTPUT);
  pinMode(MOTOR_RIGHT_IN1, OUTPUT);
  pinMode(MOTOR_RIGHT_IN2, OUTPUT);
  
  // Set LED pins as outputs
  pinMode(WIFI_LED_PIN, OUTPUT);
  pinMode(ERROR_LED_PIN, OUTPUT);
  pinMode(FORWARD_LED_PIN, OUTPUT);
  pinMode(REVERSE_LED_PIN, OUTPUT);
  pinMode(LEFT_LED_PIN, OUTPUT);
  pinMode(RIGHT_LED_PIN, OUTPUT);
  pinMode(STOP_LED_PIN, OUTPUT);
  
  // Initialize all LEDs to OFF
  digitalWrite(WIFI_LED_PIN, LOW);
  digitalWrite(ERROR_LED_PIN, LOW);
  digitalWrite(FORWARD_LED_PIN, LOW);
  digitalWrite(REVERSE_LED_PIN, LOW);
  digitalWrite(LEFT_LED_PIN, LOW);
  digitalWrite(RIGHT_LED_PIN, LOW);
  digitalWrite(STOP_LED_PIN, LOW);
  
  Serial.println("Motor control pins and LEDs initialized");
  
  // Connect to Wi-Fi
  if (!connectToWiFi()) {
    Serial.println("Failed to connect to any WiFi network. Restarting...");
    ESP.restart();
  }

  Serial.println("Setup complete. Entering main loop...");
}

void turnOffAllLEDs() {
  digitalWrite(ERROR_LED_PIN, LOW);
  digitalWrite(FORWARD_LED_PIN, LOW);
  digitalWrite(REVERSE_LED_PIN, LOW);
  digitalWrite(LEFT_LED_PIN, LOW);
  digitalWrite(RIGHT_LED_PIN, LOW);
  digitalWrite(STOP_LED_PIN, LOW);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Attempting to reconnect...");
    digitalWrite(WIFI_LED_PIN, LOW); // Turn off LED when disconnected
    if (!connectToWiFi()) {
      Serial.println("Failed to reconnect to any WiFi network. Restarting...");
      ESP.restart();
    }
  }

  if (millis() - lastFetchTime > fetchInterval) {
    lastFetchTime = millis();
    
    HTTPClient http;
    http.begin(apiEndpoint);
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("API Response: " + response);
      
      turnOffAllLEDs(); // Turn off all LEDs before setting the new state
      
      // Control the car based on the received command
      if (response == "forward") {
        moveForward();
        digitalWrite(FORWARD_LED_PIN, HIGH);
      } else if (response == "reverse") {
        moveBackward();
        digitalWrite(REVERSE_LED_PIN, HIGH);
      } else if (response == "left") {
        turnLeft();
        digitalWrite(LEFT_LED_PIN, HIGH);
      } else if (response == "right") {
        turnRight();
        digitalWrite(RIGHT_LED_PIN, HIGH);
      } else if (response == "stop") {
        stop();
        digitalWrite(STOP_LED_PIN, HIGH);
      } else {
        Serial.println("Unknown command received");
        // Blink the error LED for unknown commands
        for (int i = 0; i < 3; i++) {
          digitalWrite(ERROR_LED_PIN, HIGH);
          delay(100);
          digitalWrite(ERROR_LED_PIN, LOW);
          delay(100);
        }
      }
    } else {
      Serial.print("Error on HTTP request. Error code: ");
      Serial.println(httpResponseCode);
      // Turn on the error LED for HTTP request errors
      digitalWrite(ERROR_LED_PIN, HIGH);
    }
    
    http.end();
  }
}

void moveForward() {
  digitalWrite(MOTOR_LEFT_IN1, HIGH);
  digitalWrite(MOTOR_LEFT_IN2, LOW);
  digitalWrite(MOTOR_RIGHT_IN1, HIGH);
  digitalWrite(MOTOR_RIGHT_IN2, LOW);
  Serial.println("Moving Forward");
}

void moveBackward() {
  digitalWrite(MOTOR_LEFT_IN1, LOW);
  digitalWrite(MOTOR_LEFT_IN2, HIGH);
  digitalWrite(MOTOR_RIGHT_IN1, LOW);
  digitalWrite(MOTOR_RIGHT_IN2, HIGH);
  Serial.println("Moving Backward");
}

void turnLeft() {
  digitalWrite(MOTOR_LEFT_IN1, LOW);
  digitalWrite(MOTOR_LEFT_IN2, HIGH);
  digitalWrite(MOTOR_RIGHT_IN1, HIGH);
  digitalWrite(MOTOR_RIGHT_IN2, LOW);
  Serial.println("Turning Left");
}

void turnRight() {
  digitalWrite(MOTOR_LEFT_IN1, HIGH);
  digitalWrite(MOTOR_LEFT_IN2, LOW);
  digitalWrite(MOTOR_RIGHT_IN1, LOW);
  digitalWrite(MOTOR_RIGHT_IN2, HIGH);
  Serial.println("Turning Right");
}

void stop() {
  digitalWrite(MOTOR_LEFT_IN1, LOW);
  digitalWrite(MOTOR_LEFT_IN2, LOW);
  digitalWrite(MOTOR_RIGHT_IN1, LOW);
  digitalWrite(MOTOR_RIGHT_IN2, LOW);
  Serial.println("Stopped");
}