#include "SeqChannel.h"
#include "SeqClip.h"
#include "SeqClipProxy.h"
#include "SeqSerializer.h"
#include "SeqList.h"
#include "SeqString.h"

SeqChannel::SeqChannel(SeqScene *parent, const char *channelName, SeqChannelType type):
	scene(parent),
	type(type),
	actionId(-1),
	nextActionId(-1)
{
	name = SeqString::Copy(channelName);
	clips = new SeqList<SeqClip*>();
	clipProxies = new SeqList<SeqClipProxy*>();
}

SeqChannel::SeqChannel(SeqScene *parent, SeqSerializer *serializer) :
	scene(parent),
	actionId(-1),
	nextActionId(-1)
{
	clips = new SeqList<SeqClip*>();
	clipProxies = new SeqList<SeqClipProxy*>();
	Deserialize(serializer);
}

SeqChannel::~SeqChannel()
{
	for (int i = 0; i < clips->Count(); i++)
		delete clips->Get(i);
	clipProxies->Clear(); // proxies should be deleted in the SeqProject Proxy pool
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

void SeqChannel::AddClipProxy(SeqClipProxy *proxy)
{
	if (proxy->GetParent() == nullptr)
	{
		proxy->SetParent(this);
	}
	else
	{
		int i = 0;
		while (i < clipProxies->Count() && clipProxies->Get(i)->location.leftTime <= proxy->location.leftTime)
			i++;
		AddClipProxyAt(proxy, i);
	}
}

void SeqChannel::AddClipProxyAt(SeqClipProxy *proxy, int const index)
{
	if (proxy->GetParent() == nullptr)
	{
		proxy->SetParent(this);
	}
	else
	{
		clipProxies->InsertAt(proxy, index);
	}
}

void SeqChannel::RemoveClipProxy(SeqClipProxy *proxy)
{
	int index = clipProxies->IndexOf(proxy);
	if (index > -1)
		RemoveClipProxyAt(index);
}

void SeqChannel::RemoveClipProxyAt(const int index)
{
	SeqClipProxy *proxy = clipProxies->Get(index);
	clipProxies->RemoveAt(index);
}

void SeqChannel::MoveClipProxy(SeqClipProxy *proxy, int64_t leftTime)
{
	// move proxy
	int64_t width = proxy->location.rightTime - proxy->location.leftTime;
	proxy->location.leftTime = leftTime;
	proxy->location.rightTime = leftTime + width;
	// sort proxy
	int index = clipProxies->IndexOf(proxy);
	SortClipProxy(index);
}

int SeqChannel::ClipProxyCount()
{
	return clipProxies->Count();
}

SeqClipProxy* SeqChannel::GetClipProxy(const int index)
{
	return clipProxies->Get(index);
}

void SeqChannel::SortClipProxy(int index)
{
	SeqClipProxy* proxy = clipProxies->Get(index);
	// sort down
	while (index > 0 && clipProxies->Get(index - 1)->location.leftTime < proxy->location.leftTime)
	{
		SwapClipProxies(index, index - 1);
		index--;
	}
	// sort up
	while (index < clipProxies->Count() - 1 && clipProxies->Get(index + 1)->location.leftTime >= proxy->location.leftTime)
	{
		SwapClipProxies(index, index + 1);
		index++;
	}
}

void SeqChannel::SwapClipProxies(const int index0, const int index1)
{
	SeqClipProxy* proxy0 = clipProxies->Get(index0);
	SeqClipProxy* proxy1 = clipProxies->Get(index1);
	clipProxies->Set(index0, proxy1);
	clipProxies->Set(index1, proxy0);
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
	type = (SeqChannelType)serializer->ReadInt();
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
