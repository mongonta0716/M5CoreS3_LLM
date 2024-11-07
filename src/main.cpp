/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 * SPDX-License-Identifier: MIT
 * M5Stack LLM Module で日本語対話。Serial MonitorでBoth BL&CRを設定するとよいです。
 */
#include <Arduino.h>
#include <M5Unified.h>
#include <M5ModuleLLM.h>

M5ModuleLLM module_llm;
String llm_work_id;

void setup()
{
    M5.begin();
    M5.Display.setTextSize(1);
    M5.Display.setTextScroll(true);
    M5.Lcd.setTextFont(&fonts::efontJA_16);

    /* Init module serial port */
    Serial.begin(115200);
    //Serial2.begin(115200, SERIAL_8N1, 16, 17);  // Basic
    //Serial2.begin(115200, SERIAL_8N1, 13, 14);  // Core2
    Serial2.begin(115200, SERIAL_8N1, 18, 17);  // CoreS3

    /* Init module */
    module_llm.begin(&Serial2);

    /* Make sure module is connected */
    M5.Display.printf(">> Check ModuleLLM connection..\n");
    while (1) {
        if (module_llm.checkConnection()) {
            break;
        }
    }

    /* Reset ModuleLLM */
    M5.Display.printf(">> Reset ModuleLLM..\n");
    module_llm.sys.reset();

    /* Setup LLM module and save returned work id */
    M5.Display.printf(">> Setup llm..\n");
    llm_work_id = module_llm.llm.setup();
    Serial.print("Initialized...");
}

void loop()
{
    if (Serial.available() > 0) { 
      String input = Serial.readString();
      std::string question = input.c_str();

      M5.Display.setTextColor(TFT_GREEN);
      M5.Display.printf("<< %s\n", question.c_str());
      Serial.printf("<< %s\n", question.c_str());
      M5.Display.setTextColor(TFT_YELLOW);
      M5.Display.printf(">> ");
      Serial.printf(">> ");

      /* Push question to LLM module and wait inference result */
      module_llm.llm.inferenceAndWaitResult(llm_work_id, question.c_str(), [](String& result) {
          /* Show result on screen */
          M5.Display.printf("%s", result.c_str());
          Serial.printf("%s", result.c_str());
      });
      M5.Display.println();
    }

    delay(500);
}