#include "SeqDecoder.h"
#include "SeqVideoContext.h"
#include "SeqTime.h"
#include <SDL.h>

SeqDecoder::SeqDecoder():
	context(nullptr),
	frameBufferSize(defaultFrameBufferSize),
	packetBufferSize(defaultPacketBufferSize),
	frameBuffer(nullptr),
	packetBuffer(nullptr)
{
	packetBuffer = new AVPacket[packetBufferSize];
	frameBuffer = new AVFrame*[frameBufferSize]();
	statusMutex = SDL_CreateMutex();
}

SeqDecoder::~SeqDecoder()
{
	Dispose();
	delete[] frameBuffer;
	delete[] packetBuffer;
	SDL_DestroyMutex(statusMutex);
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
	return context->formatContext->duration;
}

int64_t SeqDecoder::GetPlaybackTime()
{
	return STREAM_TIME_TO_SEQ_TIME(lastRequestedFrameTime, context->timeBase);
}

int64_t SeqDecoder::GetBufferLeft()
{
	return STREAM_TIME_TO_SEQ_TIME(SDL_max(frameBuffer[displayFrameCursor]->pkt_dts, 0), context->timeBase);
}

int64_t SeqDecoder::GetBufferRight()
{
	return STREAM_TIME_TO_SEQ_TIME(SDL_max(frameBuffer[(frameBufferCursor - 1 + frameBufferSize) % frameBufferSize]->pkt_dts, 0), context->timeBase);
}

void SeqDecoder::Dispose()
{
	if (status != SeqDecoderStatus::Disposing && status != SeqDecoderStatus::Inactive)
	{
		SetStatusDisposing();
		for (int i = 0; i < packetBufferSize; ++i)
			av_packet_unref(&packetBuffer[i]);
		for (int i = 0; i < frameBufferSize; ++i)
			av_frame_free(&frameBuffer[i]);
		av_frame_free(&audioFrame);
		av_free(videoDestData[0]);
		SDL_DestroyMutex(seekMutex);
		SetStatusInactive();
	}
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

int SeqDecoder::Preload(SeqVideoContext *videoContext)
{
	context = videoContext;
	RefreshCodecContexts();

	// create buffers
	SetStatusOpening();
	// create buffers
	if (frameBuffer[0] == nullptr)
	{
		displayFrameCursor = frameBufferSize - 1;
		seekMutex = SDL_CreateMutex();

		/* allocate image where the decoded image will be put */
		int ret = av_image_alloc(videoDestData, context->videoDestLinesize,
			context->videoCodec->width, context->videoCodec->height, context->videoCodec->pix_fmt, 1);
		if (ret < 0)
		{
			fprintf(stderr, "Could not allocate raw video buffer\n");
			Dispose();
			return 1;
		}
		context->videoDestBufferSize = ret;

		for (int i = 0; i < frameBufferSize; i++)
		{
			frameBuffer[i] = av_frame_alloc();
			if (!frameBuffer[i])
			{
				fprintf(stderr, "Could not allocate buffer frame\n");
				ret = AVERROR(ENOMEM);
				Dispose();
				return 1;
			}
		}
		audioFrame = av_frame_alloc();
		if (!audioFrame)
		{
			fprintf(stderr, "Could not allocate audio frame\n");
			ret = AVERROR(ENOMEM);
			Dispose();
			return 1;
		}

		/* initialize packets, set data to nullptr, let the demuxer fill it */
		for (int i = 0; i < packetBufferSize; ++i)
		{
			AVPacket* pkt = &packetBuffer[i];
			av_init_packet(pkt);
			pkt->data = nullptr;
			pkt->size = 0;
		}
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
		if (shouldRefreshCodex)
		{
			shouldRefreshCodex = false;
			RefreshCodecContexts();
		}
		// seeking
		if (shouldSeek)
		{
			SDL_LockMutex(seekMutex);
			shouldSeek = false;
			int64_t tempSeekTime = seekTime;
			SDL_UnlockMutex(seekMutex);
			// ffmpeg seek
			avformat_seek_file(context->formatContext, context->videoStreamIndex, 0, tempSeekTime, tempSeekTime, 0);
			// set state to 'loading' as the complete buffer is now invalid
			SetStatusLoading();
			// reset the packet and frame buffer so it will completely be refilled
			displayPacketCursor = 0;
			packetBufferCursor = 0;
			frameBufferCursor = (displayFrameCursor + 1) % frameBufferSize;
			lastRequestedFrameTime = tempSeekTime;
			printf("SEEKING time:%d lb: %d rb: %d disp_pkt:%d pkt_cur:%d disp_frm:%d frm_cur:%d\n", 
				seekTime, GetBufferLeft(), GetBufferRight(), displayPacketCursor, packetBufferCursor, displayFrameCursor, frameBufferCursor);
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
		// - it's from the wrong stream
		if (pkt.stream_index != videoContext->videoStreamIndex)
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
		int nextFrameBufferCursor = (frameBufferCursor + 1) % frameBufferSize;
		while (nextFrameBufferCursor == displayFrameCursor)
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
		AVFrame* swapFrame = frameBuffer[frameBufferCursor];
		frameBuffer[frameBufferCursor] = tempFrame;
		tempFrame = swapFrame;
		bufferPunctuality = frameBuffer[frameBufferCursor]->pkt_dts - lastRequestedFrameTime;

		// move the buffer cursor
		frameBufferCursor = nextFrameBufferCursor;

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
	int64_t streamTime = SEQ_TIME_TO_STREAM_TIME(time, context->timeBase);
	// if searching forwards
	if (streamTime > frameBuffer[displayFrameCursor]->pkt_dts)
	{
		// first check if the location we're seeking for is already buffered
		int nextDisplayFrameCursor = (displayFrameCursor + 1) % frameBufferSize;
		int latest_frame = displayFrameCursor;
		int64_t latest_dts = -1;
		while (nextDisplayFrameCursor != frameBufferCursor &&
			frameBuffer[displayFrameCursor]->pkt_dts < streamTime)
		{
			if (frameBuffer[nextDisplayFrameCursor]->pkt_dts > latest_dts)
			{
				latest_dts = frameBuffer[nextDisplayFrameCursor]->pkt_dts;
				latest_frame = nextDisplayFrameCursor;
			}
			nextDisplayFrameCursor = (nextDisplayFrameCursor + 1) % frameBufferSize;
		}
		// did we find the requested frame in the buffer?
		if (frameBuffer[displayFrameCursor]->pkt_dts < streamTime &&
			frameBuffer[nextDisplayFrameCursor]->pkt_dts > streamTime)
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

AVFrame* SeqDecoder::NextFrame(int64_t time)
{
	int64_t streamTime = SEQ_TIME_TO_STREAM_TIME(time, context->timeBase);
	// remember in the decoder what time was last requested, we can use this for buffering more relevant frames
	lastRequestedFrameTime = streamTime;
	// only look up to the frame buffer cursor, the decoder thread may be writing to the buffer_cursor index right now
	int oneBeforeFrameBufferCursor = (frameBufferCursor - 1 + frameBufferSize) % frameBufferSize;
	// start looking from the last frame we displayed
	int candidateFrameCursor = displayFrameCursor;
	// figure out the first candidate's frame time
	int64_t candidateFrameDeltaTime = 0;
	if (frameBuffer[candidateFrameCursor])
		candidateFrameDeltaTime = abs(frameBuffer[candidateFrameCursor]->pkt_dts - streamTime);
	int64_t lowestDeltaTime = candidateFrameDeltaTime;
	// go through all frame in the buffer until we found one where the requested time is smaller than the frame time
	while (candidateFrameCursor != oneBeforeFrameBufferCursor)
	{
		if (lowestDeltaTime < 0 ||
			candidateFrameDeltaTime < lowestDeltaTime)
		{
			lowestDeltaTime = candidateFrameDeltaTime;
			displayFrameCursor = candidateFrameCursor;
		}
		candidateFrameCursor = (candidateFrameCursor + 1) % frameBufferSize;
		candidateFrameDeltaTime = abs(frameBuffer[candidateFrameCursor]->pkt_dts - streamTime);
	}
	// return the best suited display frame
	return frameBuffer[displayFrameCursor];
}

AVFrame* SeqDecoder::NextFrame()
{
	// simply return the next display frame in the buffer
	int nextFrameCursor = (displayFrameCursor + 1) % frameBufferSize;
	if (nextFrameCursor != frameBufferCursor)
	{
		displayFrameCursor = nextFrameCursor;
	}
	return frameBuffer[displayFrameCursor];
}

void SeqDecoder::SetVideoStreamIndex(int videoStreamIndex)
{
	newVideoStreamIndex = videoStreamIndex;
	shouldRefreshCodex = true;
}

void SeqDecoder::SetAudioStreamIndex(int audioStreamIndex)
{
	newAudioStreamIndex = audioStreamIndex;
	shouldRefreshCodex = true;
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
		ret = av_read_frame(context->formatContext, &packetBuffer[packetBufferCursor]);
		if (ret >= 0)
			nextPacketBufferCursor = (packetBufferCursor + 1) % packetBufferSize;
	}
}

bool SeqDecoder::IsSlowAndShouldSkip()
{
	int64_t nextKeyFrame;
	if (NextKeyFrameDts(&nextKeyFrame))
		return bufferPunctuality < FFMAX(0, frameBuffer[displayFrameCursor]->pkt_dts) - nextKeyFrame + lowestKeyFrameDecodeTime;
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
		if (pkt->stream_index == context->videoStreamIndex && (pkt->flags & AV_PKT_FLAG_KEY) > 0)
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
	*frameIndex = 0;
	if (pkt.stream_index == context->videoStreamIndex)
	{
		// keep track of frame decoding performance
		Uint32 decoding_start_time = SDL_GetTicks();
		// decode video frame
		ret = avcodec_decode_video2(context->videoCodec, target, frameIndex, &pkt);
		if (ret < 0)
		{
			PrintAVError("Error decoding video frame (%s)\n", ret);
			return ret;
		}
		if (*frameIndex)
		{
			if (target->width != context->videoCodec->width || 
				target->height != context->videoCodec->height ||
				target->format != context->videoCodec->pix_fmt)
			{
				// To handle this change, one could call av_image_alloc again and
				// decode the following frames into another rawvideo file.
				fprintf(stderr, "Error: Width, height and pixel format have to be "
					"constant in a rawvideo file, but the width, height or "
					"pixel format of the input video changed:\n"
					"old: width = %d, height = %d, format = %s\n"
					"new: width = %d, height = %d, format = %s\n",
					context->videoCodec->width, context->videoCodec->height, av_get_pix_fmt_name(context->videoCodec->pix_fmt),
					target->width, target->height, av_get_pix_fmt_name((AVPixelFormat)target->format));
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
				context->videoFrameCount++, target->coded_picture_number,
				target->key_frame, pkt.flags, pkt.dts, pkt.pts, STREAM_TIME_TO_SEQ_TIME(lastRequestedFrameTime / 1000, context->timeBase));
		}
	}
	else if (pkt.stream_index == context->audioStreamIndex)
	{
		/* decode audio frame */
		ret = avcodec_decode_audio4(context->audioCodec, target, frameIndex, &pkt);
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
			//size_t unpaddedLinesize = target->nb_samples * av_get_bytes_per_sample((AVSampleFormat)target->format);
			//char buff[256];
			//av_ts_make_time_string(buff, target->pts, &videoInfo.audioCodec->time_base);
			//printf("audio_frame%s n:%d nb_samples:%d pts:%s\n",
			//	cached ? "(cached)" : "",
			//	videoInfo.audioFrameCount++, target->nb_samples, buff);
		}
	}
	return decoded;
}

void SeqDecoder::RefreshCodecContexts()
{
	// video codec
	if (context->videoCodec != nullptr && 
		(context->videoStreamIndex < 0 || context->videoStreamIndex != newVideoStreamIndex))
	{
		// free old video codec context
		avcodec_free_context(&context->videoCodec);
		context->videoCodec = nullptr;
	}
	if (newVideoStreamIndex > -1 &&
		(context->videoCodec == nullptr || context->videoStreamIndex != newVideoStreamIndex))
	{
		// create new video codec context
		AVCodec *decoder = nullptr;
		OpenCodecContext(newVideoStreamIndex, context->formatContext, &decoder, &context->videoCodec, &context->timeBase);
		AVStream *stream = context->formatContext->streams[newVideoStreamIndex];
		AVDictionary *opts = nullptr;
		av_dict_set(&opts, "refcounted_frames", "1", 0);
		int ret = avcodec_open2(context->videoCodec, decoder, &opts);
		if (ret < 0)
		{
			fprintf(stderr, "Failed to open %s codec\n",
				av_get_media_type_string(stream->codecpar->codec_type));
			newVideoStreamIndex = -1;
		}
	}
	context->videoStreamIndex = newVideoStreamIndex;

	// audio codec
	if (context->audioCodec != nullptr &&
		(context->audioStreamIndex < 0 || context->audioStreamIndex != newAudioStreamIndex))
	{
		// free old audio codec context
		avcodec_free_context(&context->audioCodec);
		context->audioCodec = nullptr;
	}
	if (newAudioStreamIndex > -1 &&
		(context->audioCodec == nullptr || context->audioStreamIndex != newAudioStreamIndex))
	{
		// create new audio codec context
		AVCodec *decoder = nullptr;
		OpenCodecContext(newAudioStreamIndex, context->formatContext, &decoder, &context->audioCodec, &context->timeBase);
		AVStream *stream = context->formatContext->streams[newAudioStreamIndex];
		AVDictionary *opts = nullptr;
		av_dict_set(&opts, "refcounted_frames", "1", 0);
		int ret = avcodec_open2(context->audioCodec, decoder, &opts);
		if (ret < 0)
		{
			fprintf(stderr, "Failed to open %s codec\n",
				av_get_media_type_string(stream->codecpar->codec_type));
			newAudioStreamIndex = -1;
		}
	}
	context->audioStreamIndex = newAudioStreamIndex;
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
