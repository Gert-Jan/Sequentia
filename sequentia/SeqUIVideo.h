#pragma once

#include "SeqWindow.h"
#include "SeqAction.h"

class SeqProject;
class SeqScene;
class SeqLibrary;
struct SeqMaterialInstance;
class SeqSerializer;
class SeqTaskDecodeVideo;
struct AVFrame;

class SeqUIVideo : public SeqWindow, SeqActionHandler
{
public:
	SeqUIVideo();
	SeqUIVideo(SeqSerializer *serializer);
	~SeqUIVideo();

	void PreExecuteAction(const SeqActionType type, const SeqActionExecution execution, const void *data);
	SeqWindowType GetWindowType();
	void Draw();
	void Serialize(SeqSerializer *serializer);

private:
	void Init();
	void Deserialize(SeqSerializer *serializer);

private:
	char *name;
	SeqMaterialInstance *material;
	SeqScene *scene;
	bool isSeeking;
	int seekVideoTime;
};
