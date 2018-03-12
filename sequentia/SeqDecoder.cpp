#include "SeqDecoder.h"
#include "SeqList.h"
#include "SeqTime.h"
#include <SDL.h>

SeqDecoder::SeqDecoder() :
	formatContext(nullptr),
	streamContexts(nullptr),
	packetBufferSize(defaultPacketBufferSize),
	packetBuffer(nullptr)
{
	packetBuffer = new AVPacket[packetBufferSize];
	frameBuffers = new SeqList<SeqFrameBuffer*>(2);
	startStreams = new SeqList<int>();
	stopStreams = new SeqList<int>();

	statusMutex = SDL_CreateMutex();
	seekMutex = SDL_CreateMutex();
	refreshStreamContextsMutex = SDL_CreateMutex();
}

SeqDecoder::~SeqDecoder()
{
	Dispose();
	delete startStreams;
	delete stopStreams;
	delete frameBuffers;
	delete[] streamContexts;
	delete[] packetBuffer;
	SDL_DestroyMutex(statusMutex);
	SDL_DestroyMutex(seekMutex);
	SDL_DestroyMutex(refreshStreamContextsMutex);
}

SeqDecoderStatus SeqDecoder::GetStatus()
{
	return status;
}

void SeqDecoder::SetStatusInactive()
{
	SDL_LockMutex(statusMutex);
	status = SeqDecoderStatus::Inactive;
	SDL_UnlockMutex(statusMutex);
}

void SeqDecoder::SetStatusOpening()
{
	SDL_LockMutex(statusMutex);
	if (status < SeqDecoderStatus::Stopping)
		status = SeqDecoderStatus::Opening;
	SDL_UnlockMutex(statusMutex);
}

void SeqDecoder::SetStatusLoading()
{
	SDL_LockMutex(statusMutex);
	if (status < SeqDecoderStatus::Stopping)
		status = SeqDecoderStatus::Loading;
	SDL_UnlockMutex(statusMutex);
}

void SeqDecoder::SetStatusReady()
{
	SDL_LockMutex(statusMutex);
	if (status < SeqDecoderStatus::Stopping)
		status = SeqDecoderStatus::Ready;
	SDL_UnlockMutex(statusMutex);
}

void SeqDecoder::SetStatusStopping()
{
	SDL_LockMutex(statusMutex);
	if (status < SeqDecoderStatus::Disposing)
		status = SeqDecoderStatus::Stopping;
	SDL_UnlockMutex(statusMutex);
}

void SeqDecoder::SetStatusDisposing()
{
	SDL_LockMutex(statusMutex);
	if (status == SeqDecoderStatus::Stopping)
		status = SeqDecoderStatus::Disposing;
	else
		SetStatusStopping();
	SDL_UnlockMutex(statusMutex);
}

int64_t SeqDecoder::GetDuration()
{
	return formatContext->duration;
}

int64_t SeqDecoder::GetPlaybackTime()
{
	return lastReturnedFrameTime;
}

int64_t SeqDecoder::GetBufferLeft()
{
	SeqStreamContext *streamContext = &streamContexts[primaryStreamIndex];
	SeqFrameBuffer * frameBuffer = streamContext->frameBuffer;
	return STREAM_TIME_TO_SEQ_TIME(SDL_max(frameBuffer->buffer[frameBuffer->inUseCursor]->pkt_dts, 0), streamContext->timeBase);
}

int64_t SeqDecoder::GetBufferRight()
{
	SeqStreamContext *streamContext = &streamContexts[primaryStreamIndex];
	SeqFrameBuffer * frameBuffer = streamContext->frameBuffer;
	return STREAM_TIME_TO_SEQ_TIME(SDL_max(frameBuffer->buffer[(frameBuffer->insertCursor - 1 + frameBuffer->size) % frameBuffer->size]->pkt_dts, 0), streamContext->timeBase);
}

void SeqDecoder::Dispose()
{
	if (status != SeqDecoderStatus::Disposing && status != SeqDecoderStatus::Inactive)
	{
		SetStatusDisposing();
		for (int i = 0; i < packetBufferSize; ++i)
			av_packet_unref(&packetBuffer[i]);
		for (int i = 0; i < frameBuffers->Count(); i++)
			DisposeFrameBuffer(frameBuffers->Get(i));
		for (int i = 0; i < formatContext->nb_streams; i++)
			DisposeStreamContext(&streamContexts[i]);
		avformat_close_input(&formatContext);
		SetStatusInactive();
	}
}

SeqFrameBuffer* SeqDecoder::CreateFrameBuffer(int size)
{
	SeqFrameBuffer *buffer = new SeqFrameBuffer();
	buffer->size = size;
	buffer->buffer = new AVFrame*[size];
	buffer->inUseCursor = size - 1;
	for (int i = 0; i < size; i++)
	{
		buffer->buffer[i] = av_frame_alloc();
		SDL_assert_always(buffer->buffer[i] != nullptr);
	}
	return buffer;
}

void SeqDecoder::DisposeFrameBuffer(SeqFrameBuffer *buffer)
{
	for (int i = 0; i < buffer->size; i++)
		av_frame_free(&buffer->buffer[i]);
	delete[] buffer->buffer;
}

void SeqDecoder::DisposeStreamContext(SeqStreamContext *context)
{
	if (context->codecContext != nullptr)
		avcodec_free_context(&context->codecContext);
}

int SeqDecoder::OpenFormatContext(const char *fullPath, AVFormatContext **formatContext)
{
	// open input file, and allocate format context
	if (avformat_open_input(formatContext, fullPath, nullptr, nullptr) < 0)
	{
		fprintf(stderr, "Could not open source file %s\n", fullPath);
		return 1;
	}

	// retrieve stream information
	if (avformat_find_stream_info(*formatContext, nullptr) < 0)
	{
		fprintf(stderr, "Could not find stream information\n");
		return 1;
	}

	// dump input information to stderr
	av_dump_format(*formatContext, 0, fullPath, 0);

	return 0;
}

void SeqDecoder::CloseFormatContext(AVFormatContext **formatContext)
{
	avformat_close_input(formatContext);
}

int SeqDecoder::OpenCodecContext(int streamIndex, AVFormatContext *format, AVCodec **outCodec, AVCodecContext **context, double *timeBase)
{
	int ret;
	AVStream *stream;

	stream = format->streams[streamIndex];
	AVMediaType type = stream->codecpar->codec_type;

	// find decoder for the stream
	AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
	if (!codec)
	{
		fprintf(stderr, "Failed to find %s codec\n",
			av_get_media_type_string(type));
		return AVERROR(EINVAL);
	}
	if (outCodec != nullptr)
		*outCodec = codec;

	// Allocate a codec context for the decoder
	*context = avcodec_alloc_context3(codec);
	if (!*context)
	{
		fprintf(stderr, "Failed to allocate the %s codec context\n",
			av_get_media_type_string(type));
		return AVERROR(ENOMEM);
	}

	// Copy codec parameters from input stream to output codec context
	if ((ret = avcodec_parameters_to_context(*context, stream->codecpar)) < 0)
	{
		fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
			av_get_media_type_string(type));
		return ret;
	}

	// return timebase
	*timeBase = av_q2d(format->streams[streamIndex]->time_base);

	return 0;
}

void SeqDecoder::CloseCodecContext(AVCodecContext **codecContext)
{
	avcodec_free_context(codecContext);
}

int SeqDecoder::Preload()
{
	SetStatusOpening();

	// create a stream index to stream context conversion table
	streamContexts = new SeqStreamContext[formatContext->nb_streams];

	// create stream context and buffers
	SDL_LockMutex(refreshStreamContextsMutex);
	RefreshStreamContexts();
	SDL_UnlockMutex(refreshStreamContextsMutex);

	// create packet buffer
	for (int i = 0; i < packetBufferSize; ++i)
	{
		AVPacket* pkt = &packetBuffer[i];
		av_init_packet(pkt);
		pkt->data = nullptr;
		pkt->size = 0;
	}

	SetStatusLoading();
	// prefetch a bunch of packets
	packetBufferCursor = 0;
	FillPacketBuffer();
	return 0;
}

int SeqDecoder::Loop()
{
	bool hasSkippedVideoFrame = false;
	// allocate temp frame
	AVFrame* tempFrame = nullptr;
	tempFrame = av_frame_alloc();
	if (!tempFrame)
	{
		fprintf(stderr, "Could not allocate temp frame\n");
		Dispose();
		return AVERROR(ENOMEM);
	}
	// allocate temp packet
	AVPacket pkt;
	av_init_packet(&pkt);
	// read frames from the file
	while (status == SeqDecoderStatus::Loading || status == SeqDecoderStatus::Ready)
	{
		// switching codex, when for example switching audio channel or removing an audio channel all together
		if (shouldRefreshStreamContexts)
		{
			SDL_LockMutex(refreshStreamContextsMutex);
			shouldRefreshStreamContexts = false;
			RefreshStreamContexts();
			startStreams->Clear();
			stopStreams->Clear();
			SDL_UnlockMutex(refreshStreamContextsMutex);
		}
		// seeking
		if (shouldSeek)
		{
			SDL_LockMutex(seekMutex);
			shouldSeek = false;
			int64_t tempSeekTime = seekTime;
			SDL_UnlockMutex(seekMutex);
			// ffmpeg seek
			avformat_seek_file(formatContext, primaryStreamIndex, 0, tempSeekTime, tempSeekTime, 0);
			// set state to 'loading' as the complete buffer is now invalid
			SetStatusLoading();
			// reset the packet and frame buffer so it will completely be refilled
			displayPacketCursor = 0;
			packetBufferCursor = 0;
			for (int i = 0; i < frameBuffers->Count(); i++)
			{
				SeqFrameBuffer *frameBuffer = frameBuffers->Get(i);
				frameBuffer->insertCursor = (frameBuffer->inUseCursor + 1) % frameBuffer->size;
			}
			lastRequestedFrameTime = tempSeekTime;
			printf("SEEKING time:%d lb: %d rb: %d disp_pkt:%d pkt_cur:%d\n", 
				seekTime, GetBufferLeft(), GetBufferRight(), displayPacketCursor, packetBufferCursor);
		}
		// fill the packet buffer
		FillPacketBuffer();

		// decode the next packet
		displayPacketCursor = (displayPacketCursor + 1) % packetBufferSize;
		pkt = packetBuffer[displayPacketCursor];

		// skip the packet if:
		// - there is no data in the packet
		if (pkt.buf == nullptr)
		{
			printf("skipping frame. pts:%d flags:%d size:%d NO DATA!\n", pkt.pts, pkt.flags, pkt.size);
			continue;
		}

		SeqStreamContext *streamContext = &streamContexts[pkt.stream_index];

		// - it's from the wrong stream
		if (streamContext->codecContext == nullptr)
		{
			//printf("skipping frame. pts:%d flags:%d size:%d AUDIO PACKET!\n", pkt.pts, pkt.flags, pkt.size);
			continue;
		}
		// - we need to skip to keep up and it's not the yet the next key frame
		if (skipFramesIfSlow && (IsSlowAndShouldSkip() || hasSkippedVideoFrame) && ((pkt.flags & AV_PKT_FLAG_KEY) == 0 || pkt.dts < lastRequestedFrameTime))
		{
			printf("skipping frame. pts:%d flags:%d size:%d SLOW!\n", pkt.pts, pkt.flags, pkt.size);
			//printf("  sfis=%d && (isass=%d || hskf=%d) && (isNotKey=%d || isLate=%d)\n",
			//	skip_frames_if_slow, is_slow_and_should_skip(), has_skipped_video_frame,
			//	(pkt.flags & AV_PKT_FLAG_KEY) == 0, pkt.dts < last_requested_frame_time);
			hasSkippedVideoFrame = true;
			continue;
		}
		else
		{
			//printf("DECODING frame. pts:%d flags:%d size:%d\n", pkt.pts, pkt.flags, pkt.size);
			//printf("  sfis=%d && (isass=%d || hskf=%d) && (isNotKey=%d || isLate=%d)\n",
			//	skip_frames_if_slow, is_slow_and_should_skip(), has_skipped_video_frame,
			//	(pkt.flags & AV_PKT_FLAG_KEY) == 0, pkt.dts < last_requested_frame_time);
		}

		// reset has_skipped_video_frame
		hasSkippedVideoFrame = false;

		// decode packet
		int ret = 0;
		int frameIndex = 0;

		// read all data in the packet 
		while (pkt.size > 0)
		{
			ret = DecodePacket(pkt, tempFrame, &frameIndex, 0);
			if (ret < 0)
			{
				char error[256];
				av_strerror(ret, error, 256);
				printf("error decoding frame: %s\n", error);
				break;
			}
			pkt.data += ret;
			pkt.size -= ret;
		}

		// skip the frame if:
		if (frameIndex <= 0 || // the packet did not contain a frame
			tempFrame->pkt_dts < 0) // the frame should not be presented
		{
			continue;
		}

		// wait until we have room in the buffer
		SeqFrameBuffer *frameBuffer = streamContext->frameBuffer;
		int nextInsertCursor = (frameBuffer->insertCursor + 1) % frameBuffer->size;
		while (nextInsertCursor == frameBuffer->inUseCursor)
		{
			if (status == SeqDecoderStatus::Stopping)
				break;
			if (status == SeqDecoderStatus::Loading)
				SetStatusReady();
			SDL_Delay(5);
			if (shouldSeek)
				break;
		}

		// if we're going to seek we can forget about the current frame buffer
		if (shouldSeek)
			continue;

		// put the new frame in the buffer
		AVFrame* swapFrame = frameBuffer->buffer[frameBuffer->insertCursor];
		frameBuffer->buffer[frameBuffer->insertCursor] = tempFrame;
		tempFrame = swapFrame;
		bufferPunctuality = frameBuffer->buffer[frameBuffer->insertCursor]->pkt_dts - lastRequestedFrameTime;

		// move the buffer cursor
		frameBuffer->insertCursor = nextInsertCursor;

		// if this decoder is done, stop the buffering
		if (status == SeqDecoderStatus::Stopping)
			break;
	}

	// disposing...
	av_frame_free(&tempFrame);
	Dispose();

	return 0;
}

void SeqDecoder::Stop()
{
	SetStatusStopping();
}

void SeqDecoder::Seek(int64_t time)
{
	SeqStreamContext *streamContext = &streamContexts[primaryStreamIndex];
	int64_t streamTime = SEQ_TIME_TO_STREAM_TIME(time, streamContext->timeBase);
	SeqFrameBuffer *frameBuffer = streamContext->frameBuffer;
	// if searching forwards	
	if (streamTime > frameBuffer->buffer[frameBuffer->inUseCursor]->pkt_dts)
	{
		// first check if the location we're seeking for is already buffered
		int nextInUseCursor = (frameBuffer->inUseCursor + 1) % frameBuffer->size;
		int latest_frame = frameBuffer->inUseCursor;
		int64_t latest_dts = -1;
		while (nextInUseCursor != frameBuffer->insertCursor &&
			frameBuffer->buffer[frameBuffer->inUseCursor]->pkt_dts < streamTime)
		{
			if (frameBuffer->buffer[nextInUseCursor]->pkt_dts > latest_dts)
			{
				latest_dts = frameBuffer->buffer[nextInUseCursor]->pkt_dts;
				latest_frame = nextInUseCursor;
			}
			nextInUseCursor = (nextInUseCursor + 1) % frameBuffer->size;
		}
		// did we find the requested frame in the buffer?
		if (frameBuffer->buffer[frameBuffer->inUseCursor]->pkt_dts < streamTime &&
			frameBuffer->buffer[nextInUseCursor]->pkt_dts > streamTime)
		{
			// the frame was in the buffer! we're done here!
			return;
		}
	}

	// if the seek time was not in the buffer:
	// schedule a ffmpeg seek which will later be done in loop()
	SDL_LockMutex(seekMutex);
	shouldSeek = true;
	seekTime = streamTime;
	SDL_UnlockMutex(seekMutex);
}

AVFrame* SeqDecoder::NextFrame(int streamIndex, int64_t time)
{
	SeqStreamContext *streamContext = &streamContexts[streamIndex];
	SeqFrameBuffer *frameBuffer = streamContext->frameBuffer;
	int64_t streamTime = SEQ_TIME_TO_STREAM_TIME(time, streamContext->timeBase);
	// remember in the decoder what time was last requested, we can use this for buffering more relevant frames
	lastRequestedFrameTime = streamTime;
	// only look up to the frame buffer cursor, the decoder thread may be writing to the buffer_cursor index right now
	int oneBeforeInsertCursor = (frameBuffer->insertCursor - 1 + frameBuffer->size) % frameBuffer->size;
	// start looking from the last frame we displayed
	int candidateDisplayFrame = frameBuffer->inUseCursor;
	// figure out the first candidate's frame time
	int64_t candidateFrameDeltaTime = 0;
	if (frameBuffer->buffer[candidateDisplayFrame])
		candidateFrameDeltaTime = abs(frameBuffer->buffer[candidateDisplayFrame]->pkt_dts - streamTime);
	int64_t lowestDeltaTime = candidateFrameDeltaTime;
	// go through all frame in the buffer until we found one where the requested time is smaller than the frame time
	while (candidateDisplayFrame != oneBeforeInsertCursor)
	{
		if (lowestDeltaTime < 0 ||
			candidateFrameDeltaTime < lowestDeltaTime)
		{
			lowestDeltaTime = candidateFrameDeltaTime;
			frameBuffer->inUseCursor = candidateDisplayFrame;
		}
		candidateDisplayFrame = (candidateDisplayFrame + 1) % frameBuffer->size;
		candidateFrameDeltaTime = abs(frameBuffer->buffer[candidateDisplayFrame]->pkt_dts - streamTime);
	}
	// frame to return
	AVFrame *frame = frameBuffer->buffer[frameBuffer->inUseCursor];
	// set playback time
	lastReturnedFrameTime = STREAM_TIME_TO_SEQ_TIME(frame->pkt_dts, streamContext->timeBase);
	// return the best suited display frame
	return frame;
}

AVFrame* SeqDecoder::NextFrame(int streamIndex)
{
	// simply return the next display frame in the buffer
	SeqStreamContext *streamContext = &streamContexts[streamIndex];
	SeqFrameBuffer *frameBuffer = streamContext->frameBuffer;
	int nextInUseCursor = (frameBuffer->inUseCursor + 1) % frameBuffer->size;
	if (nextInUseCursor != frameBuffer->insertCursor)
	{
		frameBuffer->inUseCursor = nextInUseCursor;
	}
	AVFrame *frame = frameBuffer->buffer[frameBuffer->inUseCursor];
	lastReturnedFrameTime = STREAM_TIME_TO_SEQ_TIME(frame->pkt_dts, streamContext->timeBase);
	return frame;
}

void SeqDecoder::StartDecodingStream(int streamIndex)
{
	SDL_LockMutex(refreshStreamContextsMutex);
	int index = stopStreams->IndexOf(streamIndex);
	if (index > -1)
		stopStreams->RemoveAt(index);
	if (streamContexts != nullptr && streamContexts[streamIndex].codecContext != nullptr)
		return;
	if (startStreams->IndexOf(streamIndex) > -1)
		return;
	startStreams->Add(streamIndex);
	shouldRefreshStreamContexts = true;
	SDL_UnlockMutex(refreshStreamContextsMutex);
}

void SeqDecoder::StopDecodingStream(int streamIndex)
{
	SDL_LockMutex(refreshStreamContextsMutex);
	int index = startStreams->IndexOf(streamIndex);
	if (index > -1)
		startStreams->RemoveAt(index);
	if (streamContexts != nullptr && streamContexts[streamIndex].codecContext == nullptr)
		return;
	if (stopStreams->IndexOf(streamIndex) > -1)
		return;
	stopStreams->Add(streamIndex);
	shouldRefreshStreamContexts = true;
	SDL_UnlockMutex(refreshStreamContextsMutex);
}

bool SeqDecoder::IsValidFrame(AVFrame *frame)
{
	return frame->pkt_dts >= 0;
}

void SeqDecoder::FillPacketBuffer()
{
	int ret = 0;
	int nextPacketBufferCursor = (packetBufferCursor + 1) % packetBufferSize;
	while (nextPacketBufferCursor != displayPacketCursor && ret >= 0)
	{
		packetBufferCursor = nextPacketBufferCursor;
		ret = av_read_frame(formatContext, &packetBuffer[packetBufferCursor]);
		if (ret >= 0)
			nextPacketBufferCursor = (packetBufferCursor + 1) % packetBufferSize;
	}
}

bool SeqDecoder::IsSlowAndShouldSkip()
{
	int64_t nextKeyFrame;
	SeqFrameBuffer *frameBuffer = streamContexts[primaryStreamIndex].frameBuffer;
	if (NextKeyFrameDts(&nextKeyFrame))
		return bufferPunctuality < FFMAX(0, frameBuffer->buffer[frameBuffer->inUseCursor]->pkt_dts) - nextKeyFrame + lowestKeyFrameDecodeTime;
	else if (bufferPunctuality < 0)
		return true;
	else
		return false;
}

bool SeqDecoder::NextKeyFrameDts(int64_t *result)
{
	for (int i = 1; i < packetBufferSize; ++i)
	{
		AVPacket* pkt = &packetBuffer[(displayPacketCursor + i) % packetBufferSize];
		if (pkt->stream_index == primaryStreamIndex && (pkt->flags & AV_PKT_FLAG_KEY) > 0)
		{
			*result = pkt->dts;
			return true;
		}
	}
	// didn't find a suitable keyframe
	return false;
}

int SeqDecoder::DecodePacket(AVPacket pkt, AVFrame *target, int *frameIndex, int cached)
{
	int ret = 0;
	int decoded = pkt.size;
	SeqStreamContext *streamContext = &streamContexts[pkt.stream_index];
	SDL_assert(streamContext->codecContext != nullptr);
	if (streamContext->codecContext == nullptr)
		return decoded;
	*frameIndex = 0;
	if (streamContext->codecContext->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO)
	{
		// keep track of frame decoding performance
		Uint32 decoding_start_time = SDL_GetTicks();
		// decode video frame
		ret = avcodec_decode_video2(streamContext->codecContext, target, frameIndex, &pkt);
		if (ret < 0)
		{
			PrintAVError("Error decoding video frame (%s)\n", ret);
			return ret;
		}
		if (*frameIndex)
		{
			if (target->width != streamContext->codecContext->width ||
				target->height != streamContext->codecContext->height ||
				target->format != streamContext->codecContext->pix_fmt)
			{
				// To handle this change, one could call av_image_alloc again and
				// decode the following frames into another rawvideo file.
				fprintf(stderr, "Error: Width, height and pixel format have to be "
					"constant in a rawvideo file, but the width, height or "
					"pixel format of the input video changed:\n"
					"old: width = %d, height = %d, format = %s\n"
					"new: width = %d, height = %d, format = %s\n",
					streamContext->codecContext->width, streamContext->codecContext->height, 
					av_get_pix_fmt_name(streamContext->codecContext->pix_fmt),
					target->width, target->height, 
					av_get_pix_fmt_name((AVPixelFormat)target->format));
				return -1;
			}

			// store the lowest keyframe decoding time, which we can use as estimate for timely skipping to
			// a next keyframe later.
			if (target->key_frame)
			{
				Uint32 decodingTime = SDL_GetTicks() - decoding_start_time;
				if (lowestKeyFrameDecodeTime == 0 || decodingTime < lowestKeyFrameDecodeTime)
					lowestKeyFrameDecodeTime = decodingTime;
			}

			printf("video_frame%s n:%d coded_n:%d isKey:%d flags:%d dts:%d pts:%d lrft:%d\n",
				cached ? "(cached)" : "",
				streamContext->frameCount++, target->coded_picture_number,
				target->key_frame, pkt.flags, pkt.dts, pkt.pts, 
				STREAM_TIME_TO_SEQ_TIME(lastRequestedFrameTime / 1000, streamContext->timeBase));
		}
	}
	else if (streamContext->codecContext->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO)
	{
		/* decode audio frame */
		ret = avcodec_decode_audio4(streamContext->codecContext, target, frameIndex, &pkt);
		if (ret < 0)
		{
			PrintAVError("Error decoding audio frame (%s)\n", ret);
			return ret;
		}
		// Some audio decoders decode only part of the packet, and have to be
		// called again with the remainder of the packet data.
		// Sample: fate-suite/lossless-audio/luckynight-partial.shn
		// Also, some decoders might over-read the packet.
		decoded = FFMIN(ret, pkt.size);
		if (*frameIndex)
		{
			size_t unpadded_linesize = target->nb_samples * av_get_bytes_per_sample((AVSampleFormat)target->format);
			printf("audio_frame%s n:%d nb_samples:%d pts:%d\n",
				cached ? "(cached)" : "",
				streamContext->frameCount++, target->nb_samples,
				STREAM_TIME_TO_SEQ_TIME(target->pts, streamContext->timeBase));
		}
	}
	return decoded;
}

void SeqDecoder::RefreshStreamContexts()
{
	// stop streams
	for (int i = 0; i < stopStreams->Count(); i++)
	{
		// get the stream context for the stream index
		int streamIndex = stopStreams->Get(i);
		SeqStreamContext *streamContext = &streamContexts[streamIndex];

		// discard primary stream context if removed
		if (primaryStreamIndex == streamIndex)
			primaryStreamIndex = -1;

		// free the AVCodecContext
		avcodec_free_context(&streamContext->codecContext);
		// reset the associated frame buffer for later re-use.
		ResetFrameBuffer(streamContext->frameBuffer);
		// dispose the codec context
		DisposeStreamContext(streamContext);
	}

	// prefer old already used streams context over new ones for primary 
	RefreshPrimaryStreamContext();

	// start new streams
	for (int i = 0; i < startStreams->Count(); i++)
	{
		int streamIndex = startStreams->Get(i);
		// get an old, or create a new framebufferi
		int frameBufferIndex = NextFrameBuffer();
		// activate a SeqStreamContext
		SeqStreamContext *streamContext = &streamContexts[streamIndex];
		streamContext->frameBuffer = frameBuffers->Get(frameBufferIndex);
		streamContext->frameBuffer->usedBy = streamContext;
		// create codec context
		AVCodec *decoder = nullptr;
		OpenCodecContext(streamIndex, formatContext, &decoder, &streamContext->codecContext, &streamContext->timeBase);
		AVStream *stream = formatContext->streams[streamIndex];
		AVDictionary *opts = nullptr;
		av_dict_set(&opts, "refcounted_frames", "1", 0);
		int ret = avcodec_open2(streamContext->codecContext, decoder, &opts);
		if (ret < 0)
		{
			fprintf(stderr, "Failed to open %s codec\n",
				av_get_media_type_string(stream->codecpar->codec_type));
		}
	}

	// if we didn't find a primary stream context previously, maybe we do now?
	RefreshPrimaryStreamContext();
}

void SeqDecoder::RefreshPrimaryStreamContext()
{
	if (primaryStreamIndex == -1 ||
		streamContexts[primaryStreamIndex].codecContext == nullptr ||
		streamContexts[primaryStreamIndex].codecContext->codec_type != AVMediaType::AVMEDIA_TYPE_VIDEO)
	{
		for (int i = 0; i < formatContext->nb_streams; i++)
		{
			AVCodecContext *codecContext = streamContexts[i].codecContext;
			if (codecContext != nullptr)
			{
				// prefer video over other stream types
				if (codecContext->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO)
				{
					primaryStreamIndex = i;
					return;
				}
				// otherwise just use the first valid one in the list
				else if (primaryStreamIndex == -1)
				{
					primaryStreamIndex = i;
				}
			}
		}
	}
}

int SeqDecoder::NextFrameBuffer()
{
	for (int i = 0; i < frameBuffers->Count(); i++)
	{
		if (frameBuffers->Get(i)->usedBy == nullptr)
			return i;
	}
	frameBuffers->Add(CreateFrameBuffer(defaultFrameBufferSize));
	return frameBuffers->Count() - 1;
}

void SeqDecoder::ResetFrameBuffer(SeqFrameBuffer *buffer)
{
	buffer->inUseCursor = buffer->size - 1;
	buffer->insertCursor = 0;
	buffer->usedBy = nullptr;
}

int SeqDecoder::GetBestStream(AVFormatContext *format, enum AVMediaType type, int *streamIndex)
{
	int ret = av_find_best_stream(format, type, -1, -1, nullptr, 0);
	if (ret < 0)
	{
		fprintf(stderr, "Could not find %s stream in input file'\n",
			av_get_media_type_string(type));
		return ret;
	}
	else
	{
		*streamIndex = ret;
		return 0;
	}
}

void SeqDecoder::PrintAVError(const char *message, int error)
{
	char buff[256];
	av_strerror(error, buff, 256);
	fprintf(stderr, message, buff);
}
