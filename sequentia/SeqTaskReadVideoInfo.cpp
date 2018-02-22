#include "SeqTaskReadVideoInfo.h"
#include "SeqLibrary.h"
#include "SeqDecoder.h"
#include "SeqVideoContext.h"
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
	resultVideoContext = new SeqVideoContext();
	SeqDecoder::OpenVideoContext(link->fullPath, resultVideoContext);
	progress = 1;
}

void SeqTaskReadVideoInfo::Stop()
{
}

void SeqTaskReadVideoInfo::Finalize()
{
	link->width = resultVideoContext->videoCodec->width;
	link->height = resultVideoContext->videoCodec->height;
	link->duration = resultVideoContext->formatContext->duration;
	link->metaDataLoaded = true;
	delete resultVideoContext;
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
