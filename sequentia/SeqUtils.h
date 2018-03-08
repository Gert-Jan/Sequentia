#pragma once

#include "SDL_config.h";

#define SEQ_COUNT(arr) ((int)(sizeof(arr)/sizeof(*arr)))

class SeqUtils
{
public:
	static void GetTimeString(char *buffer, int bufferLen, int64_t time);
	static void GetTimeString(char *buffer, int bufferLen, uint32_t time);
};
