#pragma once

class SeqChannel;
enum class SeqChannelType;

class SeqSerializer;
template<class T>
class SeqList;

class SeqScene
{
public:
	SeqScene(int id, char *name);
	SeqScene(SeqSerializer *serializer);
	~SeqScene();
	 
	void AddChannel(SeqChannelType type, char *name);
	void RemoveChannel(const int index);
	int GetChannelCount();
	SeqChannel* GetChannel(const int index);
	SeqChannel* GetChannelByActionId(const int id);
	int GetChannelIndexByActionId(const int id);

	double GetLength();

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

private:
	SeqList<SeqChannel*> *channels;
	int nextActionId = 0;
};
