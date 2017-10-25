#pragma once

#include "SeqAction.h";

class SeqProject;
class SeqSerializer;

class SeqUIVideo : SeqActionHandler
{
public:
	SeqUIVideo(SeqProject *project);
	SeqUIVideo(SeqProject *project, SeqSerializer *serializer);
	~SeqUIVideo();

	void ActionDone(const SeqAction action);
	void ActionUndone(const SeqAction action);
	void Draw();
	void Serialize(SeqSerializer *serializer);

private:
	void Init();
	void Deserialize(SeqSerializer *serializer);

private:
	char *name;
	SeqProject *project;
};
