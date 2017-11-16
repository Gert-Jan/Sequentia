#pragma once

#include "SeqWorkerTask.h";

enum class SeqWorkerTaskPriority;
struct SeqLibraryLink;
class SeqDecoder;
struct SDL_mutex;

class SeqTaskDecodeVideo : public SeqWorkerTask
{
public:
	SeqTaskDecodeVideo(SeqLibraryLink *link);
	~SeqTaskDecodeVideo();
	SeqLibraryLink* GetLink();
	void Start();
	void Stop();
	void Finalize();
	SeqDecoder* GetDecoder();
	SeqWorkerTaskPriority GetPriority();
	float GetProgress();
private:
	SeqLibraryLink *link;
	SeqDecoder *decoder;
	SDL_mutex *mutex;
	int error;
	bool done;
};
