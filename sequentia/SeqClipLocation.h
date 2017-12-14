#pragma once

#include "SDL_config.h"

class SeqChannel;
class SeqSerializer;

class SeqClipLocation
{
public:
	SeqClipLocation();
	~SeqClipLocation();
	void Reset();
	void SetPosition(int64_t leftTime);

	void Serialize(SeqSerializer *serializer);
	void Deserialize(SeqSerializer *serializer);

public:
	SeqChannel *parent;
	int64_t leftTime;
	int64_t rightTime;
	int64_t startTime;
};
