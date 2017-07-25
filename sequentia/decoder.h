// ffmpeg powered video decoder
extern "C"
{
	#include "libavformat/avformat.h"
	#include "libavcodec/avcodec.h"
	#include "libavutil/imgutils.h"
	#include "libavutil/samplefmt.h"
	#include "libavutil/timestamp.h"
}

enum DecoderStatus
{
	Inactive,
	Opening,
	Loading,
	Ready,
	Disposing
};

/* Enable or disable frame reference counting. You are not supposed to support
* both paths in your application but pick the one most appropriate to your
* needs. */
static const int ffmpeg_refcount = 1;
static const int frame_buffer_size = 60;
static const int pkt_buffer_size = 500;

struct Decoder
{
	DecoderStatus status = DecoderStatus::Inactive;
	AVFormatContext* fmt_ctx = NULL;
	AVCodecContext* video_dec_ctx = NULL;
	AVCodecContext* audio_dec_ctx = NULL;
	int width, height;
	AVPixelFormat pix_fmt;
	const char* src_filename = NULL;
	uint8_t* video_dst_data[4] = { NULL };
	int video_dst_linesize[4];
	int video_dst_bufsize;
	int video_stream_idx = -1;
	int audio_stream_idx = -1;
	int video_frame_count = 0;
	int audio_frame_count = 0;
	bool skip_frames_if_slow = false;
	int64_t last_requested_frame_time = 0;
	int64_t buffer_punctuality = 0;
	int64_t lowest_key_frame_decode_time = 0;
	bool should_seek = false;
	int64_t seek_time = 0;
	SDL_mutex* seek_mutex;
	AVFrame* audio_frame = NULL;
	int display_frame_cursor = frame_buffer_size - 1;
	int frame_buffer_cursor = 0;
	AVFrame* frame_buffer[frame_buffer_size];
	int display_pkt_cursor = 0;
	int pkt_buffer_cursor = 0;
	AVPacket pkt_buffer[pkt_buffer_size];

	static int ThreadProxy(void* instance)
	{
		return ((Decoder*)instance)->open();
	}
	
	void Open(const char* filename)
	{
		src_filename = filename;
		seek_mutex = SDL_CreateMutex();
		SDL_Thread* thread = SDL_CreateThread(ThreadProxy, "Decoder", this);
	}

	int open()
	{
		status = DecoderStatus::Opening;
		int ret = 0, got_frame;
		/* register all formats and codecs */
		av_register_all();
		/* open input file, and allocate format context */
		if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0)
		{
			fprintf(stderr, "Could not open source file %s\n", src_filename);
			return 1;
		}
		/* retrieve stream information */
		if (avformat_find_stream_info(fmt_ctx, NULL) < 0)
		{
			fprintf(stderr, "Could not find stream information\n");
			return 1;
		}
		if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0)
		{
			/* allocate image where the decoded image will be put */
			width = video_dec_ctx->width;
			height = video_dec_ctx->height;
			pix_fmt = video_dec_ctx->pix_fmt;
			ret = av_image_alloc(video_dst_data, video_dst_linesize,
				width, height, pix_fmt, 1);
			if (ret < 0)
			{
				fprintf(stderr, "Could not allocate raw video buffer\n");
				Dispose();
				return 1;
			}
			video_dst_bufsize = ret;
		}
		if (open_codec_context(&audio_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0)
		{
		}
		/* dump input information to stderr */
		av_dump_format(fmt_ctx, 0, src_filename, 0);
		for (int i = 0; i < frame_buffer_size; i++)
		{
			frame_buffer[i] = av_frame_alloc();
			if (!frame_buffer[i])
			{
				fprintf(stderr, "Could not allocate buffer frame\n");
				ret = AVERROR(ENOMEM);
				Dispose();
				return 1;
			}
		}
		audio_frame = av_frame_alloc();
		if (!audio_frame)
		{
			fprintf(stderr, "Could not allocate audio frame\n");
			ret = AVERROR(ENOMEM);
			Dispose();
			return 1;
		}
		
		/* initialize packets, set data to NULL, let the demuxer fill it */
		for (int i = 0; i < pkt_buffer_size; ++i)
		{
			av_init_packet(&pkt_buffer[i]);
			pkt_buffer[i].data = NULL;
			pkt_buffer[i].size = 0;
		}

		/* start decoding frames... */
		return load();
	}

	int load()
	{
		status = DecoderStatus::Loading;

		// prefetch a bunch of packets
		pkt_buffer_cursor = 0;
		fill_pkt_buffer();

		return loop();
	}

	void fill_pkt_buffer()
	{
		int ret = 0;
		int next_pkt_buffer_cursor = (pkt_buffer_cursor + 1) % pkt_buffer_size;
		while (next_pkt_buffer_cursor != display_pkt_cursor && ret >= 0)
		{
			pkt_buffer_cursor = next_pkt_buffer_cursor;
			ret = av_read_frame(fmt_ctx, &pkt_buffer[pkt_buffer_cursor]);
			if (ret >= 0)
				next_pkt_buffer_cursor = (pkt_buffer_cursor + 1) % pkt_buffer_size;
		}
	}
	
	// for now just decode frames as quickly as possible, overwriting until the last read frame
	int loop()
	{
		bool has_skipped_video_frame = false;
		// allocate temp frame
		AVFrame* temp_frame = NULL;
		temp_frame = av_frame_alloc();
		if (!temp_frame)
		{
			fprintf(stderr, "Could not allocate temp frame\n");
			Dispose();
			return AVERROR(ENOMEM);
		}
		// allocate temp packet
		AVPacket pkt;
		av_init_packet(&pkt);
		// read frames from the file
		while (status == DecoderStatus::Loading || status == DecoderStatus::Ready)
		{
			// seeking
			if (should_seek)
			{
				SDL_LockMutex(seek_mutex);
				should_seek = false;
				int64_t temp_seek_time = seek_time;
				SDL_UnlockMutex(seek_mutex);
				// ffmpeg seek
				av_seek_frame(fmt_ctx, video_stream_idx, temp_seek_time, 0);
				// set state to 'loading' as the complete buffer is now invalid
				status = DecoderStatus::Loading;
				// reset the packet and frame buffer so it will completely be refilled
				display_pkt_cursor = 0;
				pkt_buffer_cursor = 0;
				frame_buffer_cursor = (display_frame_cursor + 1) % frame_buffer_size;
				last_requested_frame_time = temp_seek_time;
				printf("SEEKING disp_pkt:%d pkt_cur:%d disp_frm:%d frm_cur:%d\n", display_pkt_cursor, pkt_buffer_cursor, display_frame_cursor, frame_buffer_cursor);
			}
			// fill the packet buffer
			fill_pkt_buffer();

			// decode the next packet
			display_pkt_cursor = (display_pkt_cursor + 1) % pkt_buffer_size;
			av_packet_unref(&pkt);
			pkt = pkt_buffer[display_pkt_cursor];

			// skip the packet if:
			// - there is no data in the packet
			if (pkt.buf == nullptr) 
			{
				printf("skipping frame. pts:%d flags:%d size:%d NO DATA!\n", pkt.pts, pkt.flags, pkt.size);
				continue;
			}
			// - it's from the wrong stream
			if (pkt.stream_index != video_stream_idx) 
			{
				//printf("skipping frame. pts:%d flags:%d size:%d AUDIO PACKET!\n", pkt.pts, pkt.flags, pkt.size);
				continue;
			}
			// - we need to skip to keep up and it's not the yet the next key frame
			if (skip_frames_if_slow && (is_slow_and_should_skip() || has_skipped_video_frame) && ((pkt.flags & AV_PKT_FLAG_KEY) == 0 || pkt.pts < last_requested_frame_time))
			{
				printf("skipping frame. pts:%d flags:%d size:%d SLOW!\n", pkt.pts, pkt.flags, pkt.size);
				//printf("  sfis=%d && (isass=%d || hskf=%d) && (isNotKey=%d || isLate=%d)\n",
				//	skip_frames_if_slow, is_slow_and_should_skip(), has_skipped_video_frame,
				//	(pkt.flags & AV_PKT_FLAG_KEY) == 0, pkt.pts < last_requested_frame_time);
				has_skipped_video_frame = true;
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
			has_skipped_video_frame = false;

			// decode packet
			int ret = 0;
			int got_frame = 0;

			// read all data in the packet 
			while (pkt.size > 0)
			{
				ret = decode_packet(pkt, temp_frame, &got_frame, 0);
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
			if (
				got_frame <= 0 || // the packet did not contain a frame
				temp_frame->pts < 0 // the frame should not be presented
				)
			{
				continue;
			}

			// wait until we have room in the buffer
			int next_frame_buffer_cursor = (frame_buffer_cursor + 1) % frame_buffer_size;
			while (next_frame_buffer_cursor == display_frame_cursor)
			{
				if (status == DecoderStatus::Loading)
					status = DecoderStatus::Ready;
				SDL_Delay(5);

				if (should_seek)
					break;
			}

			// if we're going to seek we can forget about the current frame buffer
			if (should_seek)
				continue;

			// put the new frame in the buffer
			AVFrame* swap_frame = frame_buffer[frame_buffer_cursor];
			frame_buffer[frame_buffer_cursor] = temp_frame;
			temp_frame = swap_frame;
			buffer_punctuality = frame_buffer[frame_buffer_cursor]->pts - last_requested_frame_time;

			// move the buffer cursor
			frame_buffer_cursor = next_frame_buffer_cursor;
		}
		
		// disposing...
		av_frame_free(&temp_frame);
		av_packet_unref(&pkt);
		Dispose();
		
		return 0;
	}

	bool is_slow_and_should_skip()
	{
		int64_t next_key_frame;
		if (next_key_frame_pts(&next_key_frame))
			return buffer_punctuality < max(0, frame_buffer[display_frame_cursor]->pts) - next_key_frame + lowest_key_frame_decode_time;
		else if (buffer_punctuality < 0)
			return true;
		else
			return false;
	}

	bool next_key_frame_pts(int64_t* result)
	{
		for (int i = 1; i < pkt_buffer_size; ++i)
		{
			AVPacket* pkt = &pkt_buffer[(display_pkt_cursor + i) % pkt_buffer_size];
			if (pkt->stream_index == video_stream_idx && pkt->flags & AV_PKT_FLAG_KEY > 0)
			{
				*result = pkt->pts;
				return true;
			}
		}
		// didn't find a suitable keyframe
		return false;
	}

	void Seek(int64_t time)
	{
		// if searching forwards
		if (time > frame_buffer[display_frame_cursor]->pts)
		{
			// first check if the location we're seeking for is already buffered
			int next_display_frame_cursor = (display_frame_cursor + 1) % frame_buffer_size;
			int latest_frame = display_frame_cursor;
			int64_t latest_pts = -1;
			while (next_display_frame_cursor != frame_buffer_cursor &&
				frame_buffer[display_frame_cursor]->pts < time)
			{
				if (frame_buffer[next_display_frame_cursor]->pts > latest_pts)
				{
					latest_pts = frame_buffer[next_display_frame_cursor]->pts;
					latest_frame = next_display_frame_cursor;
				}
				next_display_frame_cursor = (next_display_frame_cursor + 1) % frame_buffer_size;
			}
			// did we find the requested frame in the buffer?
			if (frame_buffer[display_frame_cursor]->pts < time &&
				frame_buffer[next_display_frame_cursor]->pts > time)
			{
				// the frame was in the buffer! we're done here!
				return;
			}
		}
		
		// if the seek time was not in the buffer:
		// schedule a ffmpeg seek which will later be done in loop()
		SDL_LockMutex(seek_mutex);
		should_seek = true;
		seek_time = time;
		SDL_UnlockMutex(seek_mutex);
	}

	AVFrame* NextFrame(int64_t time)
	{
		// remember in the decoder what time was last requested, we can use this for buffering more relevant frames
		last_requested_frame_time = time;
		// only look up to the frame buffer cursor, the decoder thread may be writing to the buffer_cursor index right now
		int one_before_frame_buffer_cursor = (frame_buffer_cursor - 1 + frame_buffer_size) % frame_buffer_size;
		// start looking from the last frame we displayed
		int candidate_frame_cursor = display_frame_cursor;
		// figure out the first candidate's frame time
		int64_t candidate_frame_delta_time = 0;
		if (frame_buffer[candidate_frame_cursor])
			candidate_frame_delta_time = abs(frame_buffer[candidate_frame_cursor]->pts - time);
		int64_t lowest_delta_time = candidate_frame_delta_time;
		// go through all frame in the buffer until we found one where the requested time is smaller than the frame time
		while (candidate_frame_cursor != one_before_frame_buffer_cursor)
		{
			if (lowest_delta_time < 0 || 
				candidate_frame_delta_time < lowest_delta_time)
			{
				lowest_delta_time = candidate_frame_delta_time;
				display_frame_cursor = candidate_frame_cursor;
			}
			candidate_frame_cursor = (candidate_frame_cursor + 1) % frame_buffer_size;
			candidate_frame_delta_time = abs(frame_buffer[candidate_frame_cursor]->pts - time);
		}
		// return the best suited display frame
		return frame_buffer[display_frame_cursor];
	}

	AVFrame* NextFrame()
	{
		// simply return the next display frame in the buffer
		int next_frame_cursor = (display_frame_cursor + 1) % frame_buffer_size;
		if (next_frame_cursor != frame_buffer_cursor)
		{
			display_frame_cursor = next_frame_cursor;
		}
		return frame_buffer[display_frame_cursor];
	}

	void Dispose()
	{
		if (status != DecoderStatus::Disposing && status != DecoderStatus::Inactive)
		{
			status = DecoderStatus::Disposing;
			avcodec_free_context(&video_dec_ctx);
			avcodec_free_context(&audio_dec_ctx);
			avformat_close_input(&fmt_ctx);
			for (int i = 0; i < pkt_buffer_size; ++i)
				av_packet_unref(&pkt_buffer[i]);
			for (int i = 0; i < frame_buffer_size; ++i)
				av_frame_free(&frame_buffer[i]);
			av_free(video_dst_data[0]);
			SDL_DestroyMutex(seek_mutex);
			status = DecoderStatus::Inactive;
		}
	}

	int decode_packet(AVPacket pkt, AVFrame* targetFrame, int *got_frame, int cached)
	{
		int ret = 0;
		int decoded = pkt.size;
		*got_frame = 0;
		if (pkt.stream_index == video_stream_idx)
		{
			// keep track of frame decoding performance
			Uint32 decoding_start_time = SDL_GetTicks();
			// decode video frame
			ret = avcodec_decode_video2(video_dec_ctx, targetFrame, got_frame, &pkt);
			if (ret < 0)
			{
				char buff[256];
				av_strerror(ret, buff, 256);
				fprintf(stderr, "Error decoding video frame (%s)\n", buff);
				return ret;
			}
			if (*got_frame)
			{
				if (targetFrame->width != width || targetFrame->height != height || targetFrame->format != pix_fmt)
				{
					// To handle this change, one could call av_image_alloc again and
					// decode the following frames into another rawvideo file.
					fprintf(stderr, "Error: Width, height and pixel format have to be "
						"constant in a rawvideo file, but the width, height or "
						"pixel format of the input video changed:\n"
						"old: width = %d, height = %d, format = %s\n"
						"new: width = %d, height = %d, format = %s\n",
						width, height, av_get_pix_fmt_name(pix_fmt),
						targetFrame->width, targetFrame->height,
						av_get_pix_fmt_name((AVPixelFormat)targetFrame->format));
					return -1;
				}

				// store the lowest keyframe decoding time, which we can use as estimate for timely skipping to
				// a next keyframe later.
				if (targetFrame->key_frame)
				{
					Uint32 decoding_time = SDL_GetTicks() - decoding_start_time;
					if (lowest_key_frame_decode_time == 0 || decoding_time < lowest_key_frame_decode_time)
						lowest_key_frame_decode_time = decoding_time;
				}

				printf("video_frame%s n:%d coded_n:%d isKey:%d flags:%d dts:%.3f pts:%.3f lrft:%.3f\n",
					cached ? "(cached)" : "",
					video_frame_count++, targetFrame->coded_picture_number, 
					targetFrame->key_frame, pkt.flags, pkt.dts / 1000.0, pkt.pts / 1000.0, last_requested_frame_time / 1000.0);
			}
		}
		else if (pkt.stream_index == audio_stream_idx)
		{
			// decode audio frame
			ret = avcodec_decode_audio4(audio_dec_ctx, targetFrame, got_frame, &pkt);
			if (ret < 0)
			{
				char buff[256];
				av_strerror(ret, buff, 256);
				fprintf(stderr, "Error decoding audio frame (%s)\n", buff);
				return ret;
			}
			// Some audio decoders decode only part of the packet, and have to be
			// called again with the remainder of the packet data.
			// Sample: fate-suite/lossless-audio/luckynight-partial.shn
			// Also, some decoders might over-read the packet.
			decoded = FFMIN(ret, pkt.size);
			if (*got_frame)
			{
				size_t unpadded_linesize = targetFrame->nb_samples * av_get_bytes_per_sample((AVSampleFormat)targetFrame->format);
				char buff[256];
				av_ts_make_time_string(buff, targetFrame->pts, &audio_dec_ctx->time_base);
				//printf("audio_frame%s n:%d nb_samples:%d pts:%s\n",
				//	cached ? "(cached)" : "",
				//	audio_frame_count++, targetFrame->nb_samples, buff);
			}
		}
		return decoded;
	}
	
	int open_codec_context(int *stream_idx,
		AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
	{
		int ret, stream_index;
		AVStream *st;
		AVCodec *dec = NULL;
		AVDictionary *opts = NULL;
		ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
		if (ret < 0)
		{
			fprintf(stderr, "Could not find %s stream in input file '%s'\n",
				av_get_media_type_string(type), src_filename);
			return ret;
		}
		else
		{
			stream_index = ret;
			st = fmt_ctx->streams[stream_index];

			/* find decoder for the stream */
			dec = avcodec_find_decoder(st->codecpar->codec_id);
			if (!dec)
			{
				fprintf(stderr, "Failed to find %s codec\n",
					av_get_media_type_string(type));
				return AVERROR(EINVAL);
			}

			/* Allocate a codec context for the decoder */
			*dec_ctx = avcodec_alloc_context3(dec);
			if (!*dec_ctx)
			{
				fprintf(stderr, "Failed to allocate the %s codec context\n",
					av_get_media_type_string(type));
				return AVERROR(ENOMEM);
			}

			/* Copy codec parameters from input stream to output codec context */
			if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0)
			{
				fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
					av_get_media_type_string(type));
				return ret;
			}

			/* Init the decoders, with or without reference counting */
			av_dict_set(&opts, "refcounted_frames", ffmpeg_refcount ? "1" : "0", 0);
			if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0)
			{
				fprintf(stderr, "Failed to open %s codec\n",
					av_get_media_type_string(type));
				return ret;
			}
			*stream_idx = stream_index;
		}

		return 0;
	}

	int get_format_from_sample_fmt(const char **fmt, enum AVSampleFormat sample_fmt)
	{
		int i;
		struct sample_fmt_entry
		{
			enum AVSampleFormat sample_fmt; const char *fmt_be, *fmt_le;
		} sample_fmt_entries[] =
		{
			{ AV_SAMPLE_FMT_U8,  "u8",    "u8" },
			{ AV_SAMPLE_FMT_S16, "s16be", "s16le" },
			{ AV_SAMPLE_FMT_S32, "s32be", "s32le" },
			{ AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
			{ AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
		};
		*fmt = NULL;
		for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++)
		{
			struct sample_fmt_entry *entry = &sample_fmt_entries[i];
			if (sample_fmt == entry->sample_fmt)
			{
				*fmt = AV_NE(entry->fmt_be, entry->fmt_le);
				return 0;
			}
		}
		fprintf(stderr,
			"sample format %s is not supported as output format\n",
			av_get_sample_fmt_name(sample_fmt));
		return -1;
	}
};
