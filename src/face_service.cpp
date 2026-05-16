// face_service.cpp
//
// Bitmap face renderer. Replaces the m5stack-avatar vector pipeline with
// JPG sprites served by the PC bridge at `serverUrl/face?name=X`. Sprites
// are fetched on first use and cached in PSRAM so a repeat trigger is
// instant.
//
// 朵朵's sprites are pre-letterboxed by the bridge to 320x240 (CoreS3 LCD),
// so we draw at (0,0) and that's it — no scaling on the ESP32.

#include "face_service.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <M5Unified.h>
#include <WiFi.h>

#include "globals.h"

#define FACE_CACHE_SIZE        8
#define FACE_NAME_MAX          16
#define FACE_FETCH_TIMEOUT_MS  5000

struct FaceCacheEntry {
    char     name[FACE_NAME_MAX];
    uint8_t* data;
    size_t   size;
};

static FaceCacheEntry s_cache[FACE_CACHE_SIZE];
static int            s_cache_count   = 0;
static String         s_current_face  = "";

// Map mechanical states to default sprite filenames (matches faces/ on the bridge).
// FACE_PLAYING defaults to "happy"; the bridge can override per-reply via setFaceByName.
static const char* sprite_for(FaceExpression expr) {
    switch (expr) {
        case FACE_IDLE:      return "idle";
        case FACE_LISTENING: return "listening";  // perked ears + sound waves
        case FACE_PLAYING:   return "happy";
        case FACE_THINKING:  return "question";
        case FACE_HAPPY:     return "happy";
    }
    return "idle";
}

static FaceCacheEntry* cache_lookup(const char* name) {
    for (int i = 0; i < s_cache_count; i++) {
        if (strcmp(s_cache[i].name, name) == 0) return &s_cache[i];
    }
    return nullptr;
}

static FaceCacheEntry* fetch_and_cache(const char* name) {
    if (s_cache_count >= FACE_CACHE_SIZE) {
        Serial.println("[FACE] cache full, skipping fetch");
        return nullptr;
    }
    if (serverUrl.length() == 0) {
        Serial.println("[FACE] serverUrl not set; cannot fetch");
        return nullptr;
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[FACE] WiFi not connected; cannot fetch");
        return nullptr;
    }

    HTTPClient http;
    String url = serverUrl + "/face?name=" + String(name);
    http.begin(url);
    http.setTimeout(FACE_FETCH_TIMEOUT_MS);
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("[FACE] GET %s -> HTTP %d\n", url.c_str(), code);
        http.end();
        return nullptr;
    }
    int len = http.getSize();
    if (len <= 0) {
        Serial.printf("[FACE] %s: bad length %d\n", name, len);
        http.end();
        return nullptr;
    }
    uint8_t* data = (uint8_t*)ps_malloc(len);
    if (!data) {
        Serial.printf("[FACE] %s: ps_malloc(%d) failed\n", name, len);
        http.end();
        return nullptr;
    }
    WiFiClient* stream = http.getStreamPtr();
    size_t read = 0;
    while (http.connected() && read < (size_t)len) {
        size_t avail = stream->available();
        if (avail) {
            size_t to_read = min(avail, (size_t)(len - read));
            stream->readBytes(data + read, to_read);
            read += to_read;
        }
        delay(1);
    }
    http.end();

    FaceCacheEntry* e = &s_cache[s_cache_count++];
    strncpy(e->name, name, FACE_NAME_MAX - 1);
    e->name[FACE_NAME_MAX - 1] = '\0';
    e->data = data;
    e->size = (size_t)len;
    Serial.printf("[FACE] cached %s: %u bytes\n", name, (unsigned)len);
    return e;
}

void initFace() {
    M5.Display.fillScreen(TFT_BLACK);
    Serial.println("[FACE] initFace() done (bitmap mode)");
    Serial.printf("[FACE] Free heap: %u  Free PSRAM: %u\n",
                  (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram());
}

void setFaceByName(const char* name) {
    if (!name || name[0] == '\0') return;
    if (s_current_face.equals(name)) return;  // no redraw

    FaceCacheEntry* e = cache_lookup(name);
    if (!e) e = fetch_and_cache(name);
    if (!e) return;  // fetch failed; keep current face

    M5.Display.drawJpg(e->data, e->size, 0, 0);
    s_current_face = name;
    Serial.printf("[FACE] -> %s\n", name);
}

void setFaceExpression(FaceExpression expr) {
    setFaceByName(sprite_for(expr));
}

void setMouthOpen(float ratio) {
    // Bitmap sprites don't carry mouth state. No-op for now;
    // future: overlay a mouth bitmap based on `ratio`.
    (void)ratio;
}
