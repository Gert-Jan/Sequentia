#include "SeqClip.h"
#include "Sequentia.h"
#include "SeqLibrary.h"
#include "SeqChannel.h"
#include "SeqSerializer.h"
#include "SeqTime.h"

SeqClip::SeqClip(SeqLibraryLink *link):
	link(link),
	isHidden(false),
	location(SeqClipLocation()),
	actionId(-1)
{
	// TODO: for now use the defaultVideoStreamInfoIndex here, this should be a parameter later on
	streamInfoIndex = link->defaultVideoStreamInfoIndex;
	if (link->metaDataLoaded)
		location.rightTime = link->duration;
	else
		location.rightTime = SEQ_TIME_BASE; // default 1 second long clips
}

SeqClip::SeqClip(SeqSerializer *serializer):
	isHidden(false),
	location(SeqClipLocation()),
	actionId(-1)
{
	Deserialize(serializer);
}

SeqClip::~SeqClip()
{
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

void SeqClip::Serialize(SeqSerializer *serializer)
{
	serializer->Write(link->fullPath);
	location.Serialize(serializer);
}

void SeqClip::Deserialize(SeqSerializer *serializer)
{
	link = Sequentia::GetLibrary()->GetLink(serializer->ReadString());
	// TODO: for now use the defaultVideoStreamInfoIndex here, this should be a serialized value later on
	streamInfoIndex = link->defaultVideoStreamInfoIndex;
	location.Deserialize(serializer);
}
