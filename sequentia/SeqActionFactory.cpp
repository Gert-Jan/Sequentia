#include "SeqActionFactory.h"
#include "SeqAction.h"
#include "SeqProjectHeaders.h"
#include "SeqMediaType.h"
#include "SeqString.h"

SeqAction SeqActionFactory::AddChannel(SeqScene *scene, SeqMediaType type, const char *name)
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

SeqAction SeqActionFactory::AddClipGroup(SeqScene *scene)
{
	return SeqAction(
		SeqActionType::AddClipGroup,
		SeqActionExecution::Do,
		new SeqActionAddClipGroup(scene));
}

SeqAction SeqActionFactory::AddClipToChannel(SeqSelection* clipSelection)
{
	return SeqAction(
		SeqActionType::AddClipToChannel,
		SeqActionExecution::Do,
		new SeqActionAddClipToChannel(clipSelection));
}

SeqAction SeqActionFactory::RemoveClipFromChannel(SeqClip* clip)
{
	return SeqAction(
		SeqActionType::AddClipToChannel,
		SeqActionExecution::Undo,
		new SeqActionAddClipToChannel(clip));
}

SeqAction SeqActionFactory::AddClipToGroup(SeqClip* clip, SeqClipGroup * group)
{
	return SeqAction(
		SeqActionType::AddClipToGroup,
		SeqActionExecution::Do,
		new SeqActionAddClipToGroup(clip, group));
}

SeqAction SeqActionFactory::MoveClip(SeqSelection* clipSelection)
{
	return SeqAction(
		SeqActionType::MoveClip,
		SeqActionExecution::Do,
		new SeqActionMoveClip(clipSelection));
}
