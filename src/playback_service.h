#ifndef PLAYBACK_SERVICE_H
#define PLAYBACK_SERVICE_H

#include "types.h"

void startPlayback(const AudioTask& task);
bool downloadVoice(const String& url, uint8_t** outData, size_t* outSize);
void processAudioQueue();
void updateLipSync(); 

#endif