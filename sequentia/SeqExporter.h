#pragma once

#include "SDL_config.h"
#include "SeqDownloadTextureTarget.h"

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
	// status
	int64_t currentFrame;
	bool isInitialized;
	SeqTaskEncodeVideo *encodeTask;
	SeqMaterialInstance *material;
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
	void DisposeMaterials();

private:
	SeqList<SeqExportTask> *taskList;
	SeqList<SeqExportTask> *doneTaskList;
	float convertMatrices[3][4][4];
	SeqMaterialInstance* convertMaterials[3];
	SeqMaterialInstance* exportToMaterials[3];
	SeqDownloadTextureTarget exportToBuffer[3];
	bool renderFrame = false;
	bool exportFrame = false;
};
