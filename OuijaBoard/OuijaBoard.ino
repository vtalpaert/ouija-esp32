#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "vocabulary.h"
#include "servo_control.h"
#include "web_pages.h"

// AP credentials (fallback when no WiFi is configured)
#define AP_SSID     "ScarySpookyAP"
#define AP_PASSWORD "Skeleton"

// Default config values (used on first boot)
#define DEFAULT_SPEED          500.0f
#define DEFAULT_THRESHOLD      10
#define DEFAULT_COMPASS_START  0.0f
#define DEFAULT_COMPASS_END    180.0f
#define DEFAULT_LETTER_PAUSE   800
#define DEFAULT_SPACE_PAUSE    1000

// ---------------------------------------------------------------------------

Preferences       prefs;
AsyncWebServer    server(80);
ServoConfig       cfg;

// ---------------------------------------------------------------------------
// Preferences helpers
// ---------------------------------------------------------------------------

void loadConfig() {
    prefs.begin("ouija", true);
    cfg.speed         = prefs.getFloat("speed",        DEFAULT_SPEED);
    cfg.threshold     = prefs.getInt("threshold",      DEFAULT_THRESHOLD);
    cfg.compassStart  = prefs.getFloat("compassStart", DEFAULT_COMPASS_START);
    cfg.compassEnd    = prefs.getFloat("compassEnd",   DEFAULT_COMPASS_END);
    cfg.letterPauseMs = prefs.getInt("letterPause",    DEFAULT_LETTER_PAUSE);
    cfg.spacePauseMs  = prefs.getInt("spacePause",     DEFAULT_SPACE_PAUSE);
    prefs.end();
}

void saveConfig(const ServoConfig& c) {
    prefs.begin("ouija", false);
    prefs.putFloat("speed",        c.speed);
    prefs.putInt("threshold",      c.threshold);
    prefs.putFloat("compassStart", c.compassStart);
    prefs.putFloat("compassEnd",   c.compassEnd);
    prefs.putInt("letterPause",    c.letterPauseMs);
    prefs.putInt("spacePause",     c.spacePauseMs);
    prefs.end();
}

// ---------------------------------------------------------------------------
// Template processor for the config page
// ---------------------------------------------------------------------------

String configTemplate(const String& var) {
    if (var == "SPEED")         return String(cfg.speed, 0);
    if (var == "THRESHOLD")     return String(cfg.threshold);
    if (var == "COMPASS_START") return String(cfg.compassStart, 0);
    if (var == "COMPASS_END")   return String(cfg.compassEnd, 0);
    if (var == "LETTER_PAUSE")  return String(cfg.letterPauseMs);
    if (var == "SPACE_PAUSE")   return String(cfg.spacePauseMs);
    return "";
}

// ---------------------------------------------------------------------------
// Route registration
// ---------------------------------------------------------------------------

void registerFullRoutes() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html", HOME_HTML);
    });

    server.on("/spell", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (req->hasParam("text", true)) {
            servoSpellText(req->getParam("text", true)->value());
        }
        req->send_P(200, "text/html", SAVED_HTML);
    });

    server.on("/spell", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (req->hasParam("text")) {
            servoSpellText(req->getParam("text")->value());
        }
        req->send(200, "text/plain", "OK");
    });

    server.on("/compass", HTTP_GET, [](AsyncWebServerRequest* req) {
        servoSetCompassMode();
        req->send_P(200, "text/html", COMPASS_HTML);
    });

    server.on("/compass", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (req->hasParam("alpha", true)) {
            float alpha = req->getParam("alpha", true)->value().toFloat();
            servoSetCompassTarget(alpha);
        }
        req->send(200, "text/plain", "OK");
    });

    server.on("/config", HTTP_GET, [](AsyncWebServerRequest* req) {
        String page = FPSTR(CONFIG_HTML);
        page.replace("%SPEED%",         String(cfg.speed, 0));
        page.replace("%THRESHOLD%",     String(cfg.threshold));
        page.replace("%COMPASS_START%", String(cfg.compassStart, 0));
        page.replace("%COMPASS_END%",   String(cfg.compassEnd, 0));
        page.replace("%LETTER_PAUSE%",  String(cfg.letterPauseMs));
        page.replace("%SPACE_PAUSE%",   String(cfg.spacePauseMs));
        req->send(200, "text/html", page);
    });

    server.on("/jog", HTTP_GET, [](AsyncWebServerRequest* req) {
        // After a POST redirect, ?pwm= carries the target so the form shows
        // the intended position rather than the in-transit current position.
        int displayPwm = req->hasParam("pwm")
            ? req->getParam("pwm")->value().toInt()
            : servoGetCurrentPwm();
        String page = FPSTR(JOG_HTML);
        page.replace("%PWM_MIN%",     String(SERVO_HW_MIN));
        page.replace("%PWM_MAX%",     String(SERVO_HW_MAX));
        page.replace("%PWM_CURRENT%", String(displayPwm));
        req->send(200, "text/html", page);
    });

    server.on("/jog", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (req->hasParam("pwm", true)) {
            int pwm = req->getParam("pwm", true)->value().toInt();
            servoJog(pwm);
            req->redirect("/jog?pwm=" + String(pwm));
        } else {
            req->redirect("/jog");
        }
    });

    server.on("/config", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (req->hasParam("speed",        true)) cfg.speed         = req->getParam("speed",        true)->value().toFloat();
        if (req->hasParam("threshold",    true)) cfg.threshold     = req->getParam("threshold",    true)->value().toInt();
        if (req->hasParam("compassStart", true)) cfg.compassStart  = req->getParam("compassStart", true)->value().toFloat();
        if (req->hasParam("compassEnd",   true)) cfg.compassEnd    = req->getParam("compassEnd",   true)->value().toFloat();
        if (req->hasParam("letterPause",  true)) cfg.letterPauseMs = req->getParam("letterPause",  true)->value().toInt();
        if (req->hasParam("spacePause",   true)) cfg.spacePauseMs  = req->getParam("spacePause",   true)->value().toInt();
        saveConfig(cfg);
        servoUpdateConfig(cfg);
        req->send_P(200, "text/html", SAVED_HTML);
    });
}

void registerApRoutes() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html", WIFI_SETUP_HTML);
    });

    server.on("/wifi-setup", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html", WIFI_SETUP_HTML);
    });

    server.on("/wifi-setup", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (req->hasParam("ssid", true) && req->hasParam("password", true)) {
            String ssid     = req->getParam("ssid",     true)->value();
            String password = req->getParam("password", true)->value();
            prefs.begin("wifi", false);
            prefs.putString("ssid",     ssid);
            prefs.putString("password", password);
            prefs.end();
        }
        req->send_P(200, "text/html", SAVED_HTML);
        delay(500);
        ESP.restart();
    });
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);

    loadConfig();

    int pwmMin, pwmMax;
    computePwmBounds(pwmMin, pwmMax);
    servoControlInit(cfg, pwmMin, pwmMax);

    prefs.begin("wifi", true);
    String storedSsid     = prefs.getString("ssid",     "");
    String storedPassword = prefs.getString("password", "");
    prefs.end();

    bool connected = false;
    if (storedSsid.length() > 0) {
        Serial.print("Connecting to WiFi: ");
        Serial.println(storedSsid);
        WiFi.begin(storedSsid.c_str(), storedPassword.c_str());
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
            delay(200);
        }
        connected = (WiFi.status() == WL_CONNECTED);
    }

    if (connected) {
        Serial.print("Connected. IP: ");
        Serial.println(WiFi.localIP());
        registerFullRoutes();
    } else {
        Serial.println("WiFi failed. Starting AP: " AP_SSID);
        WiFi.softAP(AP_SSID, AP_PASSWORD);
        Serial.print("AP IP: ");
        Serial.println(WiFi.softAPIP());
        registerApRoutes();
    }

    server.onNotFound([](AsyncWebServerRequest* req) {
        req->send(404, "text/plain", "Not found");
    });

    server.begin();
    Serial.println("Server started.");
}

// ---------------------------------------------------------------------------
// Loop
// ---------------------------------------------------------------------------

void loop() {
    servoUpdate();
}
