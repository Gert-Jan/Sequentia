#pragma once

#include "SDL_config.h"

class SeqScene;
class SeqTaskDecodeVideo;

struct AVFrame;

class SeqPlayer
{
public:
	SeqPlayer(SeqScene *scene);
	~SeqPlayer();

	void Play();
	void Pause();
	void Stop();
	void Seek(int64_t time);
	bool IsActive();

private:
	int activateCount;
	SeqScene *scene;
	//SeqTaskDecodeVideo *decoderTask;
	//AVFrame *previousFrame;
	bool isSeeking;
	int64_t seekVideoTime;
	int64_t startVideoTime;
};
