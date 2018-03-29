#pragma once

#include "SDL_config.h"
#include "SDL_audio.h"
#include <imgui.h>
#include "SeqAction.h"
#include "SeqTime.h"

class SeqScene;
class SeqClip;
class SeqTaskDecodeVideo;
class SeqMaterialInstance;

template<class T>
class SeqList;

struct AVFrame;

struct SeqClipPlayerVideo
{
	SeqMaterialInstance *material;
	int maxLineSize;
};

struct SeqClipPlayerAudio
{
	SDL_AudioDeviceID deviceId;
};

struct SeqClipPlayer
{
	SeqClip *clip;
	SeqTaskDecodeVideo *decoderTask;
	AVFrame *lastFrame;
	bool isWaitingForSeek;
	union
	{
		SeqClipPlayerVideo videoPlayer;
		SeqClipPlayerAudio audioPlayer;
	};
};

enum class SeqPlayerSyncState
{
	INDEPENDANT,
	INVALID_SYNC,
	SYNCED
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

	void PreExecuteAction(const SeqActionType type, const SeqActionExecution execution, const void *data);

private:
	SeqClipPlayer* GetClipPlayerFor(SeqClip *clip);
	void CreateDecoderTaskFor(SeqClipPlayer *player);
	int GetClipPlayerIndexFor(SeqClip *clip);
	SeqClipPlayer* GetSyncedClipPlayerFor(SeqClipPlayer* player);
	SeqClipPlayer* GetSyncableClipPlayer(SeqClip *clip);
	bool ClipsSyncable(SeqClip *clipA, SeqClip *clipB);
	SeqPlayerSyncState ValidateSyncing(SeqClipPlayer *player);
	void DisposeClipPlayerAt(const int index);
	void SafeStopDecodingStream(SeqClipPlayer* palyer);
	void UpdateClipPlayers(bool *canPlay);
	void Render(const int fromChannelIndex, const int toChannelIndex);

private:
	static ImU32 LOADING_FRAME_COLOR;
	static const int64_t DECODER_SYNC_TOLERANCE = SEQ_TIME_FROM_MILLISECONDS(100);
	SeqScene *scene;
	SeqList<SeqClipPlayer> *clipPlayers;
	SeqList<int> *viewers;
	SeqList<SeqPlayerRenderTarget> *renderTargets;
	bool isPlaying;
	bool isSeeking;
	int64_t playTime;
	Uint32 lastMeasuredTime;
};
