#include <Arduino.h>
#include <Servo.h>
#include <PCMmem.h>
#include "sound3.h"
#include "gotcha2-1.h"
#include "gotcha2-2.h"
#include "noise1.h"

#define START_BUTTON_PIN 12
#define RELEASE_BUTTON_PIN A0
#define INNER_LED_1_PIN A3
#define INNER_LED_2_PIN A4
#define INNER_LED_3_PIN A5
#define FILL_FIRST_LED_PIN 1
#define FILL_LAST_LED_PIN 10
#define SPEAKER_PIN 11
#define SERVO_1_PIN A1
#define SERVO_2_PIN A2

#define SERVO_1_CLOSE_ANGLE 93
#define SERVO_1_OPEN_ANGLE 60
#define SERVO_2_CLOSE_ANGLE 93
#define SERVO_2_OPEN_ANGLE 126

#define TRAPPING_ANIMATION_DURATION_MIN 8000
#define TRAPPING_ANIMATION_DURATION_MAX 25000
#define TRAPPING_PROGRESS_LED_ON_DELAY 50
#define TRAPPING_PROGRESS_END_DELAY 50
#define TRAPPING_PROGRESS_DARK_DELAY 50
#define INNER_LEDS_ANIM_ITERATION1_MIN 600
#define INNER_LEDS_ANIM_ITERATION1_MAX 2500
#define INNER_LEDS_ANIM_ITERATION2_MIN 600
#define INNER_LEDS_ANIM_ITERATION2_MAX 2500
#define FILLED_INDICATION_DURATION 30000


enum State {
    READY,
    TRAPPING,
    FILLED,
    RELEASING,
};

enum DoorsState {
    OPEN,
    CLOSE,
};

enum State state = READY;
enum DoorsState doorsState = CLOSE;
Servo servo_1;
Servo servo_2;
unsigned long mainTimer = 0;

void closeDoors();

void openDoors();

void playTrappingLedAnimation(long duration);

__attribute__((unused)) void setup() {
    for (int pin = FILL_FIRST_LED_PIN; pin <= FILL_LAST_LED_PIN; ++pin)
        pinMode(pin, OUTPUT);
    pinMode(INNER_LED_1_PIN, OUTPUT);
    pinMode(INNER_LED_2_PIN, OUTPUT);
    pinMode(INNER_LED_3_PIN, OUTPUT);
    setSpeakerPin(SPEAKER_PIN);
    // timings bugfix
    const unsigned char emptySound[] = {0x00, 0x00, 0x00, 0x00};
    startPlayback(emptySound, sizeof(emptySound), true);
    altDelay(20);
    stopPlayback();
    pinMode(START_BUTTON_PIN, INPUT_PULLUP);
    pinMode(RELEASE_BUTTON_PIN, INPUT_PULLUP);
//    Serial.begin(9600);
    randomSeed(analogRead(0));
}

__attribute__((unused)) void loop() {
    switch (state) {
        case READY: {
            if (digitalRead(START_BUTTON_PIN) == LOW) {
                if (doorsState == CLOSE) {
                    state = TRAPPING;
                }
            }
            if (digitalRead(RELEASE_BUTTON_PIN) == LOW) {
                if (doorsState == OPEN) {
                    closeDoors();
                    doorsState = CLOSE;
                } else {
                    state = RELEASING;
                }
            }
            break;
        }

        case TRAPPING: {
            openDoors();
            startPlayback(sound3_wav, sizeof(sound3_wav), true);
            playTrappingLedAnimation(
                    random(TRAPPING_ANIMATION_DURATION_MIN, TRAPPING_ANIMATION_DURATION_MAX));
            stopPlayback();
            closeDoors();
            for (int pin = FILL_FIRST_LED_PIN; pin <= FILL_LAST_LED_PIN; ++pin) {
                digitalWrite(pin, HIGH);
                altDelay(100);
            }
            startPlayback(gotcha2_1_wav, sizeof(gotcha2_1_wav), false);
            altDelay(500);
            startPlayback(gotcha2_2_wav, sizeof(gotcha2_2_wav), false);
            altDelay(500);
            stopPlayback();
            altDelay(50);
            mainTimer = altMillis();
            state = FILLED;
            break;
        }

        case FILLED: {
            if (digitalRead(RELEASE_BUTTON_PIN) == LOW) {
                for (int pin = FILL_FIRST_LED_PIN; pin <= FILL_LAST_LED_PIN; ++pin)
                    digitalWrite(pin, LOW);
                state = RELEASING;
            } else if (altMillis() - mainTimer < FILLED_INDICATION_DURATION) {
                for (int pin = FILL_FIRST_LED_PIN; pin <= FILL_LAST_LED_PIN; ++pin)
                    digitalWrite(pin, HIGH);
                altDelay(100);
                for (int pin = FILL_FIRST_LED_PIN; pin <= FILL_LAST_LED_PIN; ++pin)
                    digitalWrite(pin, LOW);
                altDelay(100);
            } else if (digitalRead(START_BUTTON_PIN) == LOW) {
                mainTimer = altMillis();
            } else {
                for (int pin = FILL_FIRST_LED_PIN; pin <= FILL_LAST_LED_PIN; ++pin)
                    digitalWrite(pin, LOW);
            }
            break;
        }

        case RELEASING: {
            if (doorsState == CLOSE) {
                openDoors();
                doorsState = OPEN;
                startPlayback(noise1_wav, sizeof(noise1_wav), true);
                altDelay(1000);
                stopPlayback();
                state = READY;
            } else {
                closeDoors();
                doorsState = CLOSE;
                state = READY;
            }
            break;
        }
    }
}

void playTrappingLedAnimation(long duration) {
    mainTimer = altMillis();

    unsigned long progressTimer = altMillis();
    byte progressLoopIteration = 0;
    long progressLoopIterationDelay = 0;

    unsigned long innerLedsAnimTimer = altMillis();
    byte innerLedsAnimIteration = 0;
    long innerLedsAnimIterationDelay = 0;

    while (altMillis() - mainTimer < duration) {
        // progress bar iteration
        if (altMillis() - progressTimer >= progressLoopIterationDelay) {
            switch (progressLoopIteration) {
                case 0 ... 4:
                    digitalWrite(FILL_FIRST_LED_PIN + progressLoopIteration, HIGH);
                    digitalWrite(FILL_LAST_LED_PIN - progressLoopIteration, HIGH);
                    progressTimer = altMillis();
                    progressLoopIterationDelay = TRAPPING_PROGRESS_END_DELAY;
                    progressLoopIteration++;
                    break;
                case 5:
                    progressTimer = altMillis();
                    progressLoopIterationDelay = TRAPPING_PROGRESS_DARK_DELAY;
                    progressLoopIteration++;
                    break;
                default:
                    for (int pin = FILL_FIRST_LED_PIN; pin <= FILL_LAST_LED_PIN; ++pin) {
                        digitalWrite(pin, LOW);
                    }
                    progressTimer = altMillis();
                    progressLoopIterationDelay = TRAPPING_PROGRESS_LED_ON_DELAY;
                    progressLoopIteration = 0;
                    break;
            }
        }

        // inner LEDs anim iteration
        if (altMillis() - innerLedsAnimTimer >= innerLedsAnimIterationDelay) {
            switch (innerLedsAnimIteration) {
                case 0:
                    innerLedsAnimTimer = altMillis();
                    innerLedsAnimIterationDelay = random(INNER_LEDS_ANIM_ITERATION2_MIN,
                                                         INNER_LEDS_ANIM_ITERATION2_MAX);
                    innerLedsAnimIteration++;
                    break;
                default:
                    innerLedsAnimTimer = altMillis();
                    innerLedsAnimIterationDelay = random(INNER_LEDS_ANIM_ITERATION1_MIN,
                                                         INNER_LEDS_ANIM_ITERATION1_MAX);
                    innerLedsAnimIteration = 0;
                    break;
            }
        }
        switch (innerLedsAnimIteration) {
            case 0: {
                byte led = altMillis() / 150 % 3;
                if (led == 0) {
                    digitalWrite(INNER_LED_1_PIN, HIGH);
                    digitalWrite(INNER_LED_2_PIN, LOW);
                    digitalWrite(INNER_LED_3_PIN, LOW);
                }
                if (led == 1) {
                    digitalWrite(INNER_LED_1_PIN, LOW);
                    digitalWrite(INNER_LED_2_PIN, HIGH);
                    digitalWrite(INNER_LED_3_PIN, LOW);
                }
                if (led == 2) {
                    digitalWrite(INNER_LED_1_PIN, LOW);
                    digitalWrite(INNER_LED_2_PIN, LOW);
                    digitalWrite(INNER_LED_3_PIN, HIGH);
                }
                break;
            }
            default:
                if (altMillis() / 50 % 2 == 0) {
                    digitalWrite(INNER_LED_1_PIN, HIGH);
                    digitalWrite(INNER_LED_2_PIN, HIGH);
                    digitalWrite(INNER_LED_3_PIN, HIGH);
                } else {
                    digitalWrite(INNER_LED_1_PIN, LOW);
                    digitalWrite(INNER_LED_2_PIN, LOW);
                    digitalWrite(INNER_LED_3_PIN, LOW);
                }
                break;
        }
    }
    digitalWrite(INNER_LED_1_PIN, LOW);
    digitalWrite(INNER_LED_2_PIN, LOW);
    digitalWrite(INNER_LED_3_PIN, LOW);
    for (int pin = FILL_FIRST_LED_PIN; pin <= FILL_LAST_LED_PIN; ++pin) {
        digitalWrite(pin, LOW);
    }
}

void openDoors() {
    servo_1.attach(SERVO_1_PIN);
    servo_2.attach(SERVO_2_PIN);
    altDelay(25);
    servo_1.write(SERVO_1_OPEN_ANGLE);
    servo_2.write(SERVO_2_OPEN_ANGLE);
    altDelay(200);
    servo_1.detach();
    servo_2.detach();
}

void closeDoors() {
    servo_1.attach(SERVO_1_PIN);
    servo_2.attach(SERVO_2_PIN);
    altDelay(25);
    servo_1.write(SERVO_1_CLOSE_ANGLE);
    servo_2.write(SERVO_2_CLOSE_ANGLE);
    altDelay(300);
    servo_1.detach();
    servo_2.detach();
}


