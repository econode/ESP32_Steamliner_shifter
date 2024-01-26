//
//  Sample defaults file copy this file to defaults.h
//  defaults.h is excluded from GIT
//

#ifndef DEFAULTS_H
#define DEFAULTS_H

// WiFi values
#define WIFI_SSID "AutoShifter"
// Comment out WIFI Password for no password IE open
// #define WIFI_PASSWORD "123456789"

// PIN Definitions
#define PIN_BUTTON_UP 12
#define PIN_BUTTON_DOWN 13
#define PIN_SHIFTER_SERVO 14
// How many samples the debounce shift register should compare
#define BUTTON_SAMPLES 4
// Set PIN_NEUTRAL_LED to 0 if no LED
#define PIN_NEUTRAL_LED 15

// Hardware watchdog timeout in seconds
#define WDT_TIMEOUT 5

// gearbox defaults
// How long a button needs to be pressed to be counted as an action in ms
#define DEFAULT_MINIMUM_BUTTON_PRESS_TIME 80
#define DEFAULT_UpDegrees 200
#define DEFAULT_NeutralDegrees 130
#define DEFAULT_MidPointDegrees 95
#define DEFAULT_DownDegrees 10
// Hold delay and press time in milli seconds
// How long to hold the gear shifter in position before returning to mid point
#define DEFAULT_HoldDelay 200
// How long to  hold the up button when in first for neutral selection
#define DEFAULT_NeutralPressTime 750


// Servo parameters
// This needs to match the servo
// For SG90 Servo 50Hz Min pulse 500 MAx pulse 2400
#define SERVO_FREQUENCY_HZ 50
#define SERVO_MINIMUM_PULSE_WIDTH 500
#define SERVO_MAXIMUM_PULSE_WIDTH 2400
// Delay in milliseconds to allow the servo to move into position
#define SERVO_SETTLE_TIME 100
// Disable the servo afer X ms, 0 = disable idle timer
#define DEFAULT_SERVO_IDLE_TIMER 3000

#endif