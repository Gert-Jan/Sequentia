#include "SeqActionFactory.h"
#include "SeqAction.h"
#include "SeqProjectHeaders.h"
#include "SeqString.h"

SeqAction SeqActionFactory::AddChannel(SeqScene *scene, SeqChannelType type, const char *name)
{
	return SeqAction(
		SeqActionType::AddChannel,
		SeqActionExecution::Do,
		new SeqActionAddChannel(scene->id, type, name));
}

SeqAction SeqActionFactory::RemoveChannel(SeqScene *scene, SeqChannel* channel)
{
	return SeqAction(
		SeqActionType::AddChannel,
		SeqActionExecution::Undo,
		new SeqActionAddChannel(scene->id, channel->type, channel->name));
}

SeqAction SeqActionFactory::AddLibraryLink(const char *fullPath)
{
	return SeqAction(
		SeqActionType::AddLibraryLink,
		SeqActionExecution::Do,
		SeqString::Copy(fullPath));
}

SeqAction SeqActionFactory::RemoveLibraryLink(const char *fullPath)
{
	return SeqAction(
		SeqActionType::AddLibraryLink,
		SeqActionExecution::Undo,
		SeqString::Copy(fullPath));
}

SeqAction SeqActionFactory::AddClipToChannel(SeqClipProxy* clipProxy)
{
	return SeqAction(
		SeqActionType::AddClipToChannel,
		SeqActionExecution::Do,
		new SeqActionAddClipToChannel(clipProxy));
}

SeqAction SeqActionFactory::RemoveClipFromChannel(SeqClip* clip)
{
	return SeqAction(
		SeqActionType::AddClipToChannel,
		SeqActionExecution::Undo,
		new SeqActionAddClipToChannel(clip));
}

SeqAction SeqActionFactory::MoveClip(SeqClipProxy* clipProxy)
{
	return SeqAction(
		SeqActionType::MoveClip,
		SeqActionExecution::Do,
		new SeqActionMoveClip(clipProxy));
}
