#ifndef GLOBALS_H
#define GLOBALS_H

#include <queue>
#include "types.h"

extern std::priority_queue<AudioTask> audioQueue;
extern bool isPlaying;
extern uint8_t* currentWavData;
extern size_t currentWavSize;
extern AudioTask currentTask;
extern unsigned long playbackDeadlineMs;
extern bool micResumeRequested;
extern unsigned long micResumeAtMs;

extern String last_played_voice_id;
extern String interrupted_voice_id;
extern String interrupted_voice_url;

// サーバーから時刻をみる
extern int serverHour;

#endif
