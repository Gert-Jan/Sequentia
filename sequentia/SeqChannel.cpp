#include "SeqChannel.h"
#include "SeqClip.h"
#include "SeqMediaType.h"
#include "SeqSelection.h"
#include "SeqSerializer.h"
#include "SeqList.h"
#include "SeqString.h"

SeqChannel::SeqChannel(SeqScene *parent, const char *channelName, SeqMediaType type):
	scene(parent),
	type(type),
	actionId(-1),
	nextActionId(-1)
{
	name = SeqString::Copy(channelName);
	clips = new SeqList<SeqClip*>();
	clipSelections = new SeqList<SeqSelection*>();
}

SeqChannel::SeqChannel(SeqScene *parent, SeqSerializer *serializer) :
	scene(parent),
	actionId(-1),
	nextActionId(-1)
{
	clips = new SeqList<SeqClip*>();
	clipSelections = new SeqList<SeqSelection*>();
	Deserialize(serializer);
}

SeqChannel::~SeqChannel()
{
	for (int i = 0; i < clips->Count(); i++)
		delete clips->Get(i);
	clipSelections->Clear(); // selections should be deleted in the SeqProject Selections pool
	delete[] name;
}

SeqScene* SeqChannel::GetParent()
{
	return scene;
}

void SeqChannel::AddClip(SeqClip *clip)
{
	if (clip->GetParent() == nullptr)
	{
		clip->SetParent(this);
	}
	else
	{
		int i = 0;
		while (i < clips->Count() && clips->Get(i)->location.leftTime <= clip->location.leftTime)
			i++;
		AddClipAt(clip, i);
	}
}

void SeqChannel::AddClipAt(SeqClip *clip, const int index)
{
	if (clip->GetParent() == nullptr)
	{
		clip->SetParent(this);
	}
	else
	{
		clips->InsertAt(clip, index);
		if (clip->actionId == -1)
			clip->actionId = NextActionId();
		else if (clip->actionId >= nextActionId)
			nextActionId = clip->actionId + 1;

	}
}

void SeqChannel::RemoveClip(SeqClip *clip)
{
	int index = clips->IndexOf(clip);
	if (index > -1)
		RemoveClipAt(index);
}

void SeqChannel::RemoveClipAt(const int index)
{
	SeqClip *clip = clips->Get(index);
	clips->RemoveAt(index);
	clip->SetParent(nullptr);
}

void SeqChannel::MoveClip(SeqClip *clip, int64_t leftTime)
{
	// move clip
	int64_t width = clip->location.rightTime - clip->location.leftTime;
	clip->location.leftTime = leftTime;
	clip->location.rightTime = leftTime + width;
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

SeqClip* SeqChannel::GetClipAt(int64_t time)
{
	for (int i = 0; i < clips->Count(); i++)
	{
		SeqClip *clip = clips->Get(i);
		if (clip->location.ContainsTime(time))
			return clip;
	}
	return nullptr;
}

SeqClip* SeqChannel::GetClipByActionId(const int id)
{
	const int index = GetClipIndexByActionId(id);
	if (index > -1)
		return GetClip(index);
	else
		return nullptr;
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
	while (index > 0 && clips->Get(index - 1)->location.leftTime < clip->location.leftTime)
	{
		SwapClips(index, index - 1);
		index--;
	}
	// sort up
	while (index < clips->Count() - 1 && clips->Get(index + 1)->location.leftTime >= clip->location.leftTime)
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

void SeqChannel::AddClipSelection(SeqSelection *selection)
{
	if (selection->GetParent() == nullptr)
	{
		selection->SetParent(this);
	}
	else
	{
		int i = 0;
		while (i < clipSelections->Count() && clipSelections->Get(i)->location.leftTime <= selection->location.leftTime)
			i++;
		AddClipSelectionAt(selection, i);
	}
}

void SeqChannel::AddClipSelectionAt(SeqSelection *selection, int const index)
{
	if (selection->GetParent() == nullptr)
	{
		selection->SetParent(this);
	}
	else
	{
		clipSelections->InsertAt(selection, index);
	}
}

void SeqChannel::RemoveClipSelection(SeqSelection *selection)
{
	int index = clipSelections->IndexOf(selection);
	if (index > -1)
		RemoveClipSelectionAt(index);
}

void SeqChannel::RemoveClipSelectionAt(const int index)
{
	SeqSelection *selection = clipSelections->Get(index);
	clipSelections->RemoveAt(index);
}

void SeqChannel::MoveClipSelection(SeqSelection *selection, int64_t leftTime)
{
	// move selection
	int64_t width = selection->location.rightTime - selection->location.leftTime;
	selection->location.leftTime = leftTime;
	selection->location.rightTime = leftTime + width;
	// sort selection
	int index = clipSelections->IndexOf(selection);
	SortClipSelection(index);
}

int SeqChannel::ClipSelectionCount()
{
	return clipSelections->Count();
}

SeqSelection* SeqChannel::GetClipSelection(const int index)
{
	return clipSelections->Get(index);
}

void SeqChannel::SortClipSelection(int index)
{
	SeqSelection* selection = clipSelections->Get(index);
	// sort down
	while (index > 0 && clipSelections->Get(index - 1)->location.leftTime < selection->location.leftTime)
	{
		SwapClipSelections(index, index - 1);
		index--;
	}
	// sort up
	while (index < clipSelections->Count() - 1 && clipSelections->Get(index + 1)->location.leftTime >= selection->location.leftTime)
	{
		SwapClipSelections(index, index + 1);
		index++;
	}
}

void SeqChannel::SwapClipSelections(const int index0, const int index1)
{
	SeqSelection* selection0 = clipSelections->Get(index0);
	SeqSelection* selection1 = clipSelections->Get(index1);
	clipSelections->Set(index0, selection1);
	clipSelections->Set(index1, selection0);
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
		clip->Serialize(serializer);
	}
}

void SeqChannel::Deserialize(SeqSerializer *serializer)
{
	type = (SeqMediaType)serializer->ReadInt();
	name = serializer->ReadString();
	int count = serializer->ReadInt();
	nextActionId = count;
	for (int i = 0; i < count; i++)
	{
		SeqClip *clip = new SeqClip(serializer);
		clip->actionId = i;
		clip->SetParent(this);
	}
}
