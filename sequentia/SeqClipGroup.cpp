#include "SeqProjectHeaders.h";
#include "SeqList.h";
#include "SeqSerializer.h";
#include "Sequentia.h";

SeqClipGroup::SeqClipGroup(SeqScene *parent):
	actionId(-1),
	scene(parent)
{
	clips = new SeqList<SeqClip*>(2);
}

SeqClipGroup::SeqClipGroup(SeqScene *parent, SeqSerializer *serializer) :
	actionId(-1),
	scene(parent)
{
	Deserialize(serializer);
}

SeqClipGroup::~SeqClipGroup()
{
	RemoveAllClips();
	delete clips;
}

SeqScene* SeqClipGroup::GetParent()
{
	return scene;
}

void SeqClipGroup::AddClip(SeqClip *clip)
{
	clip->group = this;
	clips->Add(clip);
}

int SeqClipGroup::ClipCount()
{
	return clips->Count();
}

SeqClip* SeqClipGroup::GetClip(int index)
{
	return clips->Get(index);
}

void SeqClipGroup::RemoveClip(SeqClip *clip)
{
	clip->group = nullptr;
	clips->Remove(clip);
}

void SeqClipGroup::RemoveAllClips()
{
	for (int i = 0; i < clips->Count(); i++)
		clips->Get(i)->group = nullptr;
	clips->Clear();
}

void SeqClipGroup::Serialize(SeqSerializer *serializer)
{
	SeqProject *project = Sequentia::GetProject();
	serializer->Write(clips->Count());
	for (int i = 0; i < clips->Count(); i++)
	{
		SeqClip *clip = clips->Get(i);
		SeqChannel *channel = clip->GetParent();
		SeqScene *scene = channel->GetParent();
		serializer->Write(scene->id);
		serializer->Write(scene->GetChannelIndexByActionId(channel->actionId));
		serializer->Write(channel->GetClipIndexByActionId(clip->actionId));
	}
}

void SeqClipGroup::Deserialize(SeqSerializer *serializer)
{
	SeqProject *project = Sequentia::GetProject();
	int clipCount = serializer->ReadInt();
	clips = new SeqList<SeqClip*>(clipCount);
	for (int i = 0; i < clipCount; i++)
	{
		int sceneId = serializer->ReadInt();
		int channelIndex = serializer->ReadInt();
		int clipIndex = serializer->ReadInt();
		AddClip(project->GetSceneById(sceneId)->GetChannel(channelIndex)->GetClip(clipIndex));
	}
}
