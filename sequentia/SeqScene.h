#pragma once

class SeqChannel;
enum class SeqChannelType;
class SeqClip;

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
	 
	void AddChannel(SeqChannelType type, const char *name);
	void RemoveChannel(const int index);
	int ChannelCount();
	SeqChannel* GetChannel(const int index);
	SeqChannel* GetChannelByActionId(const int id);
	int GetChannelIndexByActionId(const int id);
	void RefreshLastClip();

	int64_t GetLength();

private:
	int NextActionId();
	void AddChannel(SeqChannel *channel);

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
	SeqClip *lastClip;
	int nextActionId = 0;
};
