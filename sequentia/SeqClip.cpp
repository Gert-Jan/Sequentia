#include "SeqProjectHeaders.h"
#include "Sequentia.h"
#include "SeqStreamInfo.h"
#include "SeqSerializer.h"
#include "SeqTime.h"

SeqClip::SeqClip(SeqLibraryLink *link, int streamIndex):
	link(link),
	streamIndex(streamIndex),
	isHidden(false),
	location(SeqClipLocation()),
	group(nullptr),
	actionId(-1)
{
	if (link->metaDataLoaded)
		location.rightTime = link->duration;
	else
		location.rightTime = SEQ_TIME_BASE; // default 1 second long clips
}

SeqClip::SeqClip(SeqSerializer *serializer):
	isHidden(false),
	location(SeqClipLocation()),
	group(nullptr),
	actionId(-1)
{
	Deserialize(serializer);
}

SeqClip::~SeqClip()
{
	if (group != nullptr)
	{
		if (group->ClipCount() <= 2)
		{
			group->GetParent()->RemoveClipGroup(group);
		}
	}
}

void SeqClip::SetPosition(int64_t leftTime)
{
	location.parent->MoveClip(this, leftTime);
}

void SeqClip::SetParent(SeqChannel* channel)
{
	if (location.parent != channel)
	{
		// remove the clip from current parent
		if (location.parent != nullptr)
			location.parent->RemoveClip(this);
		// set new parent
		location.parent = channel;
		// add clip to new parent
		if (channel != nullptr)
			channel->AddClip(this);
	}
}

SeqChannel* SeqClip::GetParent()
{
	return location.parent;
}

char* SeqClip::GetLabel()
{
	return link->fullPath;
}

SeqLibraryLink* SeqClip::GetLink()
{
	return link;
}

SeqStreamInfo* SeqClip::GetStreamInfo()
{
	return &link->streamInfos[streamIndex];
}

void SeqClip::Serialize(SeqSerializer *serializer)
{
	serializer->Write(link->fullPath);
	serializer->Write(streamIndex);
	location.Serialize(serializer);
}

void SeqClip::Deserialize(SeqSerializer *serializer)
{
	link = Sequentia::GetLibrary()->GetLink(serializer->ReadString());
	streamIndex = serializer->ReadInt();
	location.Deserialize(serializer);
}
