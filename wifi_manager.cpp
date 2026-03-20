#include <M5CoreS3.h>
#include <WiFi.h>
#include "wifi_manager.h"
#include "config.h"

void connectWiFi() {
    Serial.println("\nConnecting to WiFi...");
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        
        M5.Display.fillScreen(GREEN);
        M5.Display.setTextColor(BLACK);
        M5.Display.setCursor(10, 10);
        M5.Display.println("WiFi OK!");
        delay(2000);
        
        M5.Display.fillScreen(BLACK);
        M5.Display.setTextColor(WHITE);
    }
}