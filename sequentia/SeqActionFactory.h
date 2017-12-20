#pragma once

struct SeqAction;
enum class SeqChannelType;
class SeqScene;
class SeqChannel;
class SeqClip;
class SeqClipProxy;

class SeqActionFactory
{
public:
	static SeqAction AddChannel(SeqScene *scene, SeqChannelType type, char* name);
	static SeqAction RemoveChannel(SeqScene *scene, SeqChannel* channel);
	static SeqAction AddLibraryLink(char *fullPath);
	static SeqAction RemoveLibraryLink(char *fullPath);
	static SeqAction AddClipToChannel(SeqClipProxy* clipProxy);
	static SeqAction RemoveClipFromChannel(SeqClip* clip);
	static SeqAction MoveClip(SeqClipProxy* clipProxy);
};