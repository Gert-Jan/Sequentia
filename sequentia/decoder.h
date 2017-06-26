// ffmpeg powered video decoder
extern "C"
{
	#include "libavformat/avformat.h"
	#include "libavcodec/avcodec.h"
	#include "libavutil/imgutils.h"
	#include "libavutil/samplefmt.h"
	#include "libavutil/timestamp.h"
}

/* Enable or disable frame reference counting. You are not supposed to support
* both paths in your application but pick the one most appropriate to your
* needs. */
static int ffmpeg_refcount = 0;

struct Decoder
{
	AVFormatContext* fmt_ctx = NULL;
	AVCodecContext* video_dec_ctx = NULL;
	AVCodecContext* audio_dec_ctx = NULL;
	int width, height;
	AVPixelFormat pix_fmt;
	AVStream* video_stream = NULL;
	AVStream* audio_stream = NULL;
	const char* src_filename = NULL;
	uint8_t* video_dst_data[4] = { NULL };
	int      video_dst_linesize[4];
	int video_dst_bufsize;
	int video_stream_idx = -1;
	int audio_stream_idx = -1;
	AVFrame* frame = NULL;
	AVPacket pkt;
	int video_frame_count = 0;
	int audio_frame_count = 0;
	
	void Open(const char* filename)
	{
		int ret = 0, got_frame;
		//src_filename = "D:/Camera/Video/20170521_032735A.mp4";
		src_filename = filename;

		/* register all formats and codecs */
		av_register_all();
		/* open input file, and allocate format context */
		if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0)
		{
			fprintf(stderr, "Could not open source file %s\n", src_filename);
			exit(1);
		}
		/* retrieve stream information */
		if (avformat_find_stream_info(fmt_ctx, NULL) < 0)
		{
			fprintf(stderr, "Could not find stream information\n");
			exit(1);
		}
		if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0)
		{
			video_stream = fmt_ctx->streams[video_stream_idx];
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
				return;
			}
			video_dst_bufsize = ret;
		}
		if (open_codec_context(&audio_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0)
		{
			audio_stream = fmt_ctx->streams[audio_stream_idx];
		}
		/* dump input information to stderr */
		av_dump_format(fmt_ctx, 0, src_filename, 0);
		if (!audio_stream && !video_stream)
		{
			fprintf(stderr, "Could not find audio or video stream in the input, aborting\n");
			ret = 1;
			Dispose();
			return;
		}
		frame = av_frame_alloc();
		if (!frame)
		{
			fprintf(stderr, "Could not allocate frame\n");
			ret = AVERROR(ENOMEM);
			Dispose();
			return;
		}
		/* initialize packet, set data to NULL, let the demuxer fill it */
		av_init_packet(&pkt);
		pkt.data = NULL;
		pkt.size = 0;
	}

	AVFrame* NextFrame()
	{
		int ret = 0, got_frame;
		/* read frames from the file */
		while (av_read_frame(fmt_ctx, &pkt) >= 0)
		{
			AVPacket orig_pkt = pkt;
			do
			{
				ret = decode_packet(&got_frame, 0);
				if (ret < 0)
					break;
				pkt.data += ret;
				pkt.size -= ret;
			} while (pkt.size > 0);
			// GJ: if we want only one frame we want to exit here.
			av_packet_unref(&orig_pkt);
			if (ret > 0 && frame->coded_picture_number > 0)
				break;
		}
		/* flush cached frames */
		pkt.data = NULL;
		pkt.size = 0;

		return frame;
	}

	void Dispose()
	{
		avcodec_free_context(&video_dec_ctx);
		avcodec_free_context(&audio_dec_ctx);
		avformat_close_input(&fmt_ctx);
		av_frame_free(&frame);
		av_free(video_dst_data[0]);
	}

	int decode_packet(int *got_frame, int cached)
	{
		int ret = 0;
		int decoded = pkt.size;
		*got_frame = 0;
		if (pkt.stream_index == video_stream_idx)
		{
			/* decode video frame */
			ret = avcodec_decode_video2(video_dec_ctx, frame, got_frame, &pkt);
			if (ret < 0)
			{
				char buff[256];
				av_strerror(ret, buff, 256);
				fprintf(stderr, "Error decoding video frame (%s)\n", buff);
				return ret;
			}
			if (*got_frame)
			{
				if (frame->width != width || frame->height != height || frame->format != pix_fmt)
				{
					/* To handle this change, one could call av_image_alloc again and
					* decode the following frames into another rawvideo file. */
					fprintf(stderr, "Error: Width, height and pixel format have to be "
						"constant in a rawvideo file, but the width, height or "
						"pixel format of the input video changed:\n"
						"old: width = %d, height = %d, format = %s\n"
						"new: width = %d, height = %d, format = %s\n",
						width, height, av_get_pix_fmt_name(pix_fmt),
						frame->width, frame->height,
						av_get_pix_fmt_name((AVPixelFormat)frame->format));
					return -1;
				}

				printf("video_frame%s n:%d coded_n:%d\n",
					cached ? "(cached)" : "",
					video_frame_count++, frame->coded_picture_number);
			}
		}
		else if (pkt.stream_index == audio_stream_idx)
		{
			/* decode audio frame */
			ret = avcodec_decode_audio4(audio_dec_ctx, frame, got_frame, &pkt);
			if (ret < 0)
			{
				char buff[256];
				av_strerror(ret, buff, 256);
				fprintf(stderr, "Error decoding audio frame (%s)\n", buff);
				return ret;
			}

			/* Some audio decoders decode only part of the packet, and have to be
			* called again with the remainder of the packet data.
			* Sample: fate-suite/lossless-audio/luckynight-partial.shn
			* Also, some decoders might over-read the packet. */
			decoded = FFMIN(ret, pkt.size);
			if (*got_frame)
			{
				size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample((AVSampleFormat)frame->format);
				char buff[256];
				av_ts_make_time_string(buff, frame->pts, &audio_dec_ctx->time_base);
				printf("audio_frame%s n:%d nb_samples:%d pts:%s\n",
					cached ? "(cached)" : "",
					audio_frame_count++, frame->nb_samples, buff);
			}
		}

		/* If we use frame reference counting, we own the data and need
		* to de-reference it when we don't use it anymore */
		if (*got_frame && ffmpeg_refcount)
			av_frame_unref(frame);
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
