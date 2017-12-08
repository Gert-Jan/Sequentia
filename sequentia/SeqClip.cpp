#include "SeqClip.h"
#include "Sequentia.h"
#include "SeqLibrary.h"
#include "SeqChannel.h"
#include "SeqSerializer.h"

SeqClip::SeqClip(SeqLibrary *library, SeqLibraryLink *link):
	library(library),
	parent(nullptr),
	link(link),
	isPreview(false),
	leftTime(0),
	rightTime(Sequentia::TimeBase), // default 1 second long clips
	startTime(0),
	actionId(0)
{
	if (link->metaDataLoaded)
		rightTime = link->duration;
}

SeqClip::SeqClip(SeqLibrary *library, SeqSerializer *serializer) :
	library(library),
	parent(nullptr),
	isPreview(false),
	actionId(0)
{
	Deserialize(serializer);
}

SeqClip::~SeqClip()
{
}

void SeqClip::SetPosition(int64_t leftTime)
{
	parent->MoveClip(this, leftTime);
}

void SeqClip::SetParent(SeqChannel* channel)
{
	if (parent != channel)
	{
		if (parent != nullptr)
			parent->RemoveClip(this);
		parent = channel;
		if (channel != nullptr)
			channel->AddClip(this);
	}
}

SeqChannel* SeqClip::GetParent()
{
	return parent;
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
	serializer->Write(leftTime);
	serializer->Write(rightTime);
	serializer->Write(startTime);
}

void SeqClip::Deserialize(SeqSerializer *serializer)
{
	link = library->GetLink(serializer->ReadString());
	leftTime = serializer->ReadLong();
	rightTime = serializer->ReadLong();
	startTime = serializer->ReadLong();
}
