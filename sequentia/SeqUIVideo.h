#pragma once

#include "SeqWindow.h"
#include "SeqAction.h"

class SeqProject;
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

	void ActionDone(const SeqAction action);
	void ActionUndone(const SeqAction action);
	SeqWindowType GetWindowType();
	void Draw();
	void Serialize(SeqSerializer *serializer);

private:
	void Init();
	void Deserialize(SeqSerializer *serializer);

private:
	char *name;
	SeqMaterialInstance *material;
	SeqTaskDecodeVideo *decoderTask;
	AVFrame *previousFrame;
	bool isSeeking;
	int seekVideoTime;
	int startVideoTime;
	bool lockVideo;
};
