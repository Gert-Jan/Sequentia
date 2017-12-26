#include "SeqProjectHeaders.h"
#include "SeqPlayer.h"
#include "SeqList.h"
#include "SeqSerializer.h"
#include "SeqString.h"
#include "SeqTime.h"

SeqScene::SeqScene(int id, const char *sceneName) :
	id(id),
	lastClip(nullptr),
	nextActionId(0)
{
	name = SeqString::Copy(sceneName);
	channels = new SeqList<SeqChannel*>();
	player = new SeqPlayer(this);
}

SeqScene::SeqScene(SeqSerializer *serializer):
	id(0),
	name(nullptr),
	lastClip(nullptr),
	nextActionId(0)
{
	channels = new SeqList<SeqChannel*>();
	Deserialize(serializer);
	player = new SeqPlayer(this);
}

SeqScene::~SeqScene()
{
	for (int i = 0; i < channels->Count(); i++)
		delete channels->Get(i);
	delete channels;
	delete[] name;
}

void SeqScene::AddChannel(SeqChannelType type, const char *name)
{
	SeqChannel *channel = new SeqChannel(this, name, type);
	AddChannel(channel);
}

void SeqScene::AddChannel(SeqChannel *channel)
{
	channels->Add(channel);
	if (channel->actionId == -1)
		channel->actionId = NextActionId();
	else if (channel->actionId >= nextActionId)
		nextActionId = channel->actionId + 1;
}

void SeqScene::RemoveChannel(const int index)
{
	channels->RemoveAt(index);
}

int SeqScene::ChannelCount()
{
	return channels->Count();
}

SeqChannel* SeqScene::GetChannel(const int index)
{
	return channels->Get(index);
}

SeqChannel* SeqScene::GetChannelByActionId(const int id)
{
	return GetChannel(GetChannelIndexByActionId(id));
}

int SeqScene::GetChannelIndexByActionId(const int id)
{
	for (int i = 0; i < channels->Count(); i++)
		if (channels->Get(i)->actionId == id)
			return i;
	return -1;
}

void SeqScene::RefreshLastClip()
{
	lastClip = nullptr;
	int64_t latestTime = 0;
	for (int i = 0; i < channels->Count(); i++)
	{
		SeqChannel *channel = channels->Get(i);
		for (int j = 0; j < channel->ClipCount(); j++)
		{
			SeqClip *clip = channel->GetClip(j);
			int64_t rightTime = clip->location.rightTime;
			if (rightTime > latestTime)
			{
				latestTime = rightTime;
				lastClip = clip;
			}
		}
	}
}

int64_t SeqScene::GetLength()
{
	if (lastClip != nullptr)
		return lastClip->location.rightTime;
	else
		return SEQ_TIME_BASE;
}

int SeqScene::NextActionId()
{
	return nextActionId++;
}

void SeqScene::Serialize(SeqSerializer *serializer)
{
	serializer->Write(id);
	serializer->Write(name);
	serializer->Write(channels->Count());
	for (int i = 0; i < channels->Count(); i++)
		channels->Get(i)->Serialize(serializer);
}

void SeqScene::Deserialize(SeqSerializer *serializer)
{
	id = serializer->ReadInt();
	name = serializer->ReadString();
	int count = serializer->ReadInt();
	for (int i = 0; i < count; i++)
	{
		SeqChannel *channel = new SeqChannel(this, serializer);
		AddChannel(channel);
	}
	RefreshLastClip();
}
