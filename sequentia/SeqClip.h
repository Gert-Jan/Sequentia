#pragma once

#include "SDL_config.h"
#include "SeqClipLocation.h"

class SeqChannel;
struct SeqLibraryLink;
struct SeqStreamInfo;
class SeqSerializer;

class SeqClip
{
public:
	SeqClip(SeqLibraryLink *link, int streamIndex);
	SeqClip(SeqSerializer *serializer);
	~SeqClip();

	void SetPosition(int64_t leftTime);
	void SetParent(SeqChannel* channel);
	SeqChannel* GetParent();
	char* GetLabel();
	SeqLibraryLink* GetLink();
	SeqStreamInfo* GetStreamInfo();

public:
	void Serialize(SeqSerializer *serializer);
private:
	void Deserialize(SeqSerializer *serializer);

public:
	int actionId;
	bool isHidden;
	SeqClipLocation location;
	int streamIndex;
private:
	SeqLibraryLink *link;
};


