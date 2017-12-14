#include "SeqClipProxy.h"
#include "SeqClip.h"
#include "SeqChannel.h"

SeqClipProxy::SeqClipProxy() :
	isActive(false),
	location(SeqClipLocation()),
	clip(clip)
{
}

SeqClipProxy::~SeqClipProxy()
{
}

void SeqClipProxy::Activate(SeqClip *childClip)
{
	clip = childClip;
	isActive = true;
}

void SeqClipProxy::Deactivate()
{
	isActive = false;
	if (location.parent != nullptr)
		location.parent->RemoveClipProxy(this);
	location.Reset();
	clip = nullptr;
}

bool SeqClipProxy::IsActive()
{
	return isActive;
}

bool SeqClipProxy::IsNewClip()
{
	return clip->GetParent() == nullptr;
}

SeqClip* SeqClipProxy::GetClip()
{
	return clip;
}

void SeqClipProxy::SetPosition(int64_t newLeftTime)
{
	location.parent->MoveClipProxy(this, newLeftTime);
}

void SeqClipProxy::SetParent(SeqChannel* channel)
{
	if (location.parent != channel)
	{
		// remove the proxy from current parent
		if (location.parent != nullptr)
			location.parent->RemoveClipProxy(this);
		// set new parent
		location.parent = channel;
		// add proxy to new parent
		if (channel != nullptr)
			channel->AddClipProxy(this);
	}
}

SeqChannel* SeqClipProxy::GetParent()
{
	return location.parent;
}
