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
		location.parent->RemoveClipSelection(this);
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
	location.parent->MoveClipSelection(this, newLeftTime);
}

void SeqSelection::SetParent(SeqChannel* channel)
{
	if (location.parent != channel)
	{
		// remove the selection from current parent
		if (location.parent != nullptr)
			location.parent->RemoveClipSelection(this);
		// set new parent
		location.parent = channel;
		// add selection to new parent
		if (channel != nullptr)
			channel->AddClipSelection(this);
	}
}

SeqChannel* SeqSelection::GetParent()
{
	return location.parent;
}
