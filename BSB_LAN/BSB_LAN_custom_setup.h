// Add custom code for setup function here which will be included at the end of the function

// PIN_RELAY is also listed in MQTT_GPIO_PINS: run the MQTT GPIO init (one-time, sets pins to INPUT) now,
// so it doesn't later override pinMode(PIN_RELAY, OUTPUT) below.
#if defined(MQTT_GPIO_PINS)
mqtt_gpio_init();
#endif

// Initialize Custom Configuration Pins
pinMode(PIN_INPUT_1, INPUT);
pinMode(PIN_INPUT_2, INPUT);
pinMode(PIN_INPUT_3, INPUT);
pinMode(PIN_RELAY, OUTPUT);
// Configure Pulse Pin
pinMode(PIN_PULSE_INPUT, INPUT);

digitalWrite(PIN_RELAY, LOW); // Start with physical relay safely turned OFF

// Attach Pulse Interrupt
attachInterrupt(digitalPinToInterrupt(PIN_PULSE_INPUT), handlePulse, RISING);