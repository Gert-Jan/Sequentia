#pragma once

#include "SDL_config.h"

class SeqSerializer;
template<class T>
class SeqList;
class SeqLibrary;
class SeqScene;
class SeqClip;
class SeqClipProxy;

enum class SeqChannelType
{
	None,
	Video,
	Audio
};

class SeqChannel
{
public:
	SeqChannel(SeqScene *parent, const char *channelName, SeqChannelType type);
	SeqChannel(SeqScene *parent, SeqSerializer *serializer);
	~SeqChannel();

	SeqScene* GetParent();

	void AddClip(SeqClip *clip);
	void AddClipAt(SeqClip *clip, int const index);
	void RemoveClip(SeqClip *clip);
	void RemoveClipAt(const int index);
	void MoveClip(SeqClip *clip, int64_t leftTime);
	int ClipCount();
	SeqClip* GetClip(const int index);
	int GetClipIndexByActionId(const int id);

	void AddClipProxy(SeqClipProxy *proxy);
	void AddClipProxyAt(SeqClipProxy *proxy, int const index);
	void RemoveClipProxy(SeqClipProxy *proxy);
	void RemoveClipProxyAt(const int index);
	void MoveClipProxy(SeqClipProxy *proxy, int64_t leftTime);
	int ClipProxyCount();
	SeqClipProxy* GetClipProxy(const int index);

private:
	void SortClip(int index);
	void SwapClips(const int index0, const int index1);

	void SortClipProxy(int index);
	void SwapClipProxies(const int index0, const int index1);

	int NextActionId();

public:
	void Serialize(SeqSerializer *serializer);
private:
	void Deserialize(SeqSerializer *serializer);

public:
	SeqChannelType type;
	char *name;
	int actionId;
private:
	SeqScene *scene;
	SeqList<SeqClip*> *clips;
	SeqList<SeqClipProxy*> *clipProxies;
	int nextActionId;
};
