#pragma once
#include <Avatar.h>

using namespace m5avatar;

// 外部からアクセスできるようにグローバル宣言
extern Avatar avatar;

// 表情の種類
enum FaceExpression {
    FACE_IDLE      = 0,  // 待機
    FACE_LISTENING = 1,  // 聞き取り中
    FACE_PLAYING   = 2,  // 音声再生中（口パク）
    FACE_THINKING  = 3,  // AI処理中
    FACE_HAPPY     = 4,  // 嬉しい
};

void initFace();
void setFaceExpression(FaceExpression expr);
void setMouthOpen(float ratio);  // 0.0〜1.0 口パク用
void updateIdleFaceByTime();