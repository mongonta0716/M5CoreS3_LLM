/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 * SPDX-License-Identifier: MIT
 * M5Stack LLM Module で日本語対話。Serial MonitorでBoth BL&CRを設定するとよいです。
 */
#include <Arduino.h>
#include <M5Unified.h>
#include <M5ModuleLLM.h>
#include "AquesTalkTTS.h" // AquesTalk-ESP32のラッパークラス
#include <vector>

M5ModuleLLM module_llm;
String llm_work_id;
std::vector<std::string> delimiter_str = { "？", "。" };

std::string talk_str_temp = "";
std::vector<std::string> talk_str; // AquesTalkで喋る文字列
int talk_num = 0;
bool isTalking = false;

void talkLoop(void* args) {
    for(;;) {
        while(isTalking) {
            M5_LOGI("AQTalk: %s", talk_str[talk_num].c_str());
            TTS.playK(talk_str[talk_num].c_str(), 100);
            TTS.wait();
            talk_num++;
            if (talk_num >= talk_str.size()) {
                isTalking = false;
                talk_num = 0;
                talk_str.clear();
                break;
            }
        }
        vTaskDelay(5/portTICK_PERIOD_MS);
    }
}


void setup()
{
    M5.begin();
    M5.Display.setTextSize(1);
    M5.Display.setTextScroll(true);
    M5.Lcd.setTextFont(&fonts::efontJA_16);
    M5.Log.setEnableColor(m5::log_target_t::log_target_serial, false);

    /* Init module serial port */
    Serial.begin(115200);

    //Serial2.begin(115200, SERIAL_8N1, 16, 17);  // Basic
    //Serial2.begin(115200, SERIAL_8N1, 13, 14);  // Core2
    //Serial2.begin(115200, SERIAL_8N1, 18, 17);  // CoreS3
    Serial2.begin(115200, SERIAL_8N1, M5.getPin(m5::pin_name_t::port_c_rxd), M5.getPin(m5::pin_name_t::port_c_txd));  // Port.C
    //Serial2.begin(115200, SERIAL_8N1, 13, 12);  // Fire Custom
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
    // AquesTalkの初期化
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    M5_LOGI("¥nMAC Address = %02X:%02X:%02X:%02X:%02X:%02X¥r¥n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    int err = TTS.createK();
    if (err) {
        M5_LOGI("ERR:TTS.createK(): %d", err);
    }
    M5.Speaker.setVolume(128);
    TTS.playK("こんにちは、漢字を混ぜた文章です。");
    TTS.wait();   // 終了まで待つ

    M5_LOGI("talkLoop");
    xTaskCreateUniversal(talkLoop,
                        "talkLoop",
                        4096,
                        NULL,
                        5,
                        NULL,
                        1);// tskNO_AFFINITY); // Core 1を指定しないと不安定
    M5_LOGI("Setup End");

}



void loop()
{
    if (Serial.available() > 0) { 
      String input = Serial.readString();
      std::string question = input.c_str();

      M5.Display.setTextColor(TFT_GREEN);
      M5.Display.printf("<< %s\n", question.c_str());
      M5_LOGI("<< %s\n", question.c_str());
      M5.Display.setTextColor(TFT_YELLOW);
      M5.Display.printf(">> ");
      M5_LOGI(">> ");

      /* Push question to LLM module and wait inference result */
      module_llm.llm.inferenceAndWaitResult(llm_work_id, question.c_str(), [](String& result) {
          /* Show result on screen */
          talk_str_temp += result.c_str();
          M5.Display.printf("%s", result.c_str());
          M5_LOGI("%s", result.c_str());
          // 文の終わりをチェック
          bool last_str = false;
          for (const auto& delimiter : delimiter_str) {
            if (talk_str_temp.find(delimiter) != std::string::npos) {
              last_str = true;
              break;
            }
          }
          if (last_str) {
              M5.Display.println(llm_work_id);
              M5_LOGI("文字列:%s", talk_str_temp.c_str());
              talk_str.push_back(talk_str_temp);
              isTalking = true;
              talk_str_temp = "";
          }
      });
      TTS.wait();
      isTalking = false;
      M5.Display.println();
    }

    delay(50);
}