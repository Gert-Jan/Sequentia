#pragma once

#include "SeqAction.h";
#include "SeqChannel.h";
#include "SeqString.h";

class SeqActionFactory
{
public:
	static inline SeqAction CreateAddChannelAction(SeqChannelType type, char* name)
	{
		return SeqAction(SeqActionType::AddChannel,
			new SeqActionAddChannel(type, name));
	}

	static inline SeqAction CreateRemoveChannelAction(SeqChannel* channel)
	{
		return SeqAction(SeqActionType::RemoveChannel,
			new SeqActionAddChannel(channel->type, channel->name));
	}

	static inline SeqAction CreateAddLibraryLinkAction(char *fullPath)
	{
		return SeqAction(SeqActionType::AddLibraryLink, SeqString::Copy(fullPath));
	}

	static inline SeqAction CreateRemoveLibraryLinkAction(char *fullPath)
	{
		return SeqAction(SeqActionType::RemoveLibraryLink, SeqString::Copy(fullPath));
	}
};