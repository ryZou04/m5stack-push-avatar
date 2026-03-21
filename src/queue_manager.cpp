#include <M5Unified.h>
#include "queue_manager.h"
#include "globals.h"
#include "playback_service.h"
#include "face_service.h"

void enqueueAudioTask(const AudioTask& task) {
    audioQueue.push(task);
    processAudioQueue();
}

void notifyPlaybackFinished() {
    isPlaying = false;
    if (currentWavData) {
        free(currentWavData);
        currentWavData = nullptr;
        currentWavSize = 0;
    }
    processAudioQueue();

    if (!isPlaying) {
        setMouthOpen(0.0f);  
        setFaceExpression(FACE_IDLE);
        micResumeRequested = true;
        micResumeAtMs = millis() + 2000;
        
    }
}
