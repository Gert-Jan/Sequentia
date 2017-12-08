#include "SeqAction.h"
#include "Sequentia.h"
#include "seqProjectHeaders.h"

SeqActionAddClipToChannel::SeqActionAddClipToChannel(SeqClip* clip)
{
	SeqLibrary* library = Sequentia::GetCurrentProject()->GetLibrary();
	SeqChannel* channel = clip->GetParent();
	channelId = channel->actionId;
	libraryLinkIndex = library->GetLinkIndex(clip->GetLink()->fullPath);
	clipId = clip->actionId;
	leftTime = clip->leftTime;
	rightTime = clip->rightTime;
	startTime = clip->startTime;
}