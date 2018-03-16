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
SeqActionAddClipToChannel::SeqActionAddClipToChannel(SeqSelection *proxy)
{
	SeqLibrary *library = Sequentia::GetLibrary();
	SeqChannel *channel = proxy->GetParent();
	sceneId = channel->GetParent()->id;
	channelId = channel->actionId;
	libraryLinkIndex = library->GetLinkIndex(proxy->GetClip()->GetLink()->fullPath);
	streamIndex = proxy->GetClip()->streamIndex;
	clipId = -1; // filled in after placing the new clip
	leftTime = proxy->location.leftTime;
	rightTime = proxy->location.rightTime;
	startTime = proxy->location.startTime;
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

SeqActionMoveClip::SeqActionMoveClip(SeqSelection *proxy)
{
	SeqClip *clip = proxy->GetClip();
	fromSceneId = clip->GetParent()->GetParent()->id;
	fromChannelId = clip->GetParent()->actionId;
	fromClipId = clip->actionId;
	fromLeftTime = clip->location.leftTime;
	toSceneId = proxy->GetParent()->GetParent()->id;
	toChannelId = proxy->GetParent()->actionId;
	toClipId = -1; // filled after moving the clip
	toLeftTime = proxy->location.leftTime;
}
