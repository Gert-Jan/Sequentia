#include "SeqTaskDecodeVideo.h"
#include "SeqLibrary.h"
#include "SeqVideoContext.h"
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

void SeqTaskDecodeVideo::AddDecodeStreamIndex(int streamIndex)
{
	// TODO: decoder should get a list of decoder indexes instead of just dumping it on the video stream index
	decoder->SetVideoStreamIndex(streamIndex);
}

void SeqTaskDecodeVideo::Start()
{
	while (!link->metaDataLoaded)
		SDL_Delay(10);

	// start decoding
	SeqVideoContext *context = new SeqVideoContext();
	error = SeqDecoder::OpenFormatContext(link->fullPath, &context->formatContext);
	if (error == 0)
	{
		decoder->Preload(context);
		decoder->Loop();
	}

	// cleanup
	delete context;
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
