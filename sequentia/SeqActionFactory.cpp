#include "SeqActionFactory.h"
#include "SeqAction.h"
#include "SeqProjectHeaders.h"
#include "SeqString.h"

SeqAction SeqActionFactory::CreateAddChannelAction(SeqChannelType type, char* name)
{
	return SeqAction(
		SeqActionType::AddChannel,
		SeqActionExecution::Do,
		new SeqActionAddChannel(type, name));
}

SeqAction SeqActionFactory::CreateRemoveChannelAction(SeqChannel* channel)
{
	return SeqAction(
		SeqActionType::AddChannel,
		SeqActionExecution::Undo,
		new SeqActionAddChannel(channel->type, channel->name));
}

SeqAction SeqActionFactory::CreateAddLibraryLinkAction(char *fullPath)
{
	return SeqAction(
		SeqActionType::AddLibraryLink,
		SeqActionExecution::Do,
		SeqString::Copy(fullPath));
}

SeqAction SeqActionFactory::CreateRemoveLibraryLinkAction(char *fullPath)
{
	return SeqAction(
		SeqActionType::AddLibraryLink,
		SeqActionExecution::Undo,
		SeqString::Copy(fullPath));
}

SeqAction SeqActionFactory::CreateAddClipToChannelAction(SeqClip* previewClip)
{
	return SeqAction(
		SeqActionType::AddClipToChannel,
		SeqActionExecution::Do,
		new SeqActionAddClipToChannel(previewClip));
}

SeqAction SeqActionFactory::CreateRemoveClipFromChannelAction(SeqClip* clip)
{
	return SeqAction(
		SeqActionType::AddClipToChannel,
		SeqActionExecution::Undo,
		new SeqActionAddClipToChannel(clip));
}
