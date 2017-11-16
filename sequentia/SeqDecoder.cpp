#include "SeqDecoder.h";
#include "SeqVideoInfo.h";
#include <SDL.h>

SeqDecoder::SeqDecoder():
	videoInfo(nullptr),
	frameBufferSize(defaultFrameBufferSize),
	packetBufferSize(defaultPacketBufferSize),
	frameBuffer(nullptr),
	packetBuffer(nullptr)
{
	packetBuffer = new AVPacket[packetBufferSize];
	frameBuffer = new AVFrame*[frameBufferSize]();
}

SeqDecoder::~SeqDecoder()
{
	Dispose();
}

SeqDecoderStatus SeqDecoder::GetStatus()
{
	return status;
}

int64_t SeqDecoder::GetDuration()
{
	return videoInfo->formatContext->duration;
}

int64_t SeqDecoder::GetPlaybackTime()
{
	return lastRequestedFrameTime;
}

int64_t SeqDecoder::GetBufferTime()
{
	return 0;
}

void SeqDecoder::Dispose()
{
	if (status != SeqDecoderStatus::Disposing && status != SeqDecoderStatus::Inactive)
	{
		status = SeqDecoderStatus::Disposing;
		for (int i = 0; i < packetBufferSize; ++i)
			av_packet_unref(&packetBuffer[i]);
		for (int i = 0; i < frameBufferSize; ++i)
			av_frame_free(&frameBuffer[i]);
		av_free(videoDestData[0]);
		SDL_DestroyMutex(seekMutex);
		status = SeqDecoderStatus::Inactive;
	}
}

int SeqDecoder::ReadVideoInfo(char *fullPath, SeqVideoInfo *videoInfo)
{
	int frameIndex = 0;

	/* open input file, and allocate format context */
	if (avformat_open_input(&videoInfo->formatContext, fullPath, nullptr, nullptr) < 0)
	{
		fprintf(stderr, "Could not open source file %s\n", fullPath);
		return 1;
	}
	/* retrieve stream information */
	if (avformat_find_stream_info(videoInfo->formatContext, nullptr) < 0)
	{
		fprintf(stderr, "Could not find stream information\n");
		return 1;
	}
	if (OpenCodecContext(&videoInfo->videoStreamIndex, &videoInfo->videoCodec, videoInfo->formatContext, AVMEDIA_TYPE_VIDEO) >= 0)
	{
	}
	if (OpenCodecContext(&videoInfo->audioStreamIndex, &videoInfo->audioCodec, videoInfo->formatContext, AVMEDIA_TYPE_AUDIO) >= 0)
	{
	}
	/* dump input information to stderr */
	av_dump_format(videoInfo->formatContext, 0, fullPath, 0);

	return 0;
}

int SeqDecoder::Preload(SeqVideoInfo *info)
{
	videoInfo = info;
	status = SeqDecoderStatus::Opening;
	// create buffers
	if (frameBuffer[0] == nullptr)
	{
		displayFrameCursor = frameBufferSize - 1;
		seekMutex = SDL_CreateMutex();

		/* allocate image where the decoded image will be put */
		int ret = av_image_alloc(videoDestData, videoInfo->videoDestLinesize,
			videoInfo->videoCodec->width, videoInfo->videoCodec->height, videoInfo->videoCodec->pix_fmt, 1);
		if (ret < 0)
		{
			fprintf(stderr, "Could not allocate raw video buffer\n");
			Dispose();
			return 1;
		}
		videoInfo->videoDestBufferSize = ret;

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

	status = SeqDecoderStatus::Loading;
	// prefetch a bunch of packets
	packetBufferCursor = 0;
	FillPacketBuffer();
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
		// seeking
		if (shouldSeek)
		{
			SDL_LockMutex(seekMutex);
			shouldSeek = false;
			int64_t tempSeekTime = seekTime;
			SDL_UnlockMutex(seekMutex);
			// ffmpeg seek
			av_seek_frame(videoInfo->formatContext, videoInfo->videoStreamIndex, tempSeekTime, 0);
			// set state to 'loading' as the complete buffer is now invalid
			status = SeqDecoderStatus::Loading;
			// reset the packet and frame buffer so it will completely be refilled
			displayPacketCursor = 0;
			packetBufferCursor = 0;
			frameBufferCursor = (displayFrameCursor + 1) % frameBufferSize;
			lastRequestedFrameTime = tempSeekTime;
			printf("SEEKING disp_pkt:%d pkt_cur:%d disp_frm:%d frm_cur:%d\n", displayPacketCursor, packetBufferCursor, displayFrameCursor, frameBufferCursor);
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
		if (pkt.stream_index != videoInfo->videoStreamIndex)
		{
			//printf("skipping frame. pts:%d flags:%d size:%d AUDIO PACKET!\n", pkt.pts, pkt.flags, pkt.size);
			continue;
		}
		// - we need to skip to keep up and it's not the yet the next key frame
		if (skipFramesIfSlow && (IsSlowAndShouldSkip() || hasSkippedVideoFrame) && ((pkt.flags & AV_PKT_FLAG_KEY) == 0 || pkt.pts < lastRequestedFrameTime))
		{
			printf("skipping frame. pts:%d flags:%d size:%d SLOW!\n", pkt.pts, pkt.flags, pkt.size);
			//printf("  sfis=%d && (isass=%d || hskf=%d) && (isNotKey=%d || isLate=%d)\n",
			//	skip_frames_if_slow, is_slow_and_should_skip(), has_skipped_video_frame,
			//	(pkt.flags & AV_PKT_FLAG_KEY) == 0, pkt.pts < last_requested_frame_time);
			hasSkippedVideoFrame = true;
			continue;
		}
		else
		{
			//printf("DECODING frame. pts:%d flags:%d size:%d\n", pkt.pts, pkt.flags, pkt.size);
			//printf("  sfis=%d && (isass=%d || hskf=%d) && (isNotKey=%d || isLate=%d)\n",
			//	skip_frames_if_slow, is_slow_and_should_skip(), has_skipped_video_frame,
			//	(pkt.flags & AV_PKT_FLAG_KEY) == 0, pkt.pts < last_requested_frame_time);
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
			tempFrame->pts < 0) // the frame should not be presented
		{
			continue;
		}

		// wait until we have room in the buffer
		int nextFrameBufferCursor = (frameBufferCursor + 1) % frameBufferSize;
		while (nextFrameBufferCursor == displayFrameCursor)
		{
			if (status == SeqDecoderStatus::Loading)
				status = SeqDecoderStatus::Ready;
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
		bufferPunctuality = frameBuffer[frameBufferCursor]->pts - lastRequestedFrameTime;

		// move the buffer cursor
		frameBufferCursor = nextFrameBufferCursor;
	}

	// disposing...
	av_frame_free(&tempFrame);
	Dispose();

	return 0;
}

void SeqDecoder::Stop()
{
	status = SeqDecoderStatus::Stopping;
}

void SeqDecoder::Seek(int64_t time)
{
	// if searching forwards
	if (time > frameBuffer[displayFrameCursor]->pts)
	{
		// first check if the location we're seeking for is already buffered
		int nextDisplayFrameCursor = (displayFrameCursor + 1) % frameBufferSize;
		int latest_frame = displayFrameCursor;
		int64_t latest_pts = -1;
		while (nextDisplayFrameCursor != frameBufferCursor &&
			frameBuffer[displayFrameCursor]->pts < time)
		{
			if (frameBuffer[nextDisplayFrameCursor]->pts > latest_pts)
			{
				latest_pts = frameBuffer[nextDisplayFrameCursor]->pts;
				latest_frame = nextDisplayFrameCursor;
			}
			nextDisplayFrameCursor = (nextDisplayFrameCursor + 1) % frameBufferSize;
		}
		// did we find the requested frame in the buffer?
		if (frameBuffer[displayFrameCursor]->pts < time &&
			frameBuffer[nextDisplayFrameCursor]->pts > time)
		{
			// the frame was in the buffer! we're done here!
			return;
		}
	}

	// if the seek time was not in the buffer:
	// schedule a ffmpeg seek which will later be done in loop()
	SDL_LockMutex(seekMutex);
	shouldSeek = true;
	seekTime = time;
	SDL_UnlockMutex(seekMutex);
}

AVFrame* SeqDecoder::NextFrame(int64_t time)
{
	// remember in the decoder what time was last requested, we can use this for buffering more relevant frames
	lastRequestedFrameTime = time;
	// only look up to the frame buffer cursor, the decoder thread may be writing to the buffer_cursor index right now
	int oneBeforeFrameBufferCursor = (frameBufferCursor - 1 + frameBufferSize) % frameBufferSize;
	// start looking from the last frame we displayed
	int candidateFrameCursor = displayFrameCursor;
	// figure out the first candidate's frame time
	int64_t candidateFrameDeltaTime = 0;
	if (frameBuffer[candidateFrameCursor])
		candidateFrameDeltaTime = abs(frameBuffer[candidateFrameCursor]->pts - time);
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
		candidateFrameDeltaTime = abs(frameBuffer[candidateFrameCursor]->pts - time);
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

void SeqDecoder::FillPacketBuffer()
{
	int ret = 0;
	int nextPacketBufferCursor = (packetBufferCursor + 1) % packetBufferSize;
	while (nextPacketBufferCursor != displayPacketCursor && ret >= 0)
	{
		packetBufferCursor = nextPacketBufferCursor;
		ret = av_read_frame(videoInfo->formatContext, &packetBuffer[packetBufferCursor]);
		if (ret >= 0)
			nextPacketBufferCursor = (packetBufferCursor + 1) % packetBufferSize;
	}
}

bool SeqDecoder::IsSlowAndShouldSkip()
{
	int64_t nextKeyFrame;
	if (NextKeyFramePts(&nextKeyFrame))
		return bufferPunctuality < FFMAX(0, frameBuffer[displayFrameCursor]->pts) - nextKeyFrame + lowestKeyFrameDecodeTime;
	else if (bufferPunctuality < 0)
		return true;
	else
		return false;
}

bool SeqDecoder::NextKeyFramePts(int64_t *result)
{
	for (int i = 1; i < packetBufferSize; ++i)
	{
		AVPacket* pkt = &packetBuffer[(displayPacketCursor + i) % packetBufferSize];
		if (pkt->stream_index == videoInfo->videoStreamIndex && pkt->flags & AV_PKT_FLAG_KEY > 0)
		{
			*result = pkt->pts;
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
	if (pkt.stream_index == videoInfo->videoStreamIndex)
	{
		// keep track of frame decoding performance
		Uint32 decoding_start_time = SDL_GetTicks();
		// decode video frame
		ret = avcodec_decode_video2(videoInfo->videoCodec, target, frameIndex, &pkt);
		if (ret < 0)
		{
			PrintAVError("Error decoding video frame (%s)\n", ret);
			return ret;
		}
		if (*frameIndex)
		{
			if (target->width != videoInfo->videoCodec->width || 
				target->height != videoInfo->videoCodec->height ||
				target->format != videoInfo->videoCodec->pix_fmt)
			{
				// To handle this change, one could call av_image_alloc again and
				// decode the following frames into another rawvideo file.
				fprintf(stderr, "Error: Width, height and pixel format have to be "
					"constant in a rawvideo file, but the width, height or "
					"pixel format of the input video changed:\n"
					"old: width = %d, height = %d, format = %s\n"
					"new: width = %d, height = %d, format = %s\n",
					videoInfo->videoCodec->width, videoInfo->videoCodec->height, av_get_pix_fmt_name(videoInfo->videoCodec->pix_fmt),
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

			printf("video_frame%s n:%d coded_n:%d isKey:%d flags:%d dts:%.3f pts:%.3f lrft:%.3f\n",
				cached ? "(cached)" : "",
				videoInfo->videoFrameCount++, target->coded_picture_number,
				target->key_frame, pkt.flags, pkt.dts / 1000.0, pkt.pts / 1000.0, lastRequestedFrameTime / 1000.0);
		}
	}
	else if (pkt.stream_index == videoInfo->audioStreamIndex)
	{
		// decode audio frame
		ret = avcodec_decode_audio4(videoInfo->audioCodec, target, frameIndex, &pkt);
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
			size_t unpaddedLinesize = target->nb_samples * av_get_bytes_per_sample((AVSampleFormat)target->format);
			//char buff[256];
			//av_ts_make_time_string(buff, target->pts, &videoInfo.audioCodec->time_base);
			//printf("audio_frame%s n:%d nb_samples:%d pts:%s\n",
			//	cached ? "(cached)" : "",
			//	videoInfo.audioFrameCount++, target->nb_samples, buff);
		}
	}
	return decoded;
}

int SeqDecoder::OpenCodecContext(int *streamIndex, AVCodecContext **codec, AVFormatContext *format, enum AVMediaType type)
{
	int ret, streamId;
	AVStream *stream;
	AVCodec *decoder = nullptr;
	AVDictionary *opts = nullptr;
	ret = av_find_best_stream(format, type, -1, -1, nullptr, 0);
	if (ret < 0)
	{
		fprintf(stderr, "Could not find %s stream in input file'\n",
			av_get_media_type_string(type));
		return ret;
	}
	else
	{
		streamId = ret;
		stream = format->streams[streamId];

		/* find decoder for the stream */
		decoder = avcodec_find_decoder(stream->codecpar->codec_id);
		if (!decoder)
		{
			fprintf(stderr, "Failed to find %s codec\n",
				av_get_media_type_string(type));
			return AVERROR(EINVAL);
		}

		/* Allocate a codec context for the decoder */
		*codec = avcodec_alloc_context3(decoder);
		if (!*codec)
		{
			fprintf(stderr, "Failed to allocate the %s codec context\n",
				av_get_media_type_string(type));
			return AVERROR(ENOMEM);
		}

		/* Copy codec parameters from input stream to output codec context */
		if ((ret = avcodec_parameters_to_context(*codec, stream->codecpar)) < 0)
		{
			fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
				av_get_media_type_string(type));
			return ret;
		}

		/* Init the decoders, with or without reference counting */
		av_dict_set(&opts, "refcounted_frames", ffmpegRefcount ? "1" : "0", 0);
		if ((ret = avcodec_open2(*codec, decoder, &opts)) < 0)
		{
			fprintf(stderr, "Failed to open %s codec\n",
				av_get_media_type_string(type));
			return ret;
		}
		*streamIndex = streamId;
	}

	return 0;
}

void SeqDecoder::PrintAVError(char *message, int error)
{
	char buff[256];
	av_strerror(error, buff, 256);
	fprintf(stderr, message, buff);
}
