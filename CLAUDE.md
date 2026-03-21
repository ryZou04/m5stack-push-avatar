# CLAUDE.md - m5stack-push-avatar

## プロジェクト概要

M5Stack CoreS3 Lite 用の Arduino C++ ファームウェア。  
PC/Mac またはスマホアプリから push 型で音声を送信できるアバターロボット。  
[yuno-chan-api](https://github.com/yukincom/yuno-chan-api) と連携して動作する。

---

## ハードウェア

- **ボード**: M5Stack CoreS3 Lite
- **マイク**: ES7210（16kHz、stereo=false、magnification=2）
- **スピーカー**: AW88298（SPEAKER_VOLUME=200）
- **ディスプレイ**: 2インチ LCD（アバター表示）

---

## アーキテクチャ

```
[外部サーバー（Mac または スマホ）]
  POST /play {"voice_url": "..."}
    → AudioTask をキューに積む
    → スピーカーで再生 + 口パク

[マイク録音]
  音声検出（RMSトリガー）
    → isValidAudio() で品質チェック
    → buildWav() で WAV 生成
    → storeLastRecording() で /audio に保存
    → APIモード時: POST /speech/transcribe に送信
    → MCPモード時: スキップ（外部が GET /audio で取得）
```

---

## ファイル構成と役割

| ファイル | 役割 |
|---------|------|
| `yunoM5CoreS3.ino` | setup() / loop() のみ。各サービスを呼ぶだけ |
| `config.h` | WiFi・サーバーURL・マイク閾値・タイムアウト等の定数 |
| `http_server.cpp` | `/play` `/mode` `/audio/status` `/audio` |
| `mic_service.cpp` | 録音・VAD・WAV生成（isValidAudio/buildWav/sendAudioToServer）|
| `chat_service.cpp` | POST /chat 呼び出し |
| `face_service.cpp` | m5stack-avatar 表情制御 |
| `globals.cpp` | AudioTask キュー・再生状態・serverUrl 等のグローバル変数 |
| `wifi_manager.cpp` | デュアルネットワーク接続・serverUrl のセット |
| `types.h` | AudioTask 構造体・優先度定義 |

---

## 重要な設計上の注意

### デュアルネットワーク
- 起動時に `WIFI_SSID_0`（自宅）→ `WIFI_SSID_1`（ホットスポット）の順に接続を試みる
- 接続成功したネットワークに対応する `serverUrl` がグローバル変数にセットされる
- 各サービス（mic/chat/notification）は `SERVER_URL` マクロではなく `serverUrl` 変数を使う

### マイク設定
- `magnification` は `Mic.config()` と `Mic.begin()` **両方**で同じ値を設定すること
- `MIC_FRAME_SAMPLES=1600`（100ms）未満だとDMAコマ落ちが発生する
- CoreS3 はマイクとスピーカーを**同時使用不可**。再生中はマイクを止める

### push 型再生
- サーバー側から `POST /play` で voice_url を送信
- M5Stack 側は AudioTask キューから順に再生
- ポーリング型（M5Stack が定期的にサーバーを叩く）ではない

### モード切り替え
- `api`（デフォルト）: 録音 → `/speech/transcribe` に直接送信
- `mcp`: 録音 → バッファ保存のみ（外部スクリプトが `/audio` で取得）
- モード切り替えは `POST /mode {"mode": "mcp"|"api"}`

### WAV フォーマット
- 16kHz / 16bit / モノラル PCM
- ffmpeg リサンプル不要（M5Stack が直接 16kHz で録音）

---

## config.h の主要定数

```cpp
// ネットワーク（複数系統を定義）
#define WIFI_NETWORK_COUNT 2
#define WIFI_SSID_0        "your-home-ssid"
#define WIFI_PASSWORD_0    "your-home-password"
#define SERVER_URL_0       "http://192.168.1.x:5000"   // 自宅Mac
#define WIFI_SSID_1        "YourHotspotName"
#define WIFI_PASSWORD_1    "your-hotspot-password"
#define SERVER_URL_1       "http://172.20.10.1:5000"   // スマホアプリ

// HTTP タイムアウト（ms）
#define HTTP_TIMEOUT_STT      30000  // 音声認識
#define HTTP_TIMEOUT_CHAT     30000  // AI応答
#define HTTP_TIMEOUT_DOWNLOAD 10000  // 音声ダウンロード
#define HTTP_TIMEOUT_SHORT     5000  // 時刻同期等

// 時刻同期間隔（push型移行後はserverHour同期のみ）
#define SERVER_HOUR_SYNC_INTERVAL 300000

// マイク
#define MIC_SAMPLE_RATE            16000
#define MIC_TRIGGER_RMS            0.0095f
#define MIC_TRIGGER_HOLD_MS        280
#define MIC_SILENCE_RMS            0.0020f
#define MIC_SILENCE_HOLD_MS        1500
#define MIC_FRAME_SAMPLES          1600
#define MIC_MIN_VALID_SAMPLES      5200
#define MIC_VOICE_CONFIRM_RMS      0.004f
#define PRE_TRIGGER_BUFFER_SAMPLES 12800

// スピーカー
#define SPEAKER_VOLUME 200
```

---

## 連携するバックエンドのエンドポイント

| エンドポイント | メソッド | 用途 |
|--------------|---------|------|
| `/speech/transcribe` | POST | WAV → テキスト（Whisper） |
| `/chat` | POST | テキスト → AI応答 + 音声生成 |
| `/voice/<id>` | GET | 音声ファイル取得 |

バックエンド実装: [yuno-chan-api](https://github.com/yukincom/yuno-chan-api)

---

## よくある問題

| 症状 | 原因 | 対処 |
|------|------|------|
| 音声がぼぼぼっと途切れる | MIC_FRAME_SAMPLES が小さい | 1600に設定 |
| マイクが初期化失敗 | スピーカーが起動中 | Mic.begin() 前に Speaker.end() |
| 声の頭が切れる | プリトリガー未設定 | PRE_TRIGGER_BUFFER_SAMPLES=12800 |
| M5Stack が起動しない | フラッシュ破損 | esptool erase-flash |
| どのネットワークにも繋がらない | SSID/パスワード不一致 | config.h の SSID/PASSWORD を確認 |
| serverUrl が空のまま | 全ネットワーク接続失敗 | シリアルモニタで [WIFI] ログを確認 |
