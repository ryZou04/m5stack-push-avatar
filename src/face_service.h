#pragma once
#include <stdint.h>

// Mechanical states. The bridge may also push a specific emotion name via
// setFaceByName() (e.g. "surprised", "sad", "shy") to override the default
// mapping when responding.
enum FaceExpression {
    FACE_IDLE      = 0,
    FACE_LISTENING = 1,
    FACE_PLAYING   = 2,
    FACE_THINKING  = 3,
    FACE_HAPPY     = 4,
};

void initFace();
void setFaceExpression(FaceExpression expr);
void setFaceByName(const char* name);  // emotion-keyed sprite (e.g. "happy", "shy")
void setMouthOpen(float ratio);        // no-op in bitmap mode; kept for API compat
