#include <WiFi.h> 
#include <DHT.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <ESP32Servo.h>

#define DHTTYPE DHT11

int DHTPin = 14;
DHT dht(DHTPin, DHTTYPE);

float hum;
float temp;

Servo servo;

int ledPin = 12;
int ledPin2 = 26;

const char *ssid = "Your SSID";
const char *password = "Your Password";

WebServer server(80);

const char *htmlContent = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Document</title>
    <style>
        html {
            font-family: 'Courier New', Courier, monospace;
            margin: 0;
            padding: 0;
            background-color: rgba(0, 0, 0, 0.8);
            height: 100%;
            width: 100%;
        }
        body {
            color: #fff;
            display: flex;
            flex-direction: column;
            align-items: center;
        }
        .button {
            border-radius: 10px;
            padding: 10px 15px;
            margin: 20px 20px;
        }
        .weatherSection {
            background-color: black;
            display: flex;
            flex-direction: column;
            align-items: center;
            border-radius: 5px;
            padding-right: 10%;
            margin: 10px 10px;
        }
    </style>
</head>
<body>
    <h1>Server PC Switch + Ambient Temp Monitor!</h1>
    <p>Press the button to turn PC On/Off</p>
    <button class="button" onclick="moveServoOpen()">Cycle Left</button>
    <button class="button" onclick="moveServoClose()">Cycle Right</button>
    <h2>Current Room's Temperature Readings...</h2>
    <div class="weatherSection">
    <button class="button" onclick="getTemperature()">Get Temperature</button>
    <p id="Temperature">Temperature:</p>
    <button class="button" onclick="getHumidity()">Get Humidity</button>
    <p id="Humidity">Humidity:</p>
    </div>
    <script>
      function moveServoOpen() {
        fetch('/moveServoOpen').then(response => response.text()).then(data => {
          alert("Servo has been moved left!");
        });
      }
      function moveServoClose() {
        fetch('/moveServoClose').then(response => response.text()).then(data => {
          alert("Servo has been moved right!");
        });
      }
      function getTemperature() {
        fetch('/Temperature').then(response => response.text()).then(data => {
          document.getElementById('Temperature').innerText = `Temperature: ${data} °F`;
        });
      }
      function getHumidity() {
        fetch('/Humidity').then(response => response.text()).then(data => {
          document.getElementById('Humidity').innerText = `Humidity: ${data} %`;
        });
      }
    </script>
</body>
</html>
)rawliteral";

int pos = 0;

void flashLed() {
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin2, HIGH);
  delay(1000);
  digitalWrite(ledPin, LOW);
  delay(100);
  digitalWrite(ledPin2, LOW);
  delay(100);
}

void errLed() {
  digitalWrite(ledPin2, HIGH);
  delay(500);
  digitalWrite(ledPin2, LOW);
  delay(100);
}

void led200() {
  digitalWrite(ledPin, HIGH);
  delay(400);
  digitalWrite(ledPin, LOW);
  delay(100);
}

void handleRoot() {
  server.send(200, "text/html", htmlContent);
  led200();
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  server.send(404, "text/plain", message);
  errLed();
}

void handleMoveServoOpen() {
  pos = servo.read();
  if (pos <= 0) {
    servo.write(180);
  } else {
    servo.write(0);
  }
  delay(15);
  server.send(200, "text/plain", "Servo moved");
  flashLed();
}

void handleMoveServoClose() {
  pos = servo.read();
  if (pos >= 160) {
    servo.write(0);
  } else {
    servo.write(180);
  }
  delay(15);
  server.send(200, "text/plain", "Servo moved");
  flashLed();
}

void handleRoomHumidity() {
  float roomHum = dht.readHumidity();
  if (isnan(roomHum)) {
    server.send(500, "text/plain", "No Data Available");
  } else {
    server.send(200, "text/plain", String(roomHum));
  }
  flashLed();
}

void handleRoomTemperature() {
  float roomTemp = dht.readTemperature(true);
  if (isnan(roomTemp)) {
    server.send(500, "text/plain", "No Data Available");
  } else {
    server.send(200, "text/plain", String(roomTemp));
  }
  flashLed();
}

void setup() {
  delay(3000);
  Serial.println("Starting Setup...");
  dht.begin();
  servo.attach(33);
  pinMode(ledPin, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  digitalWrite(ledPin, LOW);
  digitalWrite(ledPin2, LOW);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Attempting WiFi Connection...");
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    if (MDNS.begin("temp")) {
      Serial.println("MDNS responder started");
      led200();
    }

    server.on("/", handleRoot);
    server.on("/moveServoOpen", handleMoveServoOpen);
    server.on("/moveServoClose", handleMoveServoClose);
    server.on("/Temperature", handleRoomTemperature);
    server.on("/Humidity", handleRoomHumidity);
    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("HTTP Server has started");
    led200();
  } else {
    Serial.println("Failed to connect to WiFi");
  }
}

void loop() {
  server.handleClient();
  delay(1000);
}
