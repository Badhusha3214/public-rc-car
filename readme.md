Certainly. I'll provide you with an updated version of the code that addresses the issues we discussed. Then I'll explain what you need to connect.

```cpp
#include <WiFi.h>
#include <HTTPClient.h>

// WiFi credentials
#define MAX_NETWORKS 4
const char* ssids[MAX_NETWORKS] = { "KCMT-LAB","Chacko Mash","KCMT-ENG-FLR-A",  "master"  };
const char* passwords[MAX_NETWORKS] = { "1ab2ac3ad4ae5af","marannupoi","1ab2ac3ad4ae5af",  "123456789"  };

// API endpoint
const char* apiEndpoint = "https://public-rc-car.onrender.com/status";

// Define the motor control pins for L298N
#define MOTOR_LEFT_IN1 26
#define MOTOR_LEFT_IN2 25
#define MOTOR_RIGHT_IN1 33
#define MOTOR_RIGHT_IN2 32

// Define PWM channels
#define PWM_CHANNEL_LEFT 0
#define PWM_CHANNEL_RIGHT 1

// Define LED pins
#define WIFI_LED_PIN 13
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

  // Configure PWM
  ledcSetup(PWM_CHANNEL_LEFT, 5000, 8);  // 5000 Hz PWM, 8-bit resolution
  ledcSetup(PWM_CHANNEL_RIGHT, 5000, 8);
  ledcAttachPin(MOTOR_LEFT_IN1, PWM_CHANNEL_LEFT);
  ledcAttachPin(MOTOR_RIGHT_IN1, PWM_CHANNEL_RIGHT);

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
    digitalWrite(WIFI_LED_PIN, LOW);
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

      turnOffAllLEDs();

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
  ledcWrite(PWM_CHANNEL_LEFT, 200);  // ~78% speed
  ledcWrite(PWM_CHANNEL_RIGHT, 200);
  digitalWrite(MOTOR_LEFT_IN2, LOW);
  digitalWrite(MOTOR_RIGHT_IN2, LOW);
  Serial.println("Moving Forward");
}

void moveBackward() {
  ledcWrite(PWM_CHANNEL_LEFT, 200);
  ledcWrite(PWM_CHANNEL_RIGHT, 200);
  digitalWrite(MOTOR_LEFT_IN2, HIGH);
  digitalWrite(MOTOR_RIGHT_IN2, HIGH);
  Serial.println("Moving Backward");
}

void turnLeft() {
  ledcWrite(PWM_CHANNEL_LEFT, 150);  // Slower speed for turning
  ledcWrite(PWM_CHANNEL_RIGHT, 150);
  digitalWrite(MOTOR_LEFT_IN2, HIGH);
  digitalWrite(MOTOR_RIGHT_IN2, LOW);
  Serial.println("Turning Left");
}

void turnRight() {
  ledcWrite(PWM_CHANNEL_LEFT, 150);
  ledcWrite(PWM_CHANNEL_RIGHT, 150);
  digitalWrite(MOTOR_LEFT_IN2, LOW);
  digitalWrite(MOTOR_RIGHT_IN2, HIGH);
  Serial.println("Turning Right");
}

void stop() {
  ledcWrite(PWM_CHANNEL_LEFT, 0);
  ledcWrite(PWM_CHANNEL_RIGHT, 0);
  digitalWrite(MOTOR_LEFT_IN2, LOW);
  digitalWrite(MOTOR_RIGHT_IN2, LOW);
  Serial.println("Stopped");
}

```

Now, let me explain what you need to connect:

1. **ESP32 Board**: This is your main controller.

2. **L298N Motor Driver**: This controls the motors.

   - Connect VCC to 5V on ESP32
   - Connect GND to GND on ESP32
   - Connect IN1 to GPIO 26 on ESP32
   - Connect IN2 to GPIO 25 on ESP32
   - Connect IN3 to GPIO 33 on ESP32
   - Connect IN4 to GPIO 32 on ESP32
   - Connect ENA and ENB to 5V (or to additional GPIO pins if you want software PWM control)

3. **DC Motors**: Connect these to the output terminals on the L298N.

   - Left motor to OUT1 and OUT2
   - Right motor to OUT3 and OUT4

4. **LEDs**: Connect these to the specified GPIO pins on the ESP32 through appropriate resistors (typically 220-330 ohms).

   - WiFi LED to GPIO 13
   - Error LED to GPIO 12
   - Forward LED to GPIO 14
   - Reverse LED to GPIO 27
   - Left LED to GPIO 16
   - Right LED to GPIO 17
   - Stop LED to GPIO 5

5. **Power Supply**:
   - Connect a suitable power supply to the L298N to power the motors. This is typically 6-12V depending on your motors.
   - The ESP32 can be powered via its USB port or through a 5V regulator connected to the motor power supply.

Key changes in the code:

1. PWM is now used for motor speed control.
2. The fetch interval has been increased to 1 second to reduce network load.
3. Motor control functions have been updated to use PWM.
4. Error handling has been improved.

Make sure to upload this code to your ESP32 board. After connecting everything as described, the RC car should respond to commands from the API endpoint more reliably and with better control over its movements.
