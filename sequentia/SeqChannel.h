#pragma once

#include "SDL_config.h"

class SeqSerializer;
template<class T>
class SeqList;
class SeqLibrary;
class SeqClip;

enum class SeqChannelType
{
	None,
	Video,
	Audio
};

class SeqChannel
{
public:
	SeqChannel(SeqLibrary* library, char *name, SeqChannelType type);
	SeqChannel(SeqLibrary* library, SeqSerializer *serializer);
	~SeqChannel();

	void AddClip(SeqClip* clip);
	void AddClipAt(SeqClip* clip, int const index);
	void RemoveClip(SeqClip* clip);
	void RemoveClip(const int index);
	void MoveClip(SeqClip* clip, int64_t leftTime);
	int ClipCount();
	SeqClip* GetClip(const int index);
	int GetClipIndexByActionId(const int id);

private:
	void SortClip(int index);
	void SwapClips(const int index0, const int index1);
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
	SeqLibrary *library;
	SeqList<SeqClip*> *clips;
	int nextActionId;
};
