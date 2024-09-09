#if defined(ESP32)
  #include <WiFi.h>
  #include <HTTPClient.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
  #include <WiFiClientSecure.h>
#else
  #error "This code is intended for ESP32 or ESP8266 boards only."
#endif

// WiFi credentials
#define MAX_NETWORKS 4
const char* ssids[MAX_NETWORKS] = { "KCMT-LAB","KCMT-ENG-FLR-A",  "master"  };
const char* passwords[MAX_NETWORKS] = { "1ab2ac3ad4ae5af","1ab2ac3ad4ae5af",  "123456789"  };

// API endpoint (note the https)
const char* apiEndpoint = "https://public-rc-car.onrender.com/status";

//esp8266

// Define the motor control pins for L298N
#define MOTOR_LEFT_ENA 16 //0
#define MOTOR_LEFT_IN1 5  //1
#define MOTOR_LEFT_IN2 4  //2
#define MOTOR_RIGHT_ENB 0 //3
#define MOTOR_RIGHT_IN1 2 //4
#define MOTOR_RIGHT_IN2 14//5

// Define LED pins
#define WIFI_LED_PIN 6   //6
#define ERROR_LED_PIN 7  //7


// #define FORWARD_LED_PIN 8
// #define REVERSE_LED_PIN 8
// #define LEFT_LED_PIN 8
// #define RIGHT_LED_PIN 8
// #define STOP_LED_PIN 8

//esp32

// // Define the motor control pins for L298N
// #define MOTOR_LEFT_ENA 26
// #define MOTOR_LEFT_IN1 25
// #define MOTOR_LEFT_IN2 33
// #define MOTOR_RIGHT_ENB 27
// #define MOTOR_RIGHT_IN1 14
// #define MOTOR_RIGHT_IN2 12

// // Define LED pins
// #define WIFI_LED_PIN 13
// #define ERROR_LED_PIN 2
// #define FORWARD_LED_PIN 15
// #define REVERSE_LED_PIN 0
// #define LEFT_LED_PIN 4
// #define RIGHT_LED_PIN 16
// #define STOP_LED_PIN 17



// Define speed constants
#define MAX_SPEED 255
#define HIGH_SPEED 200  // ~78% of max speed
#define MEDIUM_SPEED 225  // ~69% of max speed
#define LOW_SPEED 225  // ~69% of max speed

unsigned long lastFetchTime = 0;
const unsigned long fetchInterval = 250; // Fetch every 0.25 seconds

#ifdef ESP8266
WiFiClientSecure wifiClient;
#endif

void printLoader(const char* message) {
  static const char loader[] = {'|', '/', '-', '\\'};
  static int loaderIndex = 0;
  Serial.printf("\r%s %c", message, loader[loaderIndex]);
  loaderIndex = (loaderIndex + 1) % 4;
}

bool connectToWiFi() {
  digitalWrite(WIFI_LED_PIN, LOW);
  
  for (int i = 0; i < MAX_NETWORKS; i++) {
    if (strlen(ssids[i]) == 0) break;
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
      digitalWrite(WIFI_LED_PIN, HIGH);
      #ifdef ESP32
      WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, IPAddress(8,8,8,8));
      #endif
      return true;
    } else {
      Serial.printf("\nFailed to connect to %s\n", ssids[i]);
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP RC Car Control - Initializing...");
  
  // Set all pins as outputs
  pinMode(MOTOR_LEFT_ENA, OUTPUT);
  pinMode(MOTOR_LEFT_IN1, OUTPUT);
  pinMode(MOTOR_LEFT_IN2, OUTPUT);
  pinMode(MOTOR_RIGHT_ENB, OUTPUT);
  pinMode(MOTOR_RIGHT_IN1, OUTPUT);
  pinMode(MOTOR_RIGHT_IN2, OUTPUT);
  pinMode(WIFI_LED_PIN, OUTPUT);
  pinMode(ERROR_LED_PIN, OUTPUT);
  // pinMode(FORWARD_LED_PIN, OUTPUT);
  // pinMode(REVERSE_LED_PIN, OUTPUT);
  // pinMode(LEFT_LED_PIN, OUTPUT);
  // pinMode(RIGHT_LED_PIN, OUTPUT);
  // pinMode(STOP_LED_PIN, OUTPUT);
  
  // Initialize motor speed to 0
  analogWrite(MOTOR_LEFT_ENA, 0);
  analogWrite(MOTOR_RIGHT_ENB, 0);
  
  // Initialize all LEDs to OFF
  digitalWrite(WIFI_LED_PIN, LOW);
  digitalWrite(ERROR_LED_PIN, LOW);
  // digitalWrite(FORWARD_LED_PIN, LOW);
  // digitalWrite(REVERSE_LED_PIN, LOW);
  // digitalWrite(LEFT_LED_PIN, LOW);
  // digitalWrite(RIGHT_LED_PIN, LOW);
  // digitalWrite(STOP_LED_PIN, LOW);
  
  Serial.println("Motor control pins and LEDs initialized");
  
  // Connect to Wi-Fi
  if (!connectToWiFi()) {
    Serial.println("Failed to connect to any WiFi network. Restarting...");
    ESP.restart();
  }

  #ifdef ESP8266
  wifiClient.setInsecure(); // Allow HTTPS connections without certificate verification
  #endif

  Serial.println("Setup complete. Entering main loop...");
}

void turnOffAllLEDs() {
  digitalWrite(ERROR_LED_PIN, LOW);
  // digitalWrite(FORWARD_LED_PIN, LOW);
  // digitalWrite(REVERSE_LED_PIN, LOW);
  // digitalWrite(LEFT_LED_PIN, LOW);
  // digitalWrite(RIGHT_LED_PIN, LOW);
  // digitalWrite(STOP_LED_PIN, LOW);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Attempting to reconnect...");
    digitalWrite(WIFI_LED_PIN, LOW);
    if (!connectToWiFi()) {
      Serial.println("Failed to reconnect to any WiFi network. Restarting...");
      ESP.restart();
    }
  }

  if (millis() - lastFetchTime > fetchInterval) {
    lastFetchTime = millis();
    
    HTTPClient http;
    #ifdef ESP32
    http.begin(apiEndpoint);
    #else
    http.begin(wifiClient, apiEndpoint);
    #endif
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("API Response: " + response);
      
      turnOffAllLEDs();
      
      if (response == "forward") {
        moveForward();
        // digitalWrite(FORWARD_LED_PIN, HIGH);
      } else if (response == "reverse") {
        moveBackward();
        // digitalWrite(REVERSE_LED_PIN, HIGH);
      } else if (response == "left") {
        turnLeft();
        // digitalWrite(LEFT_LED_PIN, HIGH);
      } else if (response == "right") {
        turnRight();
        // digitalWrite(RIGHT_LED_PIN, HIGH);
      } else if (response == "stop") {
        stop();
        // digitalWrite(STOP_LED_PIN, HIGH);
      } else {
        Serial.println("Unknown command received: " + response);
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
      digitalWrite(ERROR_LED_PIN, HIGH);
    }
    
    http.end();
  }
}

void moveForward() {
  analogWrite(MOTOR_LEFT_ENA, MEDIUM_SPEED);
  analogWrite(MOTOR_RIGHT_ENB, MEDIUM_SPEED);
  digitalWrite(MOTOR_LEFT_IN1, HIGH);
  digitalWrite(MOTOR_LEFT_IN2, LOW);
  digitalWrite(MOTOR_RIGHT_IN1, HIGH);
  digitalWrite(MOTOR_RIGHT_IN2, LOW);
  Serial.println("Moving Forward");
}

void moveBackward() {
  analogWrite(MOTOR_LEFT_ENA, MEDIUM_SPEED);
  analogWrite(MOTOR_RIGHT_ENB, MEDIUM_SPEED);
  digitalWrite(MOTOR_LEFT_IN1, LOW);
  digitalWrite(MOTOR_LEFT_IN2, HIGH);
  digitalWrite(MOTOR_RIGHT_IN1, LOW);
  digitalWrite(MOTOR_RIGHT_IN2, HIGH);
  Serial.println("Moving Backward");
}

void turnLeft() {
  analogWrite(MOTOR_LEFT_ENA, LOW_SPEED);
  analogWrite(MOTOR_RIGHT_ENB, LOW_SPEED);
  digitalWrite(MOTOR_LEFT_IN1, LOW);
  digitalWrite(MOTOR_LEFT_IN2, HIGH);
  digitalWrite(MOTOR_RIGHT_IN1, HIGH);
  digitalWrite(MOTOR_RIGHT_IN2, LOW);
  Serial.println("Turning Left");
}

void turnRight() {
  analogWrite(MOTOR_LEFT_ENA, LOW_SPEED);
  analogWrite(MOTOR_RIGHT_ENB, LOW_SPEED);
  digitalWrite(MOTOR_LEFT_IN1, HIGH);
  digitalWrite(MOTOR_LEFT_IN2, LOW);
  digitalWrite(MOTOR_RIGHT_IN1, LOW);
  digitalWrite(MOTOR_RIGHT_IN2, HIGH);
  Serial.println("Turning Right");
}

void stop() {
  analogWrite(MOTOR_LEFT_ENA, 0);
  analogWrite(MOTOR_RIGHT_ENB, 0);
  digitalWrite(MOTOR_LEFT_IN1, LOW);
  digitalWrite(MOTOR_LEFT_IN2, LOW);
  digitalWrite(MOTOR_RIGHT_IN1, LOW);
  digitalWrite(MOTOR_RIGHT_IN2, LOW);
  Serial.println("Stopped");
}