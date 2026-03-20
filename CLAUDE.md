# CLAUDE.md - m5stack-push-avatar

## プロジェクト概要

M5Stack CoreS3 Lite 用の Arduino C++ ファームウェア。  
PC/Mac から push 型で音声を送信できるアバターロボット。  
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
[外部サーバー（Mac）]
  POST /play {"voice_url": "..."}
    → AudioTask をキューに積む
    → スピーカーで再生 + 口パク

[マイク録音]
  音声検出（RMSトリガー）
    → WAV 生成
    → storeLastRecording() で /audio に保存
    → APIモード時: POST /speech/transcribe に送信
    → MCPモード時: スキップ（外部が GET /audio で取得）
```

---

## ファイル構成と役割

| ファイル | 役割 |
|---------|------|
| `yunoM5CoreS3.ino` | setup() / loop() のみ。各サービスを呼ぶだけ |
| `config.h` | WiFi・サーバーURL・マイク閾値等の定数 |
| `http_server.cpp` | `/play` `/mode` `/audio/status` `/audio` |
| `mic_service.cpp` | 録音・VAD・WAV生成・送信 |
| `chat_service.cpp` | POST /chat 呼び出し |
| `face_service.cpp` | m5stack-avatar 表情制御 |
| `globals.cpp` | AudioTask キュー・再生状態等のグローバル変数 |
| `types.h` | AudioTask 構造体・優先度定義 |

---

## 重要な設計上の注意

### マイク設定
- `magnification` は `Mic.config()` と `Mic.begin()` **両方**で同じ値を設定すること
- `MIC_FRAME_SAMPLES=1600`（100ms）未満だとDMAコマ落ちが発生する
- CoreS3 はマイクとスピーカーを**同時使用不可**。再生中はマイクを止める

### push 型再生
- Mac 側から `POST /play` で voice_url を送信
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
#define MIC_SAMPLE_RATE          16000
#define MIC_TRIGGER_RMS          0.0095f   // 録音開始閾値
#define MIC_TRIGGER_HOLD_MS      280       // トリガー持続時間
#define MIC_SILENCE_RMS          0.0020f   // 無音判定閾値
#define MIC_SILENCE_HOLD_MS      1500      // 無音終了判定
#define MIC_FRAME_SAMPLES        1600      // 100ms フレーム
#define MIC_MIN_VALID_SAMPLES    5200      // 最低サンプル数
#define MIC_VOICE_CONFIRM_RMS    0.004f    // ノイズ判定閾値
#define PRE_TRIGGER_BUFFER_SAMPLES 4800   // プリトリガー 0.3秒
#define SPEAKER_VOLUME           200
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
| 声の頭が切れる | プリトリガー未設定 | PRE_TRIGGER_BUFFER_SAMPLES=4800 |
| M5Stack が起動しない | フラッシュ破損 | esptool erase-flash |
| /play に繋がらない | IP・ポートの不一致 | config.h の SERVER_URL を確認 |
