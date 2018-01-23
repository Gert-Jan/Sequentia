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

	bool ContainsTime(int64_t time);
	bool OverlapsTimeRange(int64_t leftBound, int64_t rightBound);

	void Serialize(SeqSerializer *serializer);
	void Deserialize(SeqSerializer *serializer);

public:
	SeqChannel *parent;
	int64_t leftTime;
	int64_t rightTime;
	int64_t startTime;
};
