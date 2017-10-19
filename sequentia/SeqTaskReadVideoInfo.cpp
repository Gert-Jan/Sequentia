#include "SeqTaskReadVideoInfo.h";
#include "SeqLibrary.h";
#include "SeqDecoder.h";
#include "SeqVideoInfo.h";
#include <SDL.h>

SeqTaskReadVideoInfo::SeqTaskReadVideoInfo(SeqLibraryLink *link) :
	link(link),
	progress(0)
{
	SDL_AtomicIncRef(&link->useCount);
}

SeqTaskReadVideoInfo::~SeqTaskReadVideoInfo()
{
}

void SeqTaskReadVideoInfo::Start()
{
	resultVideoInfo = new SeqVideoInfo();
	SeqDecoder::ReadVideoInfo(link->fullPath, resultVideoInfo);
	progress = 1;
}

void SeqTaskReadVideoInfo::Stop()
{
}

void SeqTaskReadVideoInfo::Finalize()
{
	if (link->info != nullptr)
		delete link->info;
	link->info = resultVideoInfo;
	SDL_AtomicDecRef(&link->useCount);
	// we're done here remove self
	delete this;
}

SeqWorkerTaskPriority SeqTaskReadVideoInfo::GetPriority()
{
	return SeqWorkerTaskPriority::Medium;
}

float SeqTaskReadVideoInfo::GetProgress()
{
	return progress;
}
