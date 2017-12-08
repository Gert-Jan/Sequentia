#pragma once

#include "SeqWorkerTask.h"

enum class SeqWorkerTaskPriority;
struct SeqLibraryLink;
class SeqVideoInfo;

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
	SeqLibraryLink *link;
	SeqVideoInfo *resultVideoInfo;
	float progress;
};
