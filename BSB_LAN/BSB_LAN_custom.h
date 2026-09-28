/* 
 * This code is run at the end of each main loop and utilizes the main loop variables 
 * custom_timer (set each loop to millis()) and custom_timer_compare.
 * This short example prints a "Ping!" message every 60 seconds.
*/

// Store a value for a local custom parameter (10100-10105) and publish it to MQTT.
// query() serves the stored value (also used by the web UI), so the MQTT message gets the proper name/unit.
auto publishCustomParam = [](float line, const char *val) {
  strncpy(customParamValues[(int)line - CUSTOM_PARAM_FIRST], val, sizeof(customParamValues[0]) - 1);
  query(line);
  LogToMQTT(line);
};

// Run loop logic every 1000ms (1 second)
if (custom_timer > custom_timer_compare + 1000) {   
  custom_timer_compare = millis();

  // Read current physical switch states
  bool current_in1 = (digitalRead(PIN_INPUT_1) == HIGH);
  bool current_in2 = (digitalRead(PIN_INPUT_2) == HIGH);
  bool current_in3 = (digitalRead(PIN_INPUT_3) == HIGH);

  // Loging
  // Serial.print("Relay: ");
  // Serial.println(digitalRead(PIN_RELAY));
  // Serial.print("Input1: ");
  // Serial.println(digitalRead(PIN_INPUT_1));

  // If a physical input switch changes position
  if (current_in1 != last_in1 || current_in2 != last_in2 || current_in3 != last_in3 || firstRunExecution) {
    bool masterChanged = (current_in1 != last_in1) || firstRunExecution;

    // Print log
    Serial.print("Input1: ");
    Serial.println(current_in1);
    Serial.print("Input2: ");
    Serial.println(current_in2);
    Serial.print("Input3: ");
    Serial.println(current_in3);

    last_in1 = current_in1;
    last_in2 = current_in2;
    last_in3 = current_in3;
    firstRunExecution = false;

    // --- INSTANT MQTT PUSH FOR INPUTS ---
    publishCustomParam(10101, current_in1 ? "1" : "0");
    publishCustomParam(10102, current_in2 ? "1" : "0");
    publishCustomParam(10103, current_in3 ? "1" : "0");


    // Commands are sent only on a switch change; in between, HA can override parameter 700.
    //  - master changed: follow the master switch (ON -> Automatic, OFF -> Protection)
    //  - floor changed:  only while master is ON: Automatic if any floor requests heat, else Protection
    //                    (ignored while master is OFF)
    bool targetHeatingState = masterChanged ? current_in1 : (current_in2 || current_in3);

    // Transmit state data directly to the heating controller parameter 700
    if (!masterChanged && !current_in1) {
      printFmtToDebug("Automation: Floor switch changed while master is OFF - ignored.\r\n");
    } else if (targetHeatingState) {
      uint32_t rc = set(700, (char*)"1", 1); // 1 = Automatic
      printFmtToDebug("Automation: Boiler set to Automatic Mode. Result: %lu\r\n", rc);
      digitalWrite(PIN_RELAY, HIGH);
    } else {
      uint32_t rc = set(700, (char*)"0", 1); // 0 = Protection (Off, frost protection only)
      printFmtToDebug("Automation: Boiler set to Protection Mode. Result: %lu\r\n", rc);
      digitalWrite(PIN_RELAY, LOW);
    }
  }
}

// Run loop logic every 60000ms (60 second)
if (custom_timer > custom_timer_compare60+60000) {    // every 60 seconds  
  double timeElapsedSeconds = (custom_timer - custom_timer_compare60) / 1000.0;
  custom_timer_compare60 = millis();

  // 1. Safely snapshot the volatile total pulse count
  noInterrupts();
  unsigned long totalPulsesTracked = pulseCount;
  interrupts();

  // 2. Calculate pulses that happened in this exact 60-second window
  unsigned long pulsesInWindow = totalPulsesTracked - oldPulseCount;
  // Convert our unsigned long pulse counter into a text string
  char pulsesInWindowStrPayload[16];
  ultoa(pulsesInWindow, pulsesInWindowStrPayload, 10); // 10 means base-10 decimal format

  publishCustomParam(10104, pulsesInWindowStrPayload);

  oldPulseCount = totalPulsesTracked;

  // 3. Math: Calculate Instantaneous Load (kW)
  if (pulsesInWindow > 0 && timeElapsedSeconds > 0) {
    // Math logic: (Pulses / Seconds) * (3600 / 1600)
    // Over an exact 60-second window, this breaks down to: Pulses * 0.0375
    currentLoadKW = (pulsesInWindow / timeElapsedSeconds) * (3600.0 / 1600.0);
    
  } else {
    currentLoadKW = 0.000; 
  }

  // 4. Print out the clean load metric
  Serial.print("[ENERGY 60s] Current Load: ");
  Serial.print(currentLoadKW, 4); // 4 decimals perfectly handles the 0.0375 increments
  Serial.println(" kW");

  Serial.print("Total Pulses Captured: ");
  Serial.println(totalPulsesTracked);
  // Convert our unsigned long pulse counter into a text string
  char pulsesStrPayload[16];
  ultoa(totalPulsesTracked, pulsesStrPayload, 10); // 10 means base-10 decimal format

  publishCustomParam(10105, pulsesStrPayload);

  // 5. NATIVE MQTT PUSH
  // Convert our double variable into a text string with 4 decimal places
  char mqttStrPayload[16];
  dtostrf(currentLoadKW, 1, 4, mqttStrPayload);
  publishCustomParam(10100, mqttStrPayload);

  // just ping
  printFmtToDebug("%lu Ping!\r\n", millis());

}

// 2. Pulse Counter Logging (Executes every 6 Seconds)
// if (custom_timer > custom_timer_compare6+6000) { // every 6 seconds 
//   // lastPulsePrintTime = custom_timer; // Reset ONLY the pulse timer baseline
//   custom_timer_compare6 = millis();


// }


// for future testing
// Automation logic conditions
  // if (input1 && (input2 || input3)) {
  //   digitalWrite(PIN_RELAY, HIGH);
  // }
  // if (!input2 && !input3) {
  //   digitalWrite(PIN_RELAY, LOW);
  // }
