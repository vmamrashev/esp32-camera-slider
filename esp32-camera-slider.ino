#include <WiFi.h>
#include <WebServer.h>
#include <Stepper.h>

// WiFi settings
const char* ssid = "ESP32-camera-slider";
const char* password = "123456789";

// Motor settings
const int STEPS_PER_REV = 2048; // For 28BYJ-48 in full step mode; adjust to 4096 for half-step if needed
Stepper stepperMotor(STEPS_PER_REV, 26, 27, 14, 12); // Pins: IN1=26, IN3=27, IN2=14, IN4=12 (standard ULN2003 wiring)

// Endstop pins
const int ENDSTOP_LEFT = 33;
const int ENDSTOP_RIGHT = 32;

// Variables
int direction = 1; // 1 for CW, -1 for CCW
int speedRPM = 0;
bool isRunning = false;
unsigned long lastStepTime = 0;
int stepDelay = 0; // ms between steps

WebServer esp32Server(80);

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <title>Slider controls</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
        }

        .container {
            text-align: center;
        }

        .top-row {
            display: flex;
            align-items: center;
            gap: 20px;
        }

        .arrow-btn {
            padding: 15px 25px;
            font-size: 16px;
            cursor: pointer;
            background-color: #f0f0f0;
            border: 1px solid #ccc;
        }

        .arrow-btn.selected {
            background-color: #a0a0a0;
        }

        .speed-block {
            display: flex;
            flex-direction: column;
            align-items: center;
        }

        .speed-block label {
            margin-bottom: 5px;
            font-weight: bold;
        }

        .speed-block input {
            width: 120px;
            padding: 8px;
            font-size: 16px;
            text-align: center;
        }

        .start-btn {
            margin-top: 20px;
            padding: 10px 30px;
            font-size: 16px;
            cursor: pointer;
        }
    </style>
</head>
<body>

<div class="container">
    <div class="top-row">
        <button id="leftBtn" class="arrow-btn" onclick="selectDirection('left')"> &#8592 Left</button>

        <div class="speed-block">
            <label for="speed">Speed</label>
            <input type="number" id="speed" placeholder="0">
        </div>

        <button id="rightBtn" class="arrow-btn" onclick="selectDirection('right')">Right &#8594 </button>
    </div>

    <button id="startBtn" class="start-btn" onclick="toggleStart()">Start</button>
</div>

<script>
    let currentDirection = 'right'; // Default to right
    document.getElementById('rightBtn').classList.add('selected');

    function selectDirection(dir) {
        fetch('/direction?dir=' + dir);
        if (dir === 'left') {
            document.getElementById('leftBtn').classList.add('selected');
            document.getElementById('rightBtn').classList.remove('selected');
        } else {
            document.getElementById('rightBtn').classList.add('selected');
            document.getElementById('leftBtn').classList.remove('selected');
        }
        currentDirection = dir;
    }

    function toggleStart() {
        let btn = document.getElementById('startBtn');
        let speed = document.getElementById('speed').value;
        if (btn.innerText === 'Start') {
            fetch('/start?speed=' + speed);
            btn.innerText = 'Stop';
        } else {
            fetch('/stop');
            btn.innerText = 'Start';
        }
    }
</script>

</body>
</html>
)rawliteral";

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);

  // Set up endstops with internal pull-up
  pinMode(ENDSTOP_LEFT, INPUT_PULLUP);
  pinMode(ENDSTOP_RIGHT, INPUT_PULLUP);

  // Set up WiFi AP
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  // Web server routes
  esp32Server.on("/", HTTP_GET, []() {
    esp32Server.send(200, "text/html", htmlPage);
  });

  esp32Server.on("/direction", HTTP_GET, []() {
    String dir = esp32Server.arg("dir");
    if (dir == "left") {
      direction = -1;
      Serial.println("direction left");
    } else if (dir == "right") {
      direction = 1;
      Serial.println("direction right");
    }
    esp32Server.send(200, "text/plain", "OK");
  });

  esp32Server.on("/start", HTTP_GET, []() {
    String speedStr = esp32Server.arg("speed");
    speedRPM = speedStr.toInt();
    if (speedRPM > 0) {
      // Calculate step delay: steps per second = (RPM / 60) * STEPS_PER_REV
      // delay ms = 1000 / steps_per_second
      float stepsPerSecond = (speedRPM / 60.0) * STEPS_PER_REV;
      stepDelay = 1000 / stepsPerSecond;
      isRunning = true;
      Serial.println("Stepper motor is running.");
    }
    esp32Server.send(200, "text/plain", "OK");
  });

  esp32Server.on("/stop", HTTP_GET, []() {
    isRunning = false;
    esp32Server.send(200, "text/plain", "OK");
    Serial.println("Stepper motor has been stopped.");
  });

  esp32Server.begin();
  Serial.println("HTTP server started");

  // Set initial motor speed (absolute value)
  stepperMotor.setSpeed(60); // Default, but we'll control manually
}

void loop() {
  esp32Server.handleClient();

  // Check endstops
  if (digitalRead(ENDSTOP_LEFT) == LOW || digitalRead(ENDSTOP_RIGHT) == LOW) {
    direction = -direction; // Reverse direction
    if (direction >0){
        Serial.println("direction right");
    }
    else {
        Serial.println("direction left");
    }
    delay(100); // Debounce
    
  }

  if (isRunning && stepDelay > 0) {
    if (millis() - lastStepTime >= stepDelay) {
      stepperMotor.step(direction); // Step in current direction
      lastStepTime = millis();
    }
  }
}