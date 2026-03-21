#include <M5Unified.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <math.h>
#include "playback_service.h"
#include "globals.h"
#include "config.h"
#include "face_service.h"

// ── 口パク用の変数
static size_t lipSyncOffset = 0;   // WAV内の現在読み取り位置
static unsigned long lastLipMs = 0;

// WAVヘッダーサイズ（44バイト固定）
#define WAV_HEADER_SIZE 44

// 口パク更新間隔（ms）
#define LIPSYNC_INTERVAL_MS 50

// RMS計算するサンプル数（50ms × 44100Hz × 2bytes ≒ 4410サンプル）
// ただし再生中なので少なめに
#define LIPSYNC_CHUNK_SAMPLES 1024


// ════════════════════════════════════════
//  口パク更新（loop()から毎回呼ぶ）
// ════════════════════════════════════════
void updateLipSync() {
    if (!isPlaying || currentWavData == nullptr || currentWavSize == 0) {
        return;
    }

    unsigned long now = millis();
    if (now - lastLipMs < LIPSYNC_INTERVAL_MS) return;
    lastLipMs = now;

    // WAVヘッダーを超えていなければスキップ
    if (lipSyncOffset < WAV_HEADER_SIZE) {
        lipSyncOffset = WAV_HEADER_SIZE;
    }

    // データ終端チェック
    if (lipSyncOffset >= currentWavSize) {
        avatar.setMouthOpenRatio(0.0f);
        return;
    }

    // 現在位置から LIPSYNC_CHUNK_SAMPLES 分のRMS計算
    int16_t* pcm = (int16_t*)(currentWavData + lipSyncOffset);
    size_t remainBytes = currentWavSize - lipSyncOffset;
    size_t samples = min((size_t)LIPSYNC_CHUNK_SAMPLES,
                         remainBytes / sizeof(int16_t));

    if (samples == 0) {
        avatar.setMouthOpenRatio(0.0f);
        return;
    }

    // RMS計算
    float sum = 0.0f; 
    for (size_t i = 0; i < samples; i++) {
        float v = (float)pcm[i] / 32768.0f;
        sum += v * v;
    }
    float rms = sqrtf(sum / (float)samples); 

    // RMS → 口の開き具合（0.0〜1.0）にマッピング
    // VOICEVOXの音声はRMS 0.01〜0.15くらいの範囲
    float ratio = constrain(rms * 8.0f, 0.0f, 1.0f);

    avatar.setMouthOpenRatio(ratio);

    // 次のチャンクへ進める
    lipSyncOffset += samples * sizeof(int16_t);
}


// ════════════════════════════════════════
//  再生開始（口パクオフセットリセット）
// ════════════════════════════════════════
void startPlayback(const AudioTask& task) {
    currentTask = task;

    // 口パクリセット
    lipSyncOffset = WAV_HEADER_SIZE;
    lastLipMs = 0;

    Serial.print("\n[PLAY] Starting playback: ");
    Serial.println(task.voice_id);

    M5.Display.fillScreen(BLACK);

    uint8_t* wavData = nullptr;
    size_t wavSize = 0;

    if (!downloadVoice(task.voice_url, &wavData, &wavSize)) {
        Serial.println("[PLAY] Failed to download");
        isPlaying = false;
        return;
    }

    Serial.printf("[PLAY] Downloaded: size=%u bytes\n", (unsigned)wavSize);

    currentWavData = wavData;
    currentWavSize = wavSize;

    const float bytes_per_sec = 24000.0f * 2.0f;  // VOICEVOX: 24kHz, 16bit
    const unsigned long expectedMs =
        (unsigned long)((currentWavSize / bytes_per_sec) * 1000.0f) + 2000;
    playbackDeadlineMs = millis() + expectedMs;

    // マイク停止 → スピーカー起動
    if (M5.Mic.isRunning()) {
        M5.Mic.end();
        Serial.println("[PLAY] Mic stopped");
    }
    if (!M5.Speaker.isRunning()) {
        M5.Speaker.begin();
    }

    M5.Speaker.setVolume(SPEAKER_VOLUME);
    M5.Speaker.playWav(currentWavData, currentWavSize);
    setFaceExpression(FACE_PLAYING); 

    delay(100);
    isPlaying = M5.Speaker.isPlaying();
    Serial.printf("[PLAY] isPlaying=%s\n", isPlaying ? "true" : "false");
}


// ════════════════════════════════════════
//  音声ダウンロード
// ════════════════════════════════════════
bool downloadVoice(const String& url, uint8_t** outData, size_t* outSize) {
    HTTPClient http;
    Serial.print("[DOWNLOAD] URL: ");
    Serial.println(url);

    http.begin(url);
    http.setTimeout(HTTP_TIMEOUT_DOWNLOAD);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[DOWNLOAD] HTTP error: %d\n", httpCode);
        http.end();
        return false;
    }

    int len = http.getSize();
    uint8_t* wavData = (uint8_t*)ps_malloc(len);
    if (wavData == NULL) {
        Serial.println("[DOWNLOAD] Memory allocation failed!");
        http.end();
        return false;
    }

    WiFiClient* stream = http.getStreamPtr();
    size_t bytesRead = 0;
    while (http.connected() && bytesRead < len) {
        size_t available = stream->available();
        if (available) {
            size_t toRead = min(available, (size_t)(len - bytesRead));
            stream->readBytes(wavData + bytesRead, toRead);
            bytesRead += toRead;
        }
        delay(1);
    }

    http.end();
    *outData = wavData;
    *outSize = len;
    return true;
}

// ════════════════════════════════════════
//  再生完了後の処理
// ════════════════════════════════════════
void processAudioQueue() {
    if (isPlaying) return;

    // 口を閉じる
    avatar.setMouthOpenRatio(0.0f);

    if (audioQueue.empty()) {
        setFaceExpression(FACE_IDLE);
    }

    if (!audioQueue.empty()) {
        AudioTask next = audioQueue.top();
        audioQueue.pop();
        startPlayback(next);
        last_played_voice_id = next.voice_id;
    }
}