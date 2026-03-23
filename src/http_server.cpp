#include <M5Unified.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "http_server.h"
#include "queue_manager.h"
#include "globals.h"
#include "types.h"

static WebServer server(80);

// ── 録音バッファ（PSRAMに確保）
static uint8_t* s_wav_buf   = nullptr;
static size_t   s_wav_size  = 0;
static bool     s_wav_ready = false;

// ── モードフラグ（false=APIモード / true=MCPモード）
static bool s_mcp_mode = false;

// ────────────────────────────────────────────
// POST /play
// body: {"voice_url": "http://..."}
// → AudioTaskをキューに積んで再生
// ────────────────────────────────────────────
static void handlePlay() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"success\":false,\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain")) != DeserializationError::Ok) {
        server.send(400, "application/json", "{\"success\":false,\"error\":\"json parse error\"}");
        return;
    }

    const char* voice_url = doc["voice_url"] | "";
    if (strlen(voice_url) == 0) {
        server.send(400, "application/json", "{\"success\":false,\"error\":\"voice_url required\"}");
        return;
    }

    AudioTask task;
    task.voice_id  = String("mcp_") + String(millis());
    task.voice_url = String(voice_url);
    task.priority  = PRIORITY_NORMAL;
    enqueueAudioTask(task);

    Serial.printf("[HTTP] POST /play -> queued: %s\n", voice_url);
    server.send(200, "application/json", "{\"success\":true}");
}

// ────────────────────────────────────────────
// POST /mode
// body: {"mode": "mcp"} または {"mode": "api"}
// → MCPモード / APIモードを切り替える
// ────────────────────────────────────────────
static void handleMode() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"success\":false,\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain")) != DeserializationError::Ok) {
        server.send(400, "application/json", "{\"success\":false,\"error\":\"json parse error\"}");
        return;
    }

    const char* mode = doc["mode"] | "";
    if (strcmp(mode, "mcp") == 0) {
        s_mcp_mode = true;
        // 古い録音を完全クリア
        s_wav_ready = false;
        s_wav_size  = 0; 
        if (s_wav_buf) {
            free(s_wav_buf); 
            s_wav_buf = nullptr;
        }
        Serial.println("[HTTP] Mode -> MCP (buffer cleared)");
        server.send(200, "application/json", "{\"success\":true,\"mode\":\"mcp\"}");
    } else if (strcmp(mode, "api") == 0) {
        s_mcp_mode = false;
        Serial.println("[HTTP] Mode -> API");
        server.send(200, "application/json", "{\"success\":true,\"mode\":\"api\"}");
    } else {
        server.send(400, "application/json", "{\"success\":false,\"error\":\"mode must be mcp or api\"}");
    }
}

// ────────────────────────────────────────────
// GET /audio/status
// → {"ready": true/false, "mode": "mcp"|"api"}
// ────────────────────────────────────────────
static void handleAudioStatus() {
    String body = "{\"ready\":";
    body += s_wav_ready ? "true" : "false";
    body += ",\"mode\":\"";
    body += s_mcp_mode ? "mcp" : "api";
    body += "\"}";
    server.send(200, "application/json", body);
}

// ────────────────────────────────────────────
// GET /audio
// → 録音済みWAVをそのまま返す（1回読んだらクリア）
// ────────────────────────────────────────────
static void handleAudio() {
    if (!s_wav_ready || s_wav_buf == nullptr || s_wav_size == 0) {
        server.send(404, "application/json", "{\"success\":false,\"error\":\"no audio\"}");
        return;
    }

    Serial.printf("[HTTP] GET /audio -> %u bytes\n", (unsigned)s_wav_size);
    server.send_P(200, "audio/wav", (const char*)s_wav_buf, s_wav_size);

    // 読んだらクリア（1回限り）
    s_wav_ready = false;
}

// ────────────────────────────────────────────
// 公開関数
// ────────────────────────────────────────────

bool isMcpMode() {
    return s_mcp_mode;
}

void storeLastRecording(const uint8_t* wav, size_t size) {
    if (s_wav_buf) {
        free(s_wav_buf);
        s_wav_buf = nullptr;
    }
    s_wav_buf = (uint8_t*)ps_malloc(size);
    if (!s_wav_buf) {
        Serial.println("[HTTP] WAV buffer alloc failed");
        s_wav_size  = 0;
        s_wav_ready = false;
        return;
    }
    memcpy(s_wav_buf, wav, size);
    s_wav_size  = size;
    s_wav_ready = true;
    Serial.printf("[HTTP] Stored recording: %u bytes\n", (unsigned)size);
}

void initHttpServer() {
    server.on("/play",         HTTP_POST, handlePlay);
    server.on("/mode",         HTTP_POST, handleMode);
    server.on("/audio/status", HTTP_GET,  handleAudioStatus);
    server.on("/audio",        HTTP_GET,  handleAudio);
    server.begin();
    Serial.println("[HTTP] Server started on port 80");
}

void handleHttpServer() {
    server.handleClient();
}
