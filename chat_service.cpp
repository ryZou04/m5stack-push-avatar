#include <M5CoreS3.h>
#include <WiFi.h>        
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "chat_service.h"
#include "queue_manager.h"
#include "globals.h"
#include "config.h"

void sendChatRequest(const char* text){
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    String chatUrl = String(SERVER_URL) + "/chat";

    StaticJsonDocument<512> req;
    req["text"] = text;
    req["generate_voice"] = true;

    String body;
    serializeJson(req, body);

    Serial.print("[CHAT] Sending: ");
    Serial.println(body);

    http.begin(chatUrl);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(30000);

    int code = http.POST(body);

    if (code != HTTP_CODE_OK) {
        Serial.printf("[CHAT] HTTP=%d\n", code);
        http.end();
        return;
    }

    String payload = http.getString();
    http.end();

    DynamicJsonDocument resp(2048);
    if (deserializeJson(resp, payload) != DeserializationError::Ok) {
        Serial.println("[CHAT] JSON parse error");
        return;
    }

    if (!resp["success"]) return;

    const char* ai = resp["response"] | "";
    if (strlen(ai) > 0) {
        Serial.print("[CHAT]AI: ");
        Serial.println(ai);
    }
}

void testChat(const char* text) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[TEST] WiFi not connected");
        return;
    }
    sendChatRequest(text);

}

void syncServerHour() {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    String url = String(SERVER_URL) + "/time";
    http.begin(url);
    http.setTimeout(5000);

    int code = http.GET();
    if (code == HTTP_CODE_OK) {
        String payload = http.getString();
        StaticJsonDocument<64> doc;
        if (deserializeJson(doc, payload) == DeserializationError::Ok) {
            serverHour = doc["server_hour"] | -1;
            Serial.printf("[TIME] serverHour = %d\n", serverHour);
        }
    } else {
        Serial.printf("[TIME] /time failed: %d\n", code);
    }
    http.end();
}