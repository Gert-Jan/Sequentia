#include "SeqClipLocation.h"
#include "SeqChannel.h"
#include "SeqSerializer.h"

SeqClipLocation::SeqClipLocation() :
	parent(nullptr),
	leftTime(0),
	rightTime(0),
	startTime(0)
{
}

SeqClipLocation::~SeqClipLocation()
{
}

void SeqClipLocation::Reset()
{
	parent = nullptr;
	leftTime = 0;
	rightTime = 0;
	startTime = 0;
}

void SeqClipLocation::SetPosition(int64_t newLeftTime)
{
	int64_t width = rightTime - leftTime;
	leftTime = newLeftTime;
	rightTime = leftTime + width;
}

void SeqClipLocation::Serialize(SeqSerializer *serializer)
{
	serializer->Write(leftTime);
	serializer->Write(rightTime);
	serializer->Write(startTime);
}

void SeqClipLocation::Deserialize(SeqSerializer *serializer)
{
	leftTime = serializer->ReadLong();
	rightTime = serializer->ReadLong();
	startTime = serializer->ReadLong();
}
