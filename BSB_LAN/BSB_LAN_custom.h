/* 
 * This code is run at the end of each main loop and utilizes the main loop variables 
 * custom_timer (set each loop to millis()) and custom_timer_compare.
 * This short example prints a "Ping!" message every 60 seconds.
*/


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
    // This forces BSB-LAN to natively broadcast these values over MQTT automatically!
    set(10101, (char*)(current_in1 ? "1" : "0"), 1);
    set(10102, (char*)(current_in2 ? "1" : "0"), 1);
    set(10103, (char*)(current_in3 ? "1" : "0"), 1);
    

    bool targetHeatingState = false;

    // Evaluate original boiler automation logic
    if (current_in1 && (current_in2 || current_in3)) {
      targetHeatingState = true;
    }
    else if (!current_in2 && !current_in3) {
      targetHeatingState = false;
    }
    else {
      // If it doesn't match either rule, we want to maintain the CURRENT physical state of the boiler.
      // We read the current state of parameter 700, or skip transmission to prevent spamming.
      // Instead of return, we just do nothing and skip the transmission code block below.
      goto skipTransmission;
    }

    // Transmit state data directly to the heating controller parameter 700 
    if (targetHeatingState) {
      uint32_t rc = set(700, (char*)"1", 1); // Comfort Mode
      printFmtToDebug("Automation: Boiler set to Comfort Mode. Result: %lu\r\n", rc);
      digitalWrite(PIN_RELAY, HIGH);
    } else {
      uint32_t rc = set(700, (char*)"0", 1); // Protection Mode
      printFmtToDebug("Automation: Boiler set to Protection Mode. Result: %lu\r\n", rc);
      digitalWrite(PIN_RELAY, LOW);
    }
    skipTransmission: // A safe way to jump over transmission without exiting the loop
    ;
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

  // Push to parameter 10104 as text
  set(10104, pulsesInWindowStrPayload, 1);

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

  // Push to parameter 10105 as int
  set(10105, pulsesStrPayload, 1);

  // 5. NATIVE MQTT PUSH
  // Convert our double variable into a text string with 4 decimal places
  char mqttStrPayload[16];
  dtostrf(currentLoadKW, 1, 4, mqttStrPayload);

  // Push to parameter 10100. BSB-LAN will automatically broadcast this via MQTT!
  set(10100, mqttStrPayload, 1);

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
