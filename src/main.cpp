#include <Arduino.h> 

#include <M5Unified.h>
#include <WiFi.h>  
#include "http_server.h" 
#include "types.h"   
#include "config.h"
#include "globals.h"
#include "queue_manager.h" 
#include "mic_service.h"    
#include "wifi_manager.h"
#include "notification_service.h"
#include "playback_service.h"
#include "chat_service.h"
#include "face_service.h"

void setup() {
    Serial.begin(115200);
    delay(1000);

    auto cfg = M5.config();
    cfg.serial_baudrate = 115200;
    cfg.internal_mic = true;
    cfg.internal_spk = true;
    M5.begin(cfg);
    M5.Display.setBrightness(DISPLAY_BRIGHTNESS);
    
    initFace();  

    Serial.println("\n=== Yuno v5.0 (Microphone) ===");

    // スピーカー初期化（再生時に begin する）
    auto spk_cfg = M5.Speaker.config();
    M5.Speaker.config(spk_cfg);
    M5.Speaker.setVolume(SPEAKER_VOLUME);

    // マイク初期化
    if (!initMicrophone()) {
        Serial.println("[ERROR] Microphone initialization failed!");
    }

    // WiFi接続
    connectWiFi();
    //時刻同期
    syncServerHour();  
    // 初回通知チェック
    checkAndEnqueueNotification();
    initHttpServer();

#ifdef TEST_MODE
    delay(3000);
    testChat("おはよう！");
#endif
}

void loop() {
    M5.update();
    handleHttpServer();
    // WiFi切断時の自動再接続
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WIFI] Disconnected. Reconnecting...");
        WiFi.reconnect();
        delay(5000);  // 再接続待ち
    }    
    updateLipSync(); 
    // マイク常時監視（再生中は自動スキップ）
    updateMicrophone();

    // 再生完了検知
    if (isPlaying && (!M5.Speaker.isPlaying() || (playbackDeadlineMs != 0 && millis() > playbackDeadlineMs))) {
        if (playbackDeadlineMs != 0 && millis() > playbackDeadlineMs) {
            Serial.println("[PLAY] Playback timeout -> force stop");
            M5.Speaker.stop();
        }
        notifyPlaybackFinished();
    }

    // マイク再開はメインループで実施（WDT回避）
    if (micResumeRequested && !isPlaying) {
        micResumeRequested = false;
        if (M5.Speaker.isRunning()) {
            M5.Speaker.end();
            Serial.println("[PLAY] Speaker stopped after playback");
        }
        if (!M5.Mic.isRunning()) {
            if (M5.Mic.begin()) {
                Serial.println("[MIC] Mic resumed after playback");
            } else {
                Serial.println("[MIC] Mic resume failed");
            }
        }
    }

    // 定期通知チェック
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > NOTIFICATION_CHECK_INTERVAL) {
        checkAndEnqueueNotification();
        lastCheck = millis();
    }

    delay(50);
}
