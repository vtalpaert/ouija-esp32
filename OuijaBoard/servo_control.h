#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include <Preferences.h>
#include "vocabulary.h"

// --- Configuration (loaded from Preferences, passed in at init) ---

struct ServoConfig {
    float         speed;            // PWM units per second
    int           threshold;        // min PWM delta before reacting (compass mode)
    int           letterPauseMs;    // pause duration after each token
    int           spacePauseMs;     // pause duration for space token
    float         compassStart;     // compass heading (deg) that maps to pwmMin
    float         compassEnd;       // compass heading (deg) that maps to pwmMax
    unsigned long idleTimeoutMs;    // ms of inactivity before detaching and saving position
};

// --- Internal state ---

enum ServoMode { MODE_IDLE, MODE_COMPASS, MODE_TEXT };

#define QUEUE_CAPACITY 256

struct QueueEntry {
    int           pwm;     // target PWM; -1 means pause only (space)
    unsigned long pauseMs;
};

static Servo         _servo;
static bool          _servoAttached  = false;
static bool          _idleSaved      = false;
static int           _pwmMin, _pwmMax;
static float         _currentPWM;
static int           _targetPWM;
static ServoMode     _mode = MODE_IDLE;
static ServoConfig   _cfg;
static unsigned long _lastUpdateMs  = 0;
static unsigned long _lastActiveMs  = 0;

static QueueEntry    _queue[QUEUE_CAPACITY];
static int           _qHead = 0, _qTail = 0;
static unsigned long _pauseEndMs = 0;

static inline void _servoEnsureAttached() {
    if (!_servoAttached) {
        _servo.setPeriodHertz(50);
        _servo.attach(SERVO_PIN, SERVO_HW_MIN, SERVO_HW_MAX);
        _servo.writeMicroseconds((int)_currentPWM);
        _servoAttached = true;
    }
}

static inline void _servoSaveAndDetach() {
    Preferences p;
    p.begin("ouija", false);
    p.putInt("lastPwm", (int)_currentPWM);
    p.end();
    _servo.detach();
    _servoAttached = false;
    _idleSaved     = true;
}

static inline bool _qEmpty()  { return _qHead == _qTail; }
static inline void _qClear()  { _qHead = _qTail = 0; _pauseEndMs = 0; }
static inline void _qPush(int pwm, unsigned long pauseMs) {
    int next = (_qTail + 1) % QUEUE_CAPACITY;
    if (next != _qHead) {
        _queue[_qTail] = {pwm, pauseMs};
        _qTail = next;
    }
}
static inline QueueEntry& _qFront() { return _queue[_qHead]; }
static inline void        _qPop()   { _qHead = (_qHead + 1) % QUEUE_CAPACITY; }

// --- Public API ---

void servoControlInit(const ServoConfig& cfg, int pwmMin, int pwmMax) {
    _cfg    = cfg;
    _pwmMin = pwmMin;
    _pwmMax = pwmMax;

    // Restore last known position from Preferences, fallback to midpoint.
    Preferences p;
    p.begin("ouija", true);
    int savedPwm = p.getInt("lastPwm", -1);
    p.end();
    _currentPWM = (savedPwm >= pwmMin && savedPwm <= pwmMax)
                  ? (float)savedPwm
                  : (pwmMin + pwmMax) / 2.0f;

    _targetPWM    = (int)_currentPWM;
    _lastUpdateMs = millis();
    _lastActiveMs = millis();
    _idleSaved    = false;

    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    _servo.setPeriodHertz(50);
    _servo.attach(SERVO_PIN, SERVO_HW_MIN, SERVO_HW_MAX);
    _servoAttached = true;
    _servo.writeMicroseconds((int)_currentPWM);
}

void servoUpdateConfig(const ServoConfig& cfg) {
    _cfg = cfg;
}

// Switch to compass mode (clears any pending text queue).
void servoSetCompassMode() {
    _qClear();
    _mode = MODE_COMPASS;
}

void servoSetIdle() {
    _mode = MODE_IDLE;
}

// Move the servo to a PWM value at the configured speed.
// Useful for calibration / vocabulary mapping.
void servoJog(int pwm) {
    pwm = constrain(pwm, SERVO_HW_MIN, SERVO_HW_MAX);
    _qClear();
    _qPush(pwm, 0);
    _mode = MODE_TEXT;
}

int servoGetCurrentPwm() { return (int)_currentPWM; }
int servoGetPwmMin()     { return _pwmMin; }
int servoGetPwmMax()     { return _pwmMax; }

// Called when a new compass alpha arrives (degrees, 0-360).
void servoSetCompassTarget(float alpha) {
    if (_mode != MODE_COMPASS) return;

    float arcLength = fmod(_cfg.compassEnd - _cfg.compassStart + 360.0f, 360.0f);
    if (arcLength < 1.0f) return;

    float offset = fmod(alpha - _cfg.compassStart + 360.0f, 360.0f);
    // If the offset went more than halfway around the remaining arc it means
    // alpha is just before compassStart (wrong side), so treat it as negative
    // so it clamps to pwmMin rather than erroneously snapping to pwmMax.
    if (offset > (arcLength + 360.0f) / 2.0f) offset -= 360.0f;
    float t      = constrain(offset / arcLength, 0.0f, 1.0f);
    int   target = (int)(_pwmMin + t * (_pwmMax - _pwmMin));

    if (abs(target - _targetPWM) > _cfg.threshold) {
        _targetPWM = target;
    }
}

// Tokenize text and push movements onto the queue.
// Switches to text mode and clears any previous queue.
void servoSpellText(const String& text) {
    _qClear();
    _mode = MODE_TEXT;

    int i   = 0;
    int len = text.length();
    while (i < len) {
        bool matched = false;
        for (int t = 0; t < VOCAB_SIZE; t++) {
            int wordLen = strlen(VOCABULARY[t].word);
            if (i + wordLen > len) continue;
            if (strncasecmp(text.c_str() + i, VOCABULARY[t].word, wordLen) == 0) {
                unsigned long pause = (VOCABULARY[t].pwm < 0)
                    ? _cfg.spacePauseMs
                    : _cfg.letterPauseMs;
                _qPush(VOCABULARY[t].pwm, pause);
                i += wordLen;
                matched = true;
                break;
            }
        }
        if (!matched) i++;
    }
}

// Call this in loop().
void servoUpdate() {
    unsigned long now     = millis();
    float         elapsed = (now - _lastUpdateMs) / 1000.0f;
    _lastUpdateMs = now;

    bool moved = false;

    if (_mode == MODE_COMPASS) {
        float diff = (float)_targetPWM - _currentPWM;
        float step = _cfg.speed * elapsed;
        if (fabsf(diff) <= step) {
            _currentPWM = (float)_targetPWM;
        } else {
            _currentPWM += (diff > 0 ? step : -step);
            moved = true;
        }
        _servoEnsureAttached();
        _servo.writeMicroseconds((int)_currentPWM);

    } else if (_mode == MODE_TEXT) {
        if (_qEmpty()) {
            _mode = MODE_IDLE;
        } else {
            QueueEntry& entry = _qFront();

            if (entry.pwm < 0) {
                // Space: pause without moving.
                if (_pauseEndMs == 0) _pauseEndMs = now + entry.pauseMs;
                if (now >= _pauseEndMs) { _qPop(); _pauseEndMs = 0; }
            } else {
                float diff = (float)entry.pwm - _currentPWM;
                float step = _cfg.speed * elapsed;

                if (fabsf(diff) <= step) {
                    // Arrived at target.
                    _currentPWM = (float)entry.pwm;
                    _servoEnsureAttached();
                    _servo.writeMicroseconds((int)_currentPWM);
                    if (_pauseEndMs == 0) _pauseEndMs = now + entry.pauseMs;
                    if (now >= _pauseEndMs) { _qPop(); _pauseEndMs = 0; }
                } else {
                    _currentPWM += (diff > 0 ? step : -step);
                    moved = true;
                    _servoEnsureAttached();
                    _servo.writeMicroseconds((int)_currentPWM);
                }
            }
        }
    }

    if (moved) {
        _lastActiveMs = now;
        _idleSaved    = false;
    } else if (!_idleSaved && (now - _lastActiveMs >= _cfg.idleTimeoutMs)) {
        _servoSaveAndDetach();
    }
}
