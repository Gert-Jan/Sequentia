#include "SeqChannel.h"
#include "SeqClip.h"
#include "SeqSerializer.h"
#include "SeqList.h"

SeqChannel::SeqChannel(SeqLibrary *library, char *name, SeqChannelType type):
	library(library),
	type(type),
	name(name),
	actionId(0),
	nextActionId(0)
{
	clips = new SeqList<SeqClip*>();
}

SeqChannel::SeqChannel(SeqLibrary *library, SeqSerializer *serializer):
	library(library),
	actionId(0),
	nextActionId(0)
{
	clips = new SeqList<SeqClip*>();
	Deserialize(serializer);
}

SeqChannel::~SeqChannel()
{
	for (int i = 0; i < clips->Count(); i++)
		delete clips->Get(i);
}

void SeqChannel::AddClip(SeqClip* clip)
{
	if (clip->GetParent() == nullptr)
	{
		clip->SetParent(this);
	}
	else
	{
		int i = 0;
		while (i < clips->Count() && clips->Get(i)->leftTime <= clip->leftTime)
			i++;
		AddClipAt(clip, i);
	}
}

void SeqChannel::AddClipAt(SeqClip* clip, const int index)
{
	if (clip->GetParent() == nullptr)
	{
		clip->SetParent(this);
	}
	{
		clips->InsertAt(clip, index);
		clip->actionId = NextActionId();
	}
}

void SeqChannel::RemoveClip(SeqClip* clip)
{
	int index = clips->IndexOf(clip);
	if (index > -1)
		RemoveClip(index);
}

void SeqChannel::RemoveClip(const int index)
{
	SeqClip *clip = clips->Get(index);
	clips->RemoveAt(index);
	if (!clip->isPreview)
		delete clip;
}

void SeqChannel::MoveClip(SeqClip* clip, int64_t leftTime)
{
	// move clip
	int64_t width = clip->rightTime - clip->leftTime;
	clip->leftTime = leftTime;
	clip->rightTime = leftTime + width;
	// sort clip
	int index = clips->IndexOf(clip);
	SortClip(index);
}

int SeqChannel::ClipCount()
{
	return clips->Count();
}

SeqClip* SeqChannel::GetClip(const int index)
{
	return clips->Get(index);
}

int SeqChannel::GetClipIndexByActionId(const int id)
{
	for (int i = 0; i < clips->Count(); i++)
		if (clips->Get(i)->actionId == id)
			return i;
	return -1;
}

void SeqChannel::SortClip(int index)
{
	SeqClip* clip = clips->Get(index);
	// sort down
	while (index > 0 && clips->Get(index - 1)->leftTime < clip->leftTime)
	{
		SwapClips(index, index - 1);
		index--;
	}
	// sort up
	while (index < clips->Count() - 1 && clips->Get(index + 1)->leftTime >= clip->leftTime)
	{
		SwapClips(index, index + 1);
		index++;
	}
}

void SeqChannel::SwapClips(const int index0, const int index1)
{
	SeqClip* clip0 = clips->Get(index0);
	SeqClip* clip1 = clips->Get(index1);
	clips->Set(index0, clip1);
	clips->Set(index1, clip0);
}

int SeqChannel::NextActionId()
{
	// TODO: on overflow rearrange action ids.
	return nextActionId++;
}

void SeqChannel::Serialize(SeqSerializer *serializer)
{
	serializer->Write((int)type);
	serializer->Write(name);
	serializer->Write(clips->Count());
	for (int i = 0; i < clips->Count(); i++)
	{
		SeqClip* clip = clips->Get(i);
		if (!clip->isPreview)
			clip->Serialize(serializer);
	}
}

void SeqChannel::Deserialize(SeqSerializer *serializer)
{
	type = (SeqChannelType)serializer->ReadInt();
	name = serializer->ReadString();
	int count = serializer->ReadInt();
	nextActionId = count;
	for (int i = 0; i < count; i++)
	{
		SeqClip *clip = new SeqClip(library, serializer);
		clip->actionId = i;
		clip->SetParent(this);
	}
}
