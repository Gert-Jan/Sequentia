#pragma once

struct SeqAction;
enum class SeqChannelType;
class SeqChannel;
class SeqClip;

class SeqActionFactory
{
public:
	static SeqAction CreateAddChannelAction(SeqChannelType type, char* name);
	static SeqAction CreateRemoveChannelAction(SeqChannel* channel);
	static SeqAction CreateAddLibraryLinkAction(char *fullPath);
	static SeqAction CreateRemoveLibraryLinkAction(char *fullPath);
	static SeqAction CreateAddClipToChannelAction(SeqClip* previewClip);
	static SeqAction CreateRemoveClipFromChannelAction(SeqClip* clip);
};