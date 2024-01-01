//
//  Sample defaults file copy this file to defaults.h
//  defaults.h is excluded from GIT
//

#ifndef DEFAULTS_H
#define DEFAULTS_H

// WiFi values
#define WIFI_SSID "AutoShifter"
#define WIFI_PASSWORD "123456789"

// PIN Definitions
#define PIN_BUTTON_UP 12
#define PIN_BUTTON_DOWN 13
#define PIN_SHIFTER_SERVO 14
#define BUTTON_DEBOUNCE_MS 50
// Set PIN_NEUTRAL_LED to 0 if no LED
#define PIN_NEUTRAL_LED 15

// Hardware watchdog timeout in seconds
#define WDT_TIMEOUT 5

// gearbox defaults
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

#endif