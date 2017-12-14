#include "SeqAction.h"
#include "Sequentia.h"
#include "SeqProjectHeaders.h"

// on add
SeqActionAddClipToChannel::SeqActionAddClipToChannel(SeqClipProxy* proxy)
{
	SeqLibrary *library = Sequentia::GetCurrentProject()->GetLibrary();
	SeqChannel *channel = proxy->GetParent();
	channelId = channel->actionId;
	libraryLinkIndex = library->GetLinkIndex(proxy->GetClip()->GetLink()->fullPath);
	clipId = -1; // filled in after placing the new clip
	leftTime = proxy->location.leftTime;
	rightTime = proxy->location.rightTime;
	startTime = proxy->location.startTime;
}

// on remove
SeqActionAddClipToChannel::SeqActionAddClipToChannel(SeqClip* clip)
{
	SeqLibrary *library = Sequentia::GetCurrentProject()->GetLibrary();
	SeqChannel *channel = clip->GetParent();
	channelId = channel->actionId;
	libraryLinkIndex = library->GetLinkIndex(clip->GetLink()->fullPath);
	clipId = clip->actionId;
	leftTime = clip->location.leftTime;
	rightTime = clip->location.rightTime;
	startTime = clip->location.startTime;
}

SeqActionMoveClipToChannel::SeqActionMoveClipToChannel(SeqClipProxy* proxy)
{
	SeqClip *clip = proxy->GetClip();
	fromChannelId = clip->GetParent()->actionId;
	fromClipId = clip->actionId;
	toChannelId = proxy->GetParent()->actionId;
	toClipId = -1; // filled after moving the clip
}
