#pragma once
#include <string.h>

// Hardware PWM limits for your specific servo (microseconds).
// Adjust these to match your servo's datasheet or calibration.
#define SERVO_HW_MIN 520
#define SERVO_HW_MAX 2200

// Servo GPIO pin
#define SERVO_PIN 16

struct Token {
    const char* word;
    int pwm; // servo pulse width in microseconds; use -1 for pause-only (space)
};

// Vocabulary: map words/letters to servo PWM positions.
//
// Rules:
//   - Entries MUST be sorted longest-first. The tokenizer is greedy and picks
//     the first match, so "YES" must appear before "Y".
//   - PWM values must be within [SERVO_HW_MIN, SERVO_HW_MAX].
//   - Use pwm = -1 for space: the planchette pauses without moving.
//   - Text matching is case-insensitive.
//
// Placeholder values below -- replace PWM numbers with your physical calibration.
static const Token VOCABULARY[] = {
    // Multi-character tokens (longest first)
    //{"YES",  ???},
    //{"NO",   ???},
    // Space: pause only, no movement
    {"%20",  -1},
    {" ",    -1},
    // Single letters A-Z (A=2160, Z=544, evenly spaced ~65 us/step)
    {"A",  2170}, // <-- anchor
    {"B",  2101},
    {"C",  2032},
    {"D",  1962},
    {"E",  1893},
    {"F",  1824},
    {"G",  1755}, // <-- anchor
    {"H",  1692},
    {"I",  1630},
    {"J",  1568},
    {"K",  1505}, // <-- anchor
    {"L",  1425},
    {"M",  1345},
    {"N",  1265}, // <-- anchor
    {"O",  1194},
    {"P",  1123},
    {"Q",  1052},
    {"R",   981},
    {"S",   910}, // <-- anchor
    {"T",   851},
    {"U",   792},
    {"V",   734},
    {"W",   675}, // <-- anchor
    {"X",   627},
    {"Y",   578},
    {"Z",   530}, // <-- anchor
};

static const int VOCAB_SIZE = sizeof(VOCABULARY) / sizeof(VOCABULARY[0]);

// Compute the min and max PWM values across the vocabulary (ignoring -1 entries).
// Called once at boot to determine the compass mapping range.
static void computePwmBounds(int& pwmMin, int& pwmMax) {
    pwmMin = SERVO_HW_MAX;
    pwmMax = SERVO_HW_MIN;
    for (int i = 0; i < VOCAB_SIZE; i++) {
        if (VOCABULARY[i].pwm < 0) continue;
        if (VOCABULARY[i].pwm < pwmMin) pwmMin = VOCABULARY[i].pwm;
        if (VOCABULARY[i].pwm > pwmMax) pwmMax = VOCABULARY[i].pwm;
    }
}
