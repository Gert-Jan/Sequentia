#include "SeqTaskDecodeVideo.h"
#include "SeqLibrary.h"
#include "SeqDecoder.h"
#include <SDL.h>

SeqTaskDecodeVideo::SeqTaskDecodeVideo(SeqLibraryLink *link):
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

void SeqTaskDecodeVideo::StartDecodeStreamIndex(int streamIndex)
{
	decoder->StartDecodingStream(streamIndex);
}

void SeqTaskDecodeVideo::StopDecodedStreamIndex(int streamIndex)
{
	decoder->StopDecodingStream(streamIndex);
}

void SeqTaskDecodeVideo::Start()
{
	while (!link->metaDataLoaded)
		SDL_Delay(10);

	// start decoding
	error = SeqDecoder::OpenFormatContext(link->fullPath, &decoder->formatContext);
	if (error == 0)
	{
		decoder->Preload();
		decoder->Loop();
	}

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
