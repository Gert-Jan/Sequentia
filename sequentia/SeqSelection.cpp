#include "SeqSelection.h"
#include "SeqClip.h"
#include "SeqChannel.h"

SeqSelection::SeqSelection():
	isActive(false),
	grip(0),
	location(SeqClipLocation()),
	clip(clip)
{
}

SeqSelection::~SeqSelection()
{
}

void SeqSelection::Activate(SeqClip *childClip)
{
	clip = childClip;
	isActive = true;
}

void SeqSelection::Deactivate()
{
	isActive = false;
	if (location.parent != nullptr)
		location.parent->RemoveClipProxy(this);
	location.Reset();
	grip = 0;
	clip = nullptr;
}

bool SeqSelection::IsActive()
{
	return isActive;
}

bool SeqSelection::IsNewClip()
{
	return clip->GetParent() == nullptr;
}

bool SeqSelection::IsMoved()
{
	return
		clip->GetParent() != GetParent() ||
		clip->location.leftTime != location.leftTime;
}

SeqClip* SeqSelection::GetClip()
{
	return clip;
}

void SeqSelection::SetPosition(int64_t newLeftTime)
{
	location.parent->MoveClipProxy(this, newLeftTime);
}

void SeqSelection::SetParent(SeqChannel* channel)
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

SeqChannel* SeqSelection::GetParent()
{
	return location.parent;
}
