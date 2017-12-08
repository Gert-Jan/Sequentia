#pragma once

#include "SDL_config.h"

class SeqLibrary;
class SeqChannel;
class SeqLibraryLink;
class SeqSerializer;

class SeqClip
{
public:
	SeqClip(SeqLibrary *library, SeqChannel* parent, SeqLibraryLink *link);
	SeqClip(SeqLibrary *library, SeqChannel* parent, SeqSerializer *serializer);
	~SeqClip();

	void SetPosition(int64_t leftTime);
	void SetParent(SeqChannel* channel);
	char* GetLabel();

public:
	void Serialize(SeqSerializer *serializer);
private:
	void Deserialize(SeqSerializer *serializer);

public:
	bool isPreview;
	int64_t leftTime;
	int64_t rightTime;
	int64_t startTime;

private:
	SeqLibrary *library;
	SeqChannel *parent;
	SeqLibraryLink *link;
};
