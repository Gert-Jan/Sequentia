#pragma once

#include "SDL_config.h"
#include "SeqAction.h"

class SeqScene;
class SeqClip;
class SeqTaskDecodeVideo;
class SeqMaterialInstance;

template<class T>
class SeqList;

struct AVFrame;

struct SeqClipPlayer
{
	SeqClip *clip;
	SeqTaskDecodeVideo *decoderTask;	
	SeqMaterialInstance *material;
	AVFrame *lastFrame;
	int maxLineSize;
	bool isWaitingForSeek;
};

struct SeqPlayerRenderTarget
{
	int untilChannel;
	SeqMaterialInstance *target;
};

class SeqPlayer : public SeqActionHandler
{
public:
	SeqPlayer(SeqScene *scene);
	~SeqPlayer();

	SeqMaterialInstance* AddViewer(int untilChannel);
	void RemoveViewer(int untilChannel);

	bool IsPlaying();
	int64_t GetPlaybackTime();
	int64_t GetDuration();

	void Play();
	void Pause();
	void Stop();
	void Seek(int64_t time);
	bool IsActive();
	void Update();
	void Render();

	void ActionDone(const SeqAction action);
	void ActionUndone(const SeqAction action);

private:
	SeqClipPlayer* GetClipPlayerFor(SeqClip *clip);
	int GetClipPlayerIndexFor(SeqClip *clip);
	void DisposeClipPlayerAt(const int index);
	void UpdateClipPlayers(bool *canPlay);
	void Render(const int fromChannelIndex, const int toChannelIndex);

private:
	SeqScene *scene;
	SeqList<SeqClipPlayer> *clipPlayers;
	SeqList<int> *viewers;
	SeqList<SeqPlayerRenderTarget> *renderTargets;
	bool isPlaying;
	bool isSeeking;
	int64_t playTime;
	Uint32 lastMeasuredTime;
};
