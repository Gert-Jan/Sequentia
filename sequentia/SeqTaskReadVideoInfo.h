#pragma once

#include "SeqWorkerTask.h"

#include <SDL_audio.h>
extern "C"
{
#include "libavutil/samplefmt.h"
}

enum class SeqWorkerTaskPriority;
struct SeqLibraryLink;
class SeqVideoContext;

class SeqTaskReadVideoInfo : public SeqWorkerTask
{
public:
	SeqTaskReadVideoInfo(SeqLibraryLink *link);
	~SeqTaskReadVideoInfo();
	void Start();
	void Stop();
	void Finalize();
	SeqWorkerTaskPriority GetPriority();
	float GetProgress();
private:
	SDL_AudioFormat FromAVSampleFormat(AVSampleFormat format, bool *isPlanar);
private:
	SeqLibraryLink *link;
	SeqLibraryLink *tempLink;
	float progress;
};
