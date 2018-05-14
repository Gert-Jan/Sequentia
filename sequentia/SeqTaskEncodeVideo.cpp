#include "SeqTaskEncodeVideo.h"
#include "SeqLibrary.h"
#include "SeqEncoder.h"
#include "SeqString.h"
#include <SDL.h>

SeqTaskEncodeVideo::SeqTaskEncodeVideo() :
	error(0),
	done(false)
{
	encoder = new SeqEncoder();
}

SeqTaskEncodeVideo::~SeqTaskEncodeVideo()
{
}

void SeqTaskEncodeVideo::Start()
{
	// TODO: create format and codec contexts, write error
	// start decoding
	if (error == 0)
	{
		encoder->Loop();
	}

	while (!done)
		SDL_Delay(10);
}

void SeqTaskEncodeVideo::Stop()
{
	done = true;
}

void SeqTaskEncodeVideo::Finalize()
{
	delete encoder;
	// we're done here remove self
	delete this;
}

SeqEncoder* SeqTaskEncodeVideo::GetEncoder()
{
	return encoder;
}

SeqWorkerTaskPriority SeqTaskEncodeVideo::GetPriority()
{
	return SeqWorkerTaskPriority::High;
}

float SeqTaskEncodeVideo::GetProgress()
{
	if (encoder->GetStatus() == SeqEncoderStatus::Inactive)
	{
		return 0;
	}
	else if (encoder->GetStatus() == SeqEncoderStatus::Disposing)
	{
		return 1;
	}
	else
	{
		// TODO: estimate progress 
		return 0;
	}
}

char* SeqTaskEncodeVideo::GetName()
{
	SeqString::Temp->Clear();
	SeqString::Temp->Append("EncodeVideo");
	return SeqString::Temp->Buffer;
}
