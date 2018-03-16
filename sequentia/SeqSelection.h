#pragma once
#include "SDL_config.h"
#include "SeqClipLocation.h"

class SeqChannel;
class SeqClip;

class SeqSelection
{
public:
	SeqSelection();
	~SeqSelection();

	void Activate(SeqClip *childClip);
	void Deactivate();
	bool IsActive();
	bool IsNewClip();
	bool IsMoved();

	SeqClip* GetClip();
	SeqChannel* GetParent();
	void SetPosition(int64_t leftTime);
	void SetParent(SeqChannel *channel);

public:
	int64_t grip;
	SeqClipLocation location;
private:
	bool isActive;
	SeqClip *clip;
};
