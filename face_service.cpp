// face_service.cpp
// m5stack-avatar ライブラリ使用
// カラーパレットはデフォルト（標準スタックちゃん）

#include "face_service.h"
#include "globals.h"
#include <M5CoreS3.h>

Avatar avatar;

void initFace() {
    // カラー設定
    avatar.init();
    Serial.println("[FACE] initFace() done");
    Serial.printf("[FACE] Free heap: %u bytes\n", ESP.getFreeHeap());
}

void setFaceExpression(FaceExpression expr) {
    switch (expr) {
        case FACE_IDLE:
            // 時間帯で表情を変える
            struct tm timeInfo;
            if (serverHour >= 19 || (serverHour >= 0 && serverHour < 7)) {
                avatar.setExpression(Expression::Sleepy);
            } else {
                avatar.setExpression(Expression::Neutral);
            }
            avatar.setMouthOpenRatio(0.0f);
            break;
        
        case FACE_LISTENING:
            avatar.setExpression(Expression::Happy);
            avatar.setMouthOpenRatio(0.0f);
            break;
        case FACE_PLAYING:
            avatar.setExpression(Expression::Neutral);
            avatar.setMouthOpenRatio(0.5f);
            break;
        case FACE_THINKING:
            avatar.setExpression(Expression::Doubt);
            avatar.setMouthOpenRatio(0.0f);
            break;
        case FACE_HAPPY:
            avatar.setExpression(Expression::Happy);
            avatar.setMouthOpenRatio(0.0f);
            break;
    }
    Serial.printf("[FACE] Expression -> %d\n", (int)expr);
}

void setMouthOpen(float ratio) {
    avatar.setMouthOpenRatio(ratio);
}