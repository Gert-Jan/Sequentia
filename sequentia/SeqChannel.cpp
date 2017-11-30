#include "SeqChannel.h";
#include "SeqClip.h";
#include "SeqSerializer.h";
#include "SeqList.h";

SeqChannel::SeqChannel(SeqLibrary *library, char *name, SeqChannelType type):
	library(library),
	type(type),
	name(name)
{
	clips = new SeqList<SeqClip*>();
}

SeqChannel::SeqChannel(SeqLibrary *library, SeqSerializer *serializer):
	library(library)
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
	int i = 0;
	while (i < clips->Count() && clips->Get(i)->leftTime <= clip->leftTime)
		i++;
	clips->InsertAt(clip, i);
}

void SeqChannel::RemoveClip(SeqClip* clip)
{
	int index = clips->IndexOf(clip);
	if (index > -1)
		clips->RemoveAt(index);
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

SeqClip* SeqChannel::GetClip(int index)
{
	return clips->Get(index);
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

void SeqChannel::SwapClips(int index0, int index1)
{
	SeqClip* clip0 = clips->Get(index0);
	clips->Set(index0, clips->Get(index1));
	clips->Set(index1, clip0);
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
	for (int i = 0; i < count; i++)
		clips->Add(new SeqClip(library, this, serializer));
}
