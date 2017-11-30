#pragma once

#include "SDL_config.h";

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
	void RemoveClip(SeqClip* clip);
	void MoveClip(SeqClip* clip, int64_t leftTime);
	int ClipCount();
	SeqClip* GetClip(int index);

private:
	void SortClip(int index);
	void SwapClips(int index0, int index1);

public:
	void Serialize(SeqSerializer *serializer);
private:
	void Deserialize(SeqSerializer *serializer);

public:
	SeqChannelType type;
	char *name;
private:
	SeqLibrary *library;
	SeqList<SeqClip*> *clips;
};
