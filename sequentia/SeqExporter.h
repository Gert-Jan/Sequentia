#pragma once

#include "SDL_config.h"
#include "SeqDownloadTextureTarget.h"

extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/samplefmt.h"
#include "libavutil/timestamp.h"
#include <libavutil/opt.h>
}

class SeqScene;
class SeqTaskEncodeVideo;
class SeqMaterialInstance;
template<class T>
class SeqList;

struct SeqExportTask
{
	// settings
	char *fullPath;
	SeqScene *scene;
	int64_t startTime;
	int64_t endTime;
	double frameTime;
	int width;
	int height;
	AVCodecID codecId;
	int bitRate;
	// status
	int64_t currentFrame;
	bool isInitialized;
	SeqTaskEncodeVideo *encodeTask;
	SeqMaterialInstance *material;
	// TODO: move below to SeqEncoder
	AVCodecContext *context;
	AVFrame *frame;
	AVPacket *packet;
	SDL_RWops *file;
	int frameCount;
};

class SeqExporter
{
public:
	SeqExporter();
	~SeqExporter();
	void RefreshMaterials();

	bool IsExporting();
	float GetProgress();
	float GetExportWidth();
	float GetExportHeight();

	void AddTask(char *fullPath, SeqScene *scene);

	void Update();
	void Export();

private:
	int CreateContext(SeqExportTask *task);
	int Encode(SeqExportTask *task);
	int Encode(AVCodecContext *context, AVFrame *frame, AVPacket *packet, SDL_RWops *file);
	void FinalizeEncoding(SeqExportTask *task);
	void DisposeMaterials();

private:
	SeqList<SeqExportTask> *taskList;
	SeqList<SeqExportTask> *doneTaskList;
	float convertMatrices[3][4][4];
	SeqMaterialInstance *convertMaterials[3];
	SeqMaterialInstance *exportToMaterials[3];
	SeqDownloadTextureTarget exportToBuffer[3];
	bool renderFrame = false;
	bool exportFrame = false;
};
