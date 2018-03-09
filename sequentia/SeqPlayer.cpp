#include <SDL.h>
#include <imgui.h>

#include "SeqPlayer.h"
#include "SeqProjectHeaders.h"
#include "Sequentia.h"
#include "SeqWorkerManager.h"
#include "SeqTaskDecodeVideo.h"
#include "SeqDecoder.h"
#include "SeqRenderer.h"
#include "SeqMaterialInstance.h"
#include "SeqList.h"
#include "SeqTime.h"

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

	// there was only one render target representing this untilChannel, now there are none, remove the render targer
	if (similairCount == 1)
	{
		for (int i = 0; i < renderTargets->Count(); i++)
		{
			SeqPlayerRenderTarget renderTarget = renderTargets->Get(i);
			if (renderTarget.untilChannel == untilChannel)
			{
				SeqRenderer::RemoveMaterialInstance(renderTarget.target);
				renderTargets->RemoveAt(i);
				return;
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

				// get the decoder
				SeqDecoder *decoder = clipPlayer->decoderTask->GetDecoder();

				// continue if the decoder is not ready yet
				if (decoder->GetStatus() != SeqDecoderStatus::Loading &&
					decoder->GetStatus() != SeqDecoderStatus::Ready)
				{
					*canPlay = false;
					continue;
				}
				// make sure the decoder is playing the right part of the video
				int64_t requestTime = clip->location.startTime + (playTime - clip->location.leftTime);
				if (!clipPlayer->isWaitingForSeek && 
					(requestTime < decoder->GetBufferLeft() - SEQ_TIME_FROM_MILLISECONDS(500) ||
					requestTime > decoder->GetBufferRight() + SEQ_TIME_FROM_MILLISECONDS(500)))
				{
					decoder->Seek(requestTime);
					clipPlayer->isWaitingForSeek = true;
					*canPlay = false;
				}
				else if (clipPlayer->isWaitingForSeek)
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
				
				// move the frame to the GPU if the decoder is ready
				AVFrame *frame = decoder->NextFrame(clip->streamInfo.streamIndex, requestTime);
				if (SeqDecoder::IsValidFrame(frame) && frame != clipPlayer->lastFrame)
				{
					if (clipPlayer->lastFrame == nullptr ||
						frame->linesize[0] > clipPlayer->maxLineSize)
					{
						clipPlayer->maxLineSize = frame->linesize[0];
						SeqRenderer::CreateVideoTextures(frame, clipPlayer->material->textureHandles);
					}
					else
					{
						SeqRenderer::OverwriteVideoTextures(frame, clipPlayer->material->textureHandles);
					}

					clipPlayer->lastFrame = frame;
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
		if (clip != nullptr)
		{
			SeqClipPlayer *player = GetClipPlayerFor(clip);
			if (player == nullptr)
				continue;
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			SeqProject* project = Sequentia::GetProject();
			if (player->lastFrame != nullptr)
			{
				drawList->AddImage(player->material, 
					ImVec2(0, 0), ImVec2(project->width, project->height), 
					ImVec2(0, 0), ImVec2((float)player->lastFrame->width / (float)player->maxLineSize, 1));
			}
		}
	}
}


void SeqPlayer::ActionDone(const SeqAction action)
{
	switch (action.type)
	{
		case SeqActionType::AddClipToChannel:
			SeqActionAddClipToChannel *data = (SeqActionAddClipToChannel*)action.data;
			if (data->sceneId == scene->id)
			{
				// removing a clip for a channel
				if (action.execution == SeqActionExecution::Undo)
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

void SeqPlayer::ActionUndone(const SeqAction action)
{
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
		player->decoderTask = new SeqTaskDecodeVideo(link);
		if (clip->streamInfo.streamIndex >= 0)
		{
			player->decoderTask->StartDecodeStreamIndex(clip->streamInfo.streamIndex);
		}
		player->material = SeqRenderer::CreateVideoMaterialInstance();
		player->lastFrame = nullptr;
		player->maxLineSize = 0;
		player->isWaitingForSeek = false;
		SeqWorkerManager::Instance()->PerformTask(player->decoderTask);
		return player;
	}
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

void SeqPlayer::DisposeClipPlayerAt(const int index)
{
	SeqClipPlayer player = clipPlayers->Get(index);
	player.decoderTask->Stop();
	SeqRenderer::RemoveMaterialInstance(player.material);
}
