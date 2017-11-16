#include "SeqTaskDecodeVideo.h";
#include "SeqLibrary.h";
#include "SeqVideoInfo.h";
#include "SeqDecoder.h";
#include <SDL.h>

SeqTaskDecodeVideo::SeqTaskDecodeVideo(SeqLibraryLink *link) :
	link(link)
{
	SDL_AtomicIncRef(&link->useCount);
	mutex = SDL_CreateMutex();
	decoder = new SeqDecoder();
}

SeqTaskDecodeVideo::~SeqTaskDecodeVideo()
{
	SDL_DestroyMutex(mutex);
}

SeqLibraryLink* SeqTaskDecodeVideo::GetLink()
{
	return link;
}

void SeqTaskDecodeVideo::Start()
{
	SeqVideoInfo *info = new SeqVideoInfo();
	SDL_LockMutex(mutex);
	error = SeqDecoder::ReadVideoInfo(link->fullPath, info);
	SDL_UnlockMutex(mutex);
	if (error == 0)
	{
		decoder->Preload(info);
		decoder->Loop();
	}
	while (!done)
		SDL_Delay(10);
}

void SeqTaskDecodeVideo::Stop()
{
	done = true;
	SDL_LockMutex(mutex);
	if (error == 0)
		decoder->Stop();
	SDL_UnlockMutex(mutex);
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
