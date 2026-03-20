# m5stack-push-avatar

**Push-based voice avatar firmware for M5Stack CoreS3**

PC/Mac から音声をプッシュ送信できる、M5Stack CoreS3 用アバターファームウェアです。  
[m5stack-avatar](https://github.com/meganetaaan/m5stack-avatar) をベースに、外部サーバーからの push 型音声再生に対応しています。

---

## ✨ 特徴

- **Push 型音声再生**: PC/Mac から `POST /play` で音声URLを送るだけで即座に再生
- **ポーリング不要**: M5Stack 側がサーバーを叩きに行かないため、レスポンスが速い
- **MCP モード対応**: Claude Desktop 等の MCP クライアントと連携可能
- **プリトリガーバッファ**: 発話の頭切れを防ぐリングバッファ実装済み
- **Arduino C++**: Moddable / TypeScript 不要、Arduino IDE でそのまま書き込める

---

## 🔧 動作確認済み環境

| ハードウェア | 備考 |
|-------------|------|
| M5Stack CoreS3 Lite | CoreS3（通常版）でも動作するはず |

---

## ⚙️ セットアップ

### 1. `config.h` を編集

```cpp
#define WIFI_SSID     "your-ssid"
#define WIFI_PASSWORD "your-password"
#define SERVER_URL    "http://192.168.1.x:5000"  // バックエンドのURL
```

### 2. Arduino IDE で書き込み

必要なライブラリ（ライブラリマネージャーからインストール）：
- M5Unified
- m5stack-avatar
- ArduinoJson

ボード設定: `M5Stack CoreS3`

---

## 📡 主なエンドポイント

| エンドポイント | 用途 |
|--------------|------|
| `POST /play` | 音声URLを受け取って再生 |
| `POST /mode` | `api` / `mcp` モード切り替え |
| `GET /audio/status` | 録音完了フラグを確認 |
| `GET /audio` | 録音済み WAV を取得 |

---

## 🔌 連携バックエンド

[yuno-chan-api](https://github.com/yukincom/yuno-chan-api) はこのファームウェアに対応したバックエンド実装です。  
Whisper（STT）・VOICEVOX / Kokoro（TTS）・Gemini（AI）を組み合わせたホームアシスタントとして動作します。

---

## 🙏 クレジット

- [m5stack-avatar](https://github.com/meganetaaan/m5stack-avatar) by meganetaaan

---

## 📄 ライセンス

MIT
