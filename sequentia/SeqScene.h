#pragma once

class SeqChannel;
enum class SeqMediaType;
class SeqClip;
class SeqClipGroup;

class SeqPlayer;
class SeqSerializer;
template<class T>
class SeqList;

class SeqScene
{
public:
	SeqScene(int id, const char *sceneName);
	SeqScene(SeqSerializer *serializer);
	~SeqScene();
	 
	void AddChannel(SeqMediaType type, const char *name);
	void RemoveChannel(const int index);
	int ChannelCount();
	SeqChannel* GetChannel(const int index);
	SeqChannel* GetChannelByActionId(const int id);
	int GetChannelIndexByActionId(const int id);

	void AddClipGroup();
	void RemoveClipGroup(const int index);
	void RemoveClipGroup(SeqClipGroup *group);
	int ClipGroupCount();
	SeqClipGroup* GetClipGroup(const int index);
	SeqClipGroup* GetClipGroupByActionId(const int id);
	int GetClipGroupIndexByActionId(const int id);

	void RefreshLastClip();
	int64_t GetLength();

private:
	int NextActionId();
	void AddChannel(SeqChannel *channel);
	void AddClipGroup(SeqClipGroup *clipGroup);

public:
	void Serialize(SeqSerializer *serializer);
private:
	void Deserialize(SeqSerializer *serializer);

public:
	int id;
	char *name;
	SeqPlayer *player;

private:
	SeqList<SeqChannel*> *channels;
	SeqList<SeqClipGroup*> *clipGroups;
	SeqClip *lastClip;
	int nextActionId = 0;
};
