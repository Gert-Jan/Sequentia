#pragma once

#include "SeqWorkerTask.h"

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
	void StartDecodeStreamIndex(int streamIndex);
	void StopDecodedStreamIndex(int streamIndex);
	void Start();
	void Stop();
	void Finalize();
	SeqDecoder* GetDecoder();
	SeqWorkerTaskPriority GetPriority();
	float GetProgress();
private:
	SeqLibraryLink *link;
	SeqDecoder *decoder;
	int error;
	bool done;
};
