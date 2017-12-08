#include "SeqClip.h"
#include "Sequentia.h"
#include "SeqLibrary.h"
#include "SeqChannel.h"
#include "SeqSerializer.h"

SeqClip::SeqClip(SeqLibrary *library, SeqChannel* parent, SeqLibraryLink *link):
	library(library),
	parent(parent),
	link(link),
	isPreview(false),
	leftTime(0),
	rightTime(Sequentia::TimeBase), // default 1 second long clips
	startTime(0)
{
	if (link->metaDataLoaded)
		rightTime = link->duration;
}

SeqClip::SeqClip(SeqLibrary *library, SeqChannel* parent, SeqSerializer *serializer) :
	library(library),
	parent(parent),
	isPreview(false)
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
	if (parent != nullptr)
		parent->RemoveClip(this);
	channel->AddClip(this);
	parent = channel;
}

char* SeqClip::GetLabel()
{
	return link->fullPath;
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
