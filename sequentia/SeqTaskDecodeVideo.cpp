#include "SeqTaskDecodeVideo.h";
#include "SeqLibrary.h";
#include "SeqDecoder.h";
#include <SDL.h>

SeqTaskDecodeVideo::SeqTaskDecodeVideo(SeqLibraryLink *link) :
	link(link)
{
	SDL_AtomicIncRef(&link->useCount);
	decoder = new SeqDecoder(link);
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
	decoder->Preload();
	decoder->Loop();
}

void SeqTaskDecodeVideo::Stop()
{
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
