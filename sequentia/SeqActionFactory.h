#pragma once

struct SeqAction;
enum class SeqChannelType;
class SeqScene;
class SeqChannel;
class SeqClip;
class SeqSelection;
class SeqClipGroup;

class SeqActionFactory
{
public:
	static SeqAction AddChannel(SeqScene *scene, SeqChannelType type, const char *name);
	static SeqAction RemoveChannel(SeqScene *scene, SeqChannel* channel);
	static SeqAction AddLibraryLink(const char *fullPath);
	static SeqAction RemoveLibraryLink(const char *fullPath);
	static SeqAction AddClipGroup(SeqScene *scene);
	static SeqAction AddClipToChannel(SeqSelection* clipSelection);
	static SeqAction RemoveClipFromChannel(SeqClip* clip);
	static SeqAction AddClipToGroup(SeqClip* clip, SeqClipGroup* group);
	static SeqAction MoveClip(SeqSelection* clipSelection);
};