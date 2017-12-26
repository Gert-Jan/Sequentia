#include "SeqProjectHeaders.h"
#include "SeqPlayer.h"

SeqPlayer::SeqPlayer(SeqScene *scene):
	activateCount(0),
	scene(scene),
	isSeeking(false),
	seekVideoTime(0),
	startVideoTime(0)
{
}

SeqPlayer::~SeqPlayer()
{
}

void SeqPlayer::Play()
{
}

void SeqPlayer::Pause()
{
}

void SeqPlayer::Stop()
{
}

void SeqPlayer::Seek(int64_t time)
{
}

bool SeqPlayer::IsActive()
{
	return activateCount > 0;
}
