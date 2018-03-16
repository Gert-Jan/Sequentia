#pragma once

#include "SDL_config.h"

class SeqSerializer;
template<class T>
class SeqList;
class SeqLibrary;
class SeqScene;
class SeqClip;
class SeqSelection;

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
	SeqClip* GetClipAt(int64_t time);
	SeqClip* GetClipByActionId(const int id);
	int GetClipIndexByActionId(const int id);

	void AddClipSelection(SeqSelection *selection);
	void AddClipSelectionAt(SeqSelection *selection, int const index);
	void RemoveClipSelection(SeqSelection *selection);
	void RemoveClipSelectionAt(const int index);
	void MoveClipSelection(SeqSelection *selection, int64_t leftTime);
	int ClipSelectionCount();
	SeqSelection* GetClipSelection(const int index);

private:
	void SortClip(int index);
	void SwapClips(const int index0, const int index1);

	void SortClipSelection(int index);
	void SwapClipSelections(const int index0, const int index1);

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
	SeqList<SeqSelection*> *clipSelections;
	int nextActionId;
};
