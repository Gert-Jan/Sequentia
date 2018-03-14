#pragma once

template<class T>
class SeqList;
class SeqSerializer;
class SeqClip;

class SeqClipGroup
{
public:
	SeqClipGroup(SeqScene *parent);
	SeqClipGroup(SeqScene *parent, SeqSerializer *serializer);
	~SeqClipGroup();

	SeqScene* GetParent();

	void AddClip(SeqClip *clip);
	int ClipCount();
	SeqClip* GetClip(int index);
	void RemoveClip(SeqClip *sclip);
	void RemoveAllClips();

public:
	void Serialize(SeqSerializer *serializer);
private:
	void Deserialize(SeqSerializer *serializer);

public:
	int actionId;
private:
	SeqScene *scene;
	SeqList<SeqClip*> *clips;
};