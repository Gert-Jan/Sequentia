#pragma once

#include "SDL_config.h"
#include "SeqClipLocation.h"

class SeqChannel;
class SeqClip;

class SeqClipProxy
{
public:
	SeqClipProxy();
	~SeqClipProxy();

	void Activate(SeqClip *childClip);
	void Deactivate();
	bool IsActive();
	bool IsNewClip();

	SeqClip* GetClip();
	SeqChannel* GetParent();
	void SetPosition(int64_t leftTime);
	void SetParent(SeqChannel *channel);

public:
	SeqClipLocation location;
private:
	bool isActive;
	SeqClip *clip;
};
