#pragma once

#include "SeqWorkerTask.h";

enum class SeqWorkerTaskPriority;
struct SeqLibraryLink;
struct SeqVideoInfo;
class SeqDecoder;

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
};
