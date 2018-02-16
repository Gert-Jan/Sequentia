#include "SeqTaskDecodeVideo.h"
#include "SeqLibrary.h"
#include "SeqVideoInfo.h"
#include "SeqDecoder.h"
#include <SDL.h>

SeqTaskDecodeVideo::SeqTaskDecodeVideo(SeqLibraryLink *link) :
	link(link)
{
	SDL_AtomicIncRef(&link->useCount);
	decoder = new SeqDecoder();
}

SeqTaskDecodeVideo::~SeqTaskDecodeVideo()
{
}

SeqLibraryLink* SeqTaskDecodeVideo::GetLink()
{
	return link;
}

void SeqTaskDecodeVideo::Start()
{
	SeqVideoInfo *info = new SeqVideoInfo();
	error = SeqDecoder::ReadVideoInfo(link->fullPath, info);
	if (error == 0)
	{
		decoder->Preload(info);
		decoder->Loop();
	}
	delete info;
	while (!done)
		SDL_Delay(10);
}

void SeqTaskDecodeVideo::Stop()
{
	done = true;
	if (error == 0)
		decoder->Stop();
}

void SeqTaskDecodeVideo::Finalize()
{
	delete decoder;
	SDL_AtomicDecRef(&link->useCount);
	// we're done here remove self
	delete this;
}

SeqDecoder* SeqTaskDecodeVideo::GetDecoder()
{
	return decoder;
}

SeqWorkerTaskPriority SeqTaskDecodeVideo::GetPriority()
{
	return SeqWorkerTaskPriority::Medium;
}

float SeqTaskDecodeVideo::GetProgress()
{
	return (float)decoder->GetPlaybackTime() / (float)decoder->GetDuration();
}
