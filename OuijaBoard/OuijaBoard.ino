// Inspired from https://randomnerdtutorials.com/esp32-http-get-post-arduino/

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

const char *ssid = "Tropi_24";
const char *password = "coolcoolco";

//Your Domain name with URL path or IP address with path
const char *serverName = "http://192.168.1.91:5000/data";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 100;
bool silentMode = true;

String sensorReadings;
float sensorReadingsArr[3];

void setup()
{
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  if (!silentMode) {
    Serial.println("Connecting");
  }
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    if (!silentMode) {
      Serial.print(".");
    }
  }
  if (!silentMode) {
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
  
    Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
  }
}

void loop()
{
  if ((millis() - lastTime) > timerDelay)
  {
    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {
      sensorReadings = httpGETRequest(serverName);
      if (!silentMode) {
        Serial.println(sensorReadings);
      }
      JSONVar myObject = JSON.parse(sensorReadings);

      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined")
      {
        if (!silentMode) {
          Serial.println("Parsing input failed!");
        }
        return;
      }

      if (!silentMode) {
        Serial.print("JSON object = ");
        Serial.println(myObject);
      }

      // myObject.keys() can be used to get an array of all the keys in the object
      JSONVar keys = myObject.keys();

      for (int i = 0; i < keys.length(); i++)
      {
        JSONVar value = myObject[keys[i]];
        if (!silentMode) {
          Serial.print(keys[i]);
          Serial.print(" = ");
          Serial.println(value);
        }
        sensorReadingsArr[i] = double(value);
      }
      Serial.print(sensorReadingsArr[0]);
      Serial.print(",");
      Serial.print(sensorReadingsArr[1]);
      Serial.print(",");
      Serial.print(sensorReadingsArr[2]);
      Serial.println();
    }
    else
    {
      if (!silentMode) {
        Serial.println("WiFi Disconnected");
      }
    }
    lastTime = millis();
  }
}

String httpGETRequest(const char *serverName)
{
  HTTPClient http;

  // Your IP address with path or Domain name with URL path
  http.begin(serverName);

  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode > 0)
  {
    if (!silentMode) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    payload = http.getString();
  }
  else
  {
    if (!silentMode) {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
  }
  // Free resources
  http.end();

  return payload;
}
