// Add custom code for global functions here which will be included in the global section

unsigned long custom_timer_compare6 = 0;
unsigned long custom_timer_compare60 = 0;

// --- Custom Heating Control Pin Definitions ---
#define PIN_INPUT_1  18  // D18: Master Input Switch
#define PIN_INPUT_2  19  // D19: Ist floor input switch
#define PIN_INPUT_3  21  // D21: IInd floor input switch
#define PIN_RELAY     4  // D4: Relay Control Output

// --- Custom Pulse Counter Definitions ---
#define PIN_PULSE_INPUT 22  // D22: Connect your pulse source here

volatile unsigned long pulseCount = 0; // 'volatile' is mandatory for interrupt variables
unsigned long lastPulseTime = 0;        // For debouncing mechanical switches
const unsigned long debounceDelay = 50; // 50ms debounce window (adjust if needed)
unsigned long oldPulseCount = 0;
double currentLoadKW = 0.000;

// unsigned long lastPulsePrint = 0;
// const unsigned long pulsePrintInterval = 5000; // Print totals every 5 seconds
// unsigned long lastPulsePrintTime = 0;

// --- Interrupt Service Routine (ISR) ---
void IRAM_ATTR handlePulse() {
  unsigned long currentTime = millis();
  if (currentTime - lastPulseTime > debounceDelay) {
    pulseCount++;
    lastPulseTime = currentTime;
  }
}

// Keeps track of the last state globally to avoid flooding the BSB bus
static bool lastSystemState = false;

// History tracking for inputs only
static bool last_in1 = false;
static bool last_in2 = false;
static bool last_in3 = false;
static bool firstRunExecution = true;

// --- Local custom parameters 10100-10105 ---
// Values are produced by BSB_LAN_custom.h and served by query() from here instead of the heater bus,
// so they show up in the web UI and are published to MQTT with their proper name/unit.
#define CUSTOM_PARAM_FIRST 10100
#define CUSTOM_PARAM_COUNT 6
char customParamValues[CUSTOM_PARAM_COUNT][16] = { "0", "0", "0", "0", "0", "0" };