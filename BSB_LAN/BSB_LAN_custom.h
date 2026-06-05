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
  bool relay = (digitalRead(PIN_RELAY));

 
  // Logging
  // Serial.print("Relay: ");
  // Serial.println(digitalRead(PIN_RELAY));
  // Serial.print("Input1: ");
  // Serial.println(digitalRead(PIN_INPUT_1));

  // If a physical input switch changes position to ON 
  if (current_in1 != last_in1 || current_in2 != last_in2 || current_in3 != last_in3 || firstRunExecution ) {
    
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

  // --- DIRECT MQTT PUSH FOR VIRTUAL INPUTS ---
    if (MQTTPubSubClient && MQTTPubSubClient->connected()) { 
      char topic[64];
      
      sprintf(topic, "%s/10101", MQTTTopicPrefix);
      MQTTPubSubClient->publish(topic, current_in1 ? "1" : "0"); 
      
      sprintf(topic, "%s/10102", MQTTTopicPrefix);
      MQTTPubSubClient->publish(topic, current_in2 ? "1" : "0"); 
      
      sprintf(topic, "%s/10103", MQTTTopicPrefix);
      MQTTPubSubClient->publish(topic, current_in3 ? "1" : "0"); 
    }

    // --- INSTANT MQTT PUSH FOR INPUTS ---
    // This forces BSB-LAN to natively broadcast these values over MQTT automatically!
    // set(10101, (char*)(current_in1 ? "1" : "0"), 1);
    // set(10102, (char*)(current_in2 ? "1" : "0"), 1);
    // set(10103, (char*)(current_in3 ? "1" : "0"), 1);
    
    if (current_in1 && (current_in2 || current_in3) ) {
      // Transmit state data directly to the heating controller parameter 700 
        uint32_t rc = set(700, (char*)"1", 1); // Comfort Mode
        printFmtToDebug("Automation: Boiler set to Comfort Mode. Result: %lu\r\n", rc);
        digitalWrite(PIN_RELAY, HIGH);
    } 
    else {
      uint32_t rc = set(700, (char*)"0", 1); // Protection Mode
      printFmtToDebug("Automation: Boiler set to Protection Mode. Result: %lu\r\n", rc);
      digitalWrite(PIN_RELAY, LOW);
    }
}
}

// Run loop logic every 60000ms (60 second)
if (custom_timer > custom_timer_compare60+60000) {    // every 60 seconds  
  custom_timer_compare60 = millis();
  

  // just ping
  printFmtToDebug("%lu Ping!\r\n", millis());

}

// 2. Pulse Counter Logging (Executes every 6 minutes)
if (custom_timer > custom_timer_compare6+360000) { // every 6 minutes 
  custom_timer_compare6 = millis();
  double timeElapsedSeconds = (custom_timer - custom_timer_compare6) / 1000.0;

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
  // set(10104, pulsesInWindowStrPayload, 1);

  oldPulseCount = totalPulsesTracked;

  // 3. Math: Calculate Instantaneous Load (kW)
  if (pulsesInWindow > 0 && timeElapsedSeconds > 0) {
    // Math logic: (Pulses / Seconds) * (3600 / 1600)
    // Over an exact 60-second window, this breaks down to: Pulses * 0.0375
    // Over an exact 6 minutes window (Pulses / 360) * (3600 / 1600) - Simplified, this means the multiplier changes from $0.0375$ to $0.00625$.
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
  // set(10105, pulsesStrPayload, 1);

  // 5. NATIVE MQTT PUSH
  // Convert our double variable into a text string with 4 decimal places
  char mqttStrPayload[16];
  dtostrf(currentLoadKW, 1, 4, mqttStrPayload);

  // Push to parameter 10100. BSB-LAN will automatically broadcast this via MQTT!
  // set(10100, mqttStrPayload, 1);

    if (MQTTPubSubClient && MQTTPubSubClient->connected()) { 
      char topic[64];
      
      sprintf(topic, "%s/10100", MQTTTopicPrefix);
      MQTTPubSubClient->publish(topic, mqttStrPayload); 
      
      sprintf(topic, "%s/10105", MQTTTopicPrefix);
      MQTTPubSubClient->publish(topic, pulsesStrPayload); 
    
    }

}

