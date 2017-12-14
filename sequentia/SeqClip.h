#pragma once

#include "SDL_config.h"
#include "SeqClipLocation.h"

class SeqLibrary;
class SeqChannel;
struct SeqLibraryLink;
class SeqSerializer;

class SeqClip
{
public:
	SeqClip(SeqLibrary *library, SeqLibraryLink *link);
	SeqClip(SeqLibrary *library, SeqSerializer *serializer);
	~SeqClip();

	void SetPosition(int64_t leftTime);
	void SetParent(SeqChannel* channel);
	SeqChannel* GetParent();
	char* GetLabel();
	SeqLibraryLink* GetLink();

public:
	void Serialize(SeqSerializer *serializer);
private:
	void Deserialize(SeqSerializer *serializer);

public:
	bool isHidden;
	SeqClipLocation location;
	int actionId;
private:
	SeqLibrary *library;
	SeqLibraryLink *link;
};


