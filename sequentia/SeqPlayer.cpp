#include <SDL.h>

#include "SeqPlayer.h"
#include "SeqProjectHeaders.h"
#include "Sequentia.h"
#include "SeqWorkerManager.h"
#include "SeqTaskDecodeVideo.h"
#include "SeqStreamInfo.h"
#include "SeqDecoder.h"
#include "SeqRenderer.h"
#include "SeqMaterialInstance.h"
#include "SeqList.h"
#include "SeqTime.h"

ImU32 SeqPlayer::LOADING_FRAME_COLOR = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 1, 0, 1));

SeqPlayer::SeqPlayer(SeqScene *scene):
	scene(scene),
	isPlaying(false),
	isSeeking(false),
	playTime(0)
{
	clipPlayers = new SeqList<SeqClipPlayer>();
	viewers = new SeqList<int>();
	renderTargets = new SeqList<SeqPlayerRenderTarget>();
	lastMeasuredTime = SDL_GetTicks();
	// start listening for project changes
	Sequentia::GetProject()->AddActionHandler(this);
}

SeqPlayer::~SeqPlayer()
{
	// stop listening for project changes
	Sequentia::GetProject()->RemoveActionHandler(this);

	for (int i = 0; i < clipPlayers->Count(); i++)
	{
		DisposeClipPlayerAt(i);
	}
	delete clipPlayers;
}

SeqMaterialInstance* SeqPlayer::AddViewer(int untilChannel)
{
	// first see if we can find a similair existing render target, return that one
	for (int i = 0; i < renderTargets->Count(); i++)
	{
		if (renderTargets->Get(i).untilChannel == untilChannel)
		{
			viewers->Add(untilChannel);
			return renderTargets->Get(i).target;
		}
	}

	// if no similair existing render target:
	// find where to insert a new render target
	int insertAt = 0;
	for (int i = 1; i < renderTargets->Count(); i++)
	{
		if (untilChannel < renderTargets->Get(i).untilChannel)
			insertAt = i;
		else
			break;
	}
	// create new render target
	SeqMaterialInstance *target = SeqRenderer::CreatePlayerMaterialInstance();
	SeqPlayerRenderTarget renderTarget = SeqPlayerRenderTarget();
	renderTarget.untilChannel = untilChannel;
	renderTarget.target = target;
	// add render target
	viewers->Add(untilChannel);
	renderTargets->InsertAt(renderTarget, insertAt);
	
	return target;
}

void SeqPlayer::RemoveViewer(int untilChannel)
{
	// remove from the viewers, see if more viewers depend on the same render target
	int similairCount = 0;
	for (int i = 0; i < viewers->Count(); i++)
	{
		if (viewers->Get(i) == untilChannel)
		{
			if (similairCount == 0)
			{
				viewers->RemoveAt(i);
				i--;
			}
			similairCount++;
		}
	}

	// there was only one render target representing this untilChannel, now there are none, remove the render target
	if (similairCount == 1)
	{
		for (int i = 0; i < renderTargets->Count(); i++)
		{
			SeqPlayerRenderTarget renderTarget = renderTargets->Get(i);
			if (renderTarget.untilChannel == untilChannel)
			{
				SeqRenderer::RemoveMaterialInstance(renderTarget.target);
				renderTargets->RemoveAt(i);
				break;
			}
		}
	}

	// if there are no viewers left, remove all clipPlayers
	if (renderTargets->Count() == 0)
	{
		for (int i = 0; i < clipPlayers->Count(); i++)
		{
			DisposeClipPlayerAt(i);
		}
		clipPlayers->Clear();
	}
}

bool SeqPlayer::IsPlaying()
{
	return isPlaying;
}

int64_t SeqPlayer::GetPlaybackTime()
{
	return playTime;
}

int64_t SeqPlayer::GetDuration()
{
	return scene->GetLength();
}

void SeqPlayer::Play()
{
	isPlaying = true;
}

void SeqPlayer::Pause()
{
	isPlaying = false;
}

void SeqPlayer::Stop()
{
	isPlaying = false;
	Seek(0);
}

void SeqPlayer::Seek(int64_t time)
{
	playTime = time;
	// reset all seek waiting states so we can seek again.
	for (int i = 0; i < clipPlayers->Count(); i++)
		clipPlayers->GetPtr(i)->isWaitingForSeek = false;
}

bool SeqPlayer::IsActive()
{
	return renderTargets->Count() > 0;
}

void SeqPlayer::Update()
{
	// early exit if the player is displayed nowhere or is not playing
	if (!IsActive() || !isPlaying)
	{
		lastMeasuredTime = SDL_GetTicks();
		return;
	}

	// keep track if we can contintue playback
	bool canPlay = true;

	// update all clip players
	UpdateClipPlayers(&canPlay);

	if (isPlaying)
	{
		// increment playtime if possible
		Uint32 measuredTime = SDL_GetTicks();
		if (canPlay)
		{
			playTime += SEQ_TIME_FROM_MILLISECONDS(measuredTime - lastMeasuredTime);
			if (playTime >= scene->GetLength())
			{
				isPlaying = false;
				playTime = scene->GetLength();
			}
		}
		lastMeasuredTime = measuredTime;
	}
}

void SeqPlayer::UpdateClipPlayers(bool *canPlay)
{
	// always assume we can play at first
	*canPlay = true;

	// define bounds in which we want clip players to be ready and waiting
	int64_t leftTime = playTime - SEQ_TIME(2);
	int64_t rightTime = playTime + SEQ_TIME(2);

	// spawn new clip players, check if we can continue playback
	for (int i = 0; i < scene->ChannelCount(); i++)
	{
		SeqChannel *channel = scene->GetChannel(i);
		for (int j = 0; j < channel->ClipCount(); j++)
		{
			SeqClip *clip = channel->GetClip(j);
			// we passed all intresting clips and can stop looking now
			if (clip->location.leftTime > rightTime)
				break;
			// these clips are now playing
			if (clip->location.ContainsTime(playTime))
			{
				// get or create a clip player
				SeqClipPlayer *clipPlayer = GetClipPlayerFor(clip);
				// the clip link meta data is not always loaded, than we can't make a clip player yet...
				if (clipPlayer == nullptr)
					continue;

				// check if we are syncing with another player and if that sync is still valid
				SeqPlayerSyncState syncState = ValidateSyncing(clipPlayer);
				if (syncState == SeqPlayerSyncState::INVALID_SYNC)
				{
					// the sync was no longer valid, stop using the decoder of the synced to player
					SafeStopDecodingStream(clipPlayer);
					// check if the player can sync with another player
					SeqClipPlayer *syncablePlayer = GetSyncableClipPlayer(clip);
					if (syncablePlayer != nullptr)
					{
						// sync with found player
						clipPlayer->decoderTask = syncablePlayer->decoderTask;
						clipPlayer->decoderTask->StartDecodeStreamIndex(clipPlayer->clip->streamIndex);
					}
					else
					{
						// there was no clip to sync with spawn a new task
						CreateDecoderTaskFor(clipPlayer);
					}
				}
				else if (syncState == SeqPlayerSyncState::INDEPENDANT)
				{
					// not synced, see if there are possibilities to sync
					SeqClipPlayer *syncablePlayer = GetSyncableClipPlayer(clip);
					if (syncablePlayer != nullptr)
					{
						// stop the current decoder task
						clipPlayer->decoderTask->Stop();
						// sync with found player
						clipPlayer->decoderTask = syncablePlayer->decoderTask;
						clipPlayer->decoderTask->StartDecodeStreamIndex(clipPlayer->clip->streamIndex);
					}
				}

				// get the decoder
				SeqDecoder *decoder = clipPlayer->decoderTask->GetDecoder();

				// continue if the decoder is not ready yet
				if (decoder == nullptr || 
					(decoder->GetStatus() != SeqDecoderStatus::Loading &&
					decoder->GetStatus() != SeqDecoderStatus::Ready))
				{
					*canPlay = false;
					continue;
				}
				// make sure the decoder is playing the right part of the video
				int64_t requestTime = clip->location.startTime + (playTime - clip->location.leftTime);
				if (!clipPlayer->isWaitingForSeek && 
					(requestTime < decoder->GetBufferLeft() ||
					requestTime > decoder->GetBufferRight()))
				{
					decoder->Seek(requestTime);
					clipPlayer->isWaitingForSeek = true;
					*canPlay = false;
				}
				else if (decoder->GetStatus() == SeqDecoderStatus::Seeking)
				{
					if (requestTime >= decoder->GetBufferLeft() &&
						requestTime <= decoder->GetBufferRight())
					{
						clipPlayer->isWaitingForSeek = false;
					}
					else
					{
						*canPlay = false;
					}
				}

				// stop decoding frames if we reached the end of the stream
				if (decoder->IsAtEndOfStream(clip->streamIndex))
					continue;
				
				// move the frame to the GPU if the decoder is ready
				AVFrame *frame = decoder->NextFrame(clip->streamIndex, requestTime);
				if (SeqDecoder::IsValidFrame(frame))
				{
					// check if we got the right frame for the given time
					if (decoder->GetSeqTimeForStreamTime(frame->pkt_dts, clip->streamIndex) < requestTime)
					{
						// the decoder did not catch up yet, stop the player for now until it did
						*canPlay = false;
					}
					// yes, we got a frame for the right time, check if the frame has changed since the last displayed frame
					else if (frame != clipPlayer->lastFrame)
					{
						// display frame
						SeqStreamInfo *streamInfo = clip->GetStreamInfo();
						if (streamInfo->type == SeqStreamInfoType::Video)
						{
							if (clipPlayer->lastFrame == nullptr ||
								frame->linesize[0] > clipPlayer->videoPlayer.maxLineSize)
							{
								clipPlayer->videoPlayer.maxLineSize = frame->linesize[0];
								SeqRenderer::CreateVideoTextures(frame, clipPlayer->videoPlayer.material->textureHandles);
							}
							else
							{
								SeqRenderer::OverwriteVideoTextures(frame, clipPlayer->videoPlayer.material->textureHandles);
							}
						}
						else if (streamInfo->type == SeqStreamInfoType::Audio)
						{
							int bytesPerSample = (streamInfo->audioInfo.format & 0xff) / 8;
							const int dataSizeInBytes = frame->nb_samples * bytesPerSample;
							if (streamInfo->audioInfo.isPlanar)
							{
								// the channel data is planar and needs to be interleaved for the SDL audio queue to be played
								int channels = streamInfo->audioInfo.channelCount;
								const int bufSize = dataSizeInBytes * channels;
								char* buf = new char[bufSize];
								int bufCursor = 0;
								for (int dataCursor = 0; dataCursor < dataSizeInBytes; dataCursor += bytesPerSample)
								{
									for (int channel = 0; channel < channels; channel++)
									{
										memcpy(&buf[bufCursor], &frame->data[channel][dataCursor], bytesPerSample);
										bufCursor += bytesPerSample;
									}
								}
								SDL_QueueAudio(clipPlayer->audioPlayer.deviceId, buf, bufSize);
								delete buf;
							}
							else
							{
								// the channel data is already interleaved, we can immediatly queue it
								SDL_QueueAudio(clipPlayer->audioPlayer.deviceId, frame->data[0], dataSizeInBytes);
							}
						}

						clipPlayer->lastFrame = frame;
					}
				}

				if (clipPlayer->lastFrame == nullptr)
					*canPlay = false;
			}
			// in preload area
			else if (clip->location.OverlapsTimeRange(leftTime, rightTime))
			{
				// make sure the clip player exist
				GetClipPlayerFor(clip);
			}
		}
	}

	// cleanup clip players that are outside of preload bounds
	for (int i = 0; i < clipPlayers->Count(); i++)
	{
		SeqClipPlayer clipPlayer = clipPlayers->Get(i);
		if (!clipPlayer.clip->location.OverlapsTimeRange(leftTime, rightTime))
		{
			DisposeClipPlayerAt(i);
			clipPlayers->RemoveAt(i);
			i--;
		}
	}
}

void SeqPlayer::Render()
{
	if (renderTargets->Count() == 0)
		return;
	SeqProject* project = Sequentia::GetProject();
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiSetCond_Always);
	ImGui::SetNextWindowSize(ImVec2(project->width, project->height), ImGuiSetCond_Always);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	// this is kind of ugly, but with alpha == 0 the window gets deactivated automatically in ImGui::Begin...
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.00001);
	ImGui::Begin("PlayerRenderer", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs);
	ImGui::PushClipRect(ImVec2(0, 0), ImVec2(project->width, project->height), false);
	int fromChannelIndex = scene->ChannelCount() - 1;
	for (int i = 0; i < renderTargets->Count(); i++)
	{
		SeqPlayerRenderTarget renderTarget = renderTargets->Get(i);
		// bind frame buffer
		if (i == 0)
			ImGui::GetWindowDrawList()->AddCallback(SeqRenderer::BindFramebuffer, renderTarget.target);
		else
			ImGui::GetWindowDrawList()->AddCallback(SeqRenderer::SwitchFramebuffer, renderTarget.target);
		Render(fromChannelIndex, renderTarget.untilChannel);
		fromChannelIndex = renderTarget.untilChannel - 1;
	}
	// unbind frame buffer
	ImGui::GetWindowDrawList()->AddCallback(SeqRenderer::BindFramebuffer, nullptr);
	ImGui::PopClipRect();
	ImGui::End();
	ImGui::PopStyleVar(1);
	ImGui::PopStyleVar(2);
}

void SeqPlayer::Render(const int fromChannelIndex, const int toChannelIndex)
{
	for (int i = fromChannelIndex; i >= toChannelIndex; i--)
	{
		SeqChannel *channel = scene->GetChannel(i);
		SeqClip *clip = channel->GetClipAt(playTime);
		if (clip != nullptr && clip->GetLink()->metaDataLoaded && 
			clip->GetStreamInfo()->type == SeqStreamInfoType::Video)
		{
			SeqClipPlayer *player = GetClipPlayerFor(clip);
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			SeqProject* project = Sequentia::GetProject();
			if (player->lastFrame != nullptr)
			{
				drawList->AddImage(player->videoPlayer.material,
					ImVec2(0, 0), ImVec2(project->width, project->height),
					ImVec2(0, 0), ImVec2((float)player->lastFrame->width / (float)player->videoPlayer.maxLineSize, 1));
			}
			else
			{
				drawList->AddRectFilled(ImVec2(0, 0), ImVec2(project->width, project->height), LOADING_FRAME_COLOR);
			}
		}
	}
}

void SeqPlayer::PreExecuteAction(const SeqActionType type, const SeqActionExecution execution, const void *actionData)
{
	switch (type)
	{
		case SeqActionType::AddClipToChannel:
		{
			SeqActionAddClipToChannel *data = (SeqActionAddClipToChannel*)actionData;
			if (data->sceneId == scene->id)
			{
				// removing a clip for a channel
				if (execution == SeqActionExecution::Undo)
				{
					// dispose the clip player if we have a clip player associated to the clip that is being removed
					SeqClip *clip = scene->GetChannelByActionId(data->channelId)->GetClipByActionId(data->clipId);
					int index = GetClipPlayerIndexFor(clip);
					if (index > -1)
					{
						DisposeClipPlayerAt(index);
						clipPlayers->RemoveAt(index);
					}
				}
			}
			break;
		}
	}
}

SeqClipPlayer* SeqPlayer::GetClipPlayerFor(SeqClip *clip)
{
	// see if we already have a decoder here
	int index = GetClipPlayerIndexFor(clip);
	if (index > -1)
	{
		return clipPlayers->GetPtr(index);
	}
	else
	{
		// see if the link in clip is loaded enough to create a SeqClipPlayer
		SeqLibraryLink *link = clip->GetLink();
		if (!link->metaDataLoaded)
			return nullptr;
		// could not find an existing decoder, create one
		clipPlayers->Add(SeqClipPlayer());
		SeqClipPlayer *player = clipPlayers->GetPtr(clipPlayers->Count() - 1);
		player->clip = clip;
		player->lastFrame = nullptr;
		player->isWaitingForSeek = false;
		SeqStreamInfo *streamInfo = clip->GetStreamInfo();
		if (streamInfo->type == SeqStreamInfoType::Video)
		{
			player->videoPlayer.material = SeqRenderer::CreateVideoMaterialInstance();
			player->videoPlayer.maxLineSize = 0;
		}
		else if (streamInfo->type == SeqStreamInfoType::Audio)
		{
			SDL_AudioSpec audioSpec = SDL_AudioSpec();
			audioSpec.freq = streamInfo->audioInfo.sampleRate;
			audioSpec.format = streamInfo->audioInfo.format;
			audioSpec.channels = streamInfo->audioInfo.channelCount;
			audioSpec.samples = 1024;
			player->audioPlayer.deviceId = SDL_OpenAudioDevice(nullptr, false, &audioSpec, nullptr, true);
			SDL_PauseAudioDevice(player->audioPlayer.deviceId, false);
		}
		// create decoder task
		SeqClipPlayer *syncablePlayer = GetSyncableClipPlayer(clip);
		if (syncablePlayer == nullptr)
		{
			CreateDecoderTaskFor(player);
		}
		else
		{
			player->decoderTask = syncablePlayer->decoderTask;
			player->decoderTask->StartDecodeStreamIndex(clip->streamIndex);
		}
		return player;
	}
}

void SeqPlayer::CreateDecoderTaskFor(SeqClipPlayer *player)
{
	player->decoderTask = new SeqTaskDecodeVideo(player->clip->GetLink());
	SeqWorkerManager::Instance()->PerformTask(player->decoderTask);
	player->decoderTask->StartDecodeStreamIndex(player->clip->streamIndex);
}

int SeqPlayer::GetClipPlayerIndexFor(SeqClip *clip)
{
	for (int i = 0; i < clipPlayers->Count(); i++)
	{
		SeqClipPlayer *player = clipPlayers->GetPtr(i);
		if (player->clip == clip)
			return i;
	}
	return -1;
}

SeqClipPlayer* SeqPlayer::GetSyncedClipPlayerFor(SeqClipPlayer *player)
{
	for (int i = 0; i < clipPlayers->Count(); i++)
	{
		SeqClipPlayer *otherPlayer = clipPlayers->GetPtr(i);
		if (otherPlayer != player &&
			otherPlayer->decoderTask == player->decoderTask)
		{
			return clipPlayers->GetPtr(i);
		}
	}
	return nullptr;
}

// find a player of which we can use the SeqTaskDecodeVideo from as they are synced up in time and use the same media content.
SeqClipPlayer* SeqPlayer::GetSyncableClipPlayer(SeqClip *clip)
{
	for (int i = 0; i < clipPlayers->Count(); i++)
	{
		SeqClipPlayer player = clipPlayers->Get(i);
		SeqClip *otherClip = player.clip;
		if (ClipsSyncable(clip, otherClip))
		{
			return clipPlayers->GetPtr(i);
		}
	}
	return nullptr;
}

bool SeqPlayer::ClipsSyncable(SeqClip *clipA, SeqClip *clipB)
{
	return clipA != clipB &&
		clipA->GetLink() == clipB->GetLink() &&
		abs(clipA->location.VideoStartTime() - clipB->location.VideoStartTime()) < DECODER_SYNC_TOLERANCE;
}

SeqPlayerSyncState SeqPlayer::ValidateSyncing(SeqClipPlayer *player)
{
	SeqClipPlayer *syncedPlayer = GetSyncedClipPlayerFor(player);
	// no sync active so it's valid
	if (syncedPlayer == nullptr)
	{
		return SeqPlayerSyncState::INDEPENDANT;
	}
	else if (ClipsSyncable(player->clip, syncedPlayer->clip))
	{
		return SeqPlayerSyncState::SYNCED;
	}
	else
	{
		return SeqPlayerSyncState::INVALID_SYNC;
	}
}

void SeqPlayer::DisposeClipPlayerAt(const int index)
{
	SeqClipPlayer *player = clipPlayers->GetPtr(index);
	SafeStopDecodingStream(player);
	SeqClipPlayer *syncedClipPlayer = GetSyncedClipPlayerFor(player);
	if (syncedClipPlayer == nullptr)
	{
		player->decoderTask->Stop();
	}
	SeqStreamInfo *streamInfo = player->clip->GetStreamInfo();
	if (streamInfo->type == SeqStreamInfoType::Video)
	{
		SeqRenderer::RemoveMaterialInstance(player->videoPlayer.material);
	}
	else if (streamInfo->type == SeqStreamInfoType::Audio)
	{
		SDL_CloseAudioDevice(player->audioPlayer.deviceId);
	}
}

void SeqPlayer::SafeStopDecodingStream(SeqClipPlayer *player)
{
	// makes sure not to stop a stream that is also used by another player
	for (int i = 0; i < clipPlayers->Count(); i++)
	{
		SeqClipPlayer *otherPlayer = clipPlayers->GetPtr(i);
		if (player != otherPlayer &&
			player->decoderTask == otherPlayer->decoderTask &&
			player->clip->streamIndex == otherPlayer->clip->streamIndex)
		{
			return;
		}
	}
	// we can safely stop the associated stream
	player->decoderTask->StopDecodedStreamIndex(player->clip->streamIndex);
}
