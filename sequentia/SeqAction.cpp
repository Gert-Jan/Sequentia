#include "SeqAction.h"
#include "Sequentia.h"
#include "SeqProjectHeaders.h"
#include "SeqString.h"

SeqActionAddChannel::SeqActionAddChannel(int sceneId, SeqChannelType type, const char *channelName):
	sceneId(sceneId),
	type(type)
{
	name = SeqString::Copy(channelName);
}

// on add
SeqActionAddClipToChannel::SeqActionAddClipToChannel(SeqSelection *selection)
{
	SeqLibrary *library = Sequentia::GetLibrary();
	SeqChannel *channel = selection->GetParent();
	sceneId = channel->GetParent()->id;
	channelId = channel->actionId;
	libraryLinkIndex = library->GetLinkIndex(selection->GetClip()->GetLink()->fullPath);
	streamIndex = selection->GetClip()->streamIndex;
	clipId = -1; // filled in after placing the new clip
	leftTime = selection->location.leftTime;
	rightTime = selection->location.rightTime;
	startTime = selection->location.startTime;
}

// on remove
SeqActionAddClipToChannel::SeqActionAddClipToChannel(SeqClip *clip)
{
	SeqLibrary *library = Sequentia::GetLibrary();
	SeqChannel *channel = clip->GetParent();
	sceneId = channel->GetParent()->id;
	channelId = channel->actionId;
	libraryLinkIndex = library->GetLinkIndex(clip->GetLink()->fullPath);
	clipId = clip->actionId;
	leftTime = clip->location.leftTime;
	rightTime = clip->location.rightTime;
	startTime = clip->location.startTime;
}

SeqActionAddClipGroup::SeqActionAddClipGroup(SeqScene *scene)
{
	sceneId = scene->id;
	groupId = -1; // filled after adding the clip group
}

SeqActionAddClipToGroup::SeqActionAddClipToGroup(SeqClip *clip, SeqClipGroup *group)
{
	SeqChannel *channel = clip->GetParent();
	sceneId = channel->GetParent()->id;
	channelId = channel->actionId;
	clipId = clip->actionId;
	groupId = group->actionId;
}

SeqActionMoveClip::SeqActionMoveClip(SeqSelection *selection)
{
	SeqClip *clip = selection->GetClip();
	fromSceneId = clip->GetParent()->GetParent()->id;
	fromChannelId = clip->GetParent()->actionId;
	fromClipId = clip->actionId;
	fromLeftTime = clip->location.leftTime;
	toSceneId = selection->GetParent()->GetParent()->id;
	toChannelId = selection->GetParent()->actionId;
	toClipId = -1; // filled after moving the clip
	toLeftTime = selection->location.leftTime;
}
