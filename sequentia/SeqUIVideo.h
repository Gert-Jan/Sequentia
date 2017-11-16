#pragma once

#include "SeqWindow.h";
#include "SeqAction.h";

class SeqProject;
class SeqLibrary;
class SeqMaterial;
class SeqSerializer;
class SeqTaskDecodeVideo;
struct AVFrame;

class SeqUIVideo : public SeqWindow, SeqActionHandler
{
public:
	SeqUIVideo(SeqProject *project, SeqLibrary *library);
	SeqUIVideo(SeqProject *project, SeqLibrary *library, SeqSerializer *serializer);
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
	SeqProject *project;
	SeqLibrary *library;
	SeqMaterial *material;
	SeqTaskDecodeVideo *decoderTask;
	AVFrame *previousFrame;
};
