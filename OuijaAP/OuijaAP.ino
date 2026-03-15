// Ouija Magic Board
// 

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <ESP32Servo.h>

float deviceOrientation = 180;
float deltaAngle = 2; // [deg/iteration]
float angleMin = 90;
float angleMax = 270;
// Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33
int servoPin = 16;
int servoMin = 544;
int servoMax = 2160;
// using default min/max of 1000us and 2000us
// different servos may require different min/max settings
// for an accurate 0 to 180 sweep
int servoPos = (servoMin + servoMax) / 2;

const char* ssid = "ScarySpookyAP";
const char* password = "Skeleton";
const char* PARAM_MESSAGE = "alpha"; // keyword for the POST request

AsyncWebServer server(80);

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Ouija</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     background-color: black;
     color: white;
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
  </style>
</head>
<body>
  <h2>Ouija whisperer</h2>
  <p>
    <span id="alpha"></span>
  </p>
</body>
<script>
var alpha = 0;
if (window.DeviceOrientationEvent) {
  window.addEventListener("deviceorientation", function (event) {
    alpha = Math.round(event.alpha);
  });
} else {
  alert("Sorry, your browser doesn't support Device Orientation");
}
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  var params = 'alpha=' + alpha;
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("alpha").innerHTML = this.responseText;
    }
  };
  xhttp.open("POST", "/post", true);
  xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhttp.send(params);
}, 100 ) ;

</script>
</html>)rawliteral";

// internals
float accountedOrientation = deviceOrientation;
Servo myservo;  // create servo object to control a servo
String message;

void setup() {
  // Allow allocation of all timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50); // standard 50 hz servo
  myservo.attach(servoPin, servoMin, servoMax); // attaches the servo on pin 18 to the servo object
  myservo.write(servoPos);

  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
  });

  // Send a POST request to <IP>/post with a form field message set to <message>
  server.on("/post", HTTP_POST, [](AsyncWebServerRequest * request) {
    if (request->hasParam(PARAM_MESSAGE, true)) {
      message = request->getParam(PARAM_MESSAGE, true)->value();
      deviceOrientation = message.toInt();
      Serial.print(deviceOrientation);
      Serial.print(", ");
      if (deviceOrientation < angleMin) {
        deviceOrientation = angleMin;
      } else if (deviceOrientation > angleMax) {
        deviceOrientation = angleMax;
      }
      Serial.print(deviceOrientation);
      Serial.print(", ");
      if (accountedOrientation < deviceOrientation - deltaAngle) {
        accountedOrientation = accountedOrientation + deltaAngle;
      } else if (accountedOrientation > deviceOrientation + deltaAngle) {
        accountedOrientation = accountedOrientation - deltaAngle;
      } else {
        accountedOrientation = deviceOrientation;
      }
      servoPos = map(accountedOrientation, angleMin, angleMax, servoMin, servoMax);
      myservo.write(servoPos);
      
      Serial.print(deltaAngle);
      Serial.print(", ");
      Serial.print(accountedOrientation);
      Serial.print(", ");
      Serial.println(servoPos);
    } else {
      message = "Browser is sending incorrect POST request";
    }
    request->send(200, "text/plain", message);
  });

  server.onNotFound(notFound);

  server.begin();
}

void loop() {
}
