#include <imgui.h>
#include <imgui_internal.h>
#include <SDL.h>
#include "SeqUIVideo.h"
#include "Sequentia.h"
#include "SeqProjectHeaders.h"
#include "SeqWorkerManager.h"
#include "SeqTaskDecodeVideo.h"
#include "SeqUtils.h"
#include "SeqString.h"
#include "SeqRenderer.h"
#include "SeqPlayer.h"
#include "SeqMaterialInstance.h"
#include "SeqWidgets.h"
#include "SeqSerializer.h"
#include "SeqTime.h"

extern "C"
{
	#include "libavformat/avformat.h"
}

SeqUIVideo::SeqUIVideo() :
	isSeeking(false),
	seekVideoTime(0),
	scene(nullptr)
{
	Init();
	InitScene();
}

SeqUIVideo::SeqUIVideo(SeqSerializer *serializer):
	isSeeking(false),
	seekVideoTime(0),
	scene(nullptr)
{
	Init();
	Deserialize(serializer);
	InitScene();
}

void SeqUIVideo::Init()
{
	SeqProject *project = Sequentia::GetProject();
	// alloc memory
	SeqString::Temp->Format("Video##%d", project->NextWindowId());
	name = SeqString::Temp->Copy();
	// start listening for project changes
	project->AddActionHandler(this);
}

void SeqUIVideo::InitScene()
{
	if (scene == nullptr)
	{
		SeqProject *project = Sequentia::GetProject();
		scene = project->GetPreviewScene();
	}
	// prepare material
	material = scene->player->AddViewer(0);
}

SeqUIVideo::~SeqUIVideo()
{
	SeqProject *project = Sequentia::GetProject();
	// stop listening for project changes
	project->RemoveActionHandler(this);
	// free memory
	delete[] name;
	// free material
	scene->player->RemoveViewer(0);
}

void SeqUIVideo::PreExecuteAction(const SeqActionType type, const SeqActionExecution execution, const void *data)
{
}

SeqWindowType SeqUIVideo::GetWindowType()
{
	return SeqWindowType::Video;
}

void SeqUIVideo::Draw()
{
	const ImGuiStyle style = ImGui::GetStyle();
	SeqProject *project = Sequentia::GetProject();
	SeqLibrary *library = Sequentia::GetLibrary();

	// draw the window
	bool isWindowNew = false;
	if (!ImGui::FindWindowByName(name))
		isWindowNew = true;
	bool isOpen = true;

	ImGui::SetNextWindowSizeConstraints(ImVec2(400, 200), ImVec2(FLT_MAX, FLT_MAX));
	if (ImGui::Begin(name, &isOpen, ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar))
	{
		// scale the video based on window size
		ImVec2 contentRegion = ImGui::GetContentRegionAvail();
		float lineHeight = ImGui::GetItemsLineHeightWithSpacing();
		contentRegion.x -= 2; // when horizontally centering an image this seems to make it just a bit more even
		contentRegion.y -= lineHeight * 2 + style.WindowPadding.y; // seek bar / play control buttons
					
		//float videoWidth = (float)frame->width;
		//float videoHeight = (float)frame->height; 
		float videoWidth = project->width;
		float videoHeight = project->height;
		float videoAspectRatio = videoWidth / videoHeight;
		float windowAspectRatio = contentRegion.x / contentRegion.y;
					
		// center the window in the available space in the window
		if (windowAspectRatio >= videoAspectRatio)
		{
			videoHeight = contentRegion.y;
			videoWidth = videoHeight * videoAspectRatio;
			ImGui::SetCursorPosX((contentRegion.x - videoWidth) / 2 + style.WindowPadding.x);
		}
		else
		{
			videoWidth = contentRegion.x;
			videoHeight = videoWidth / videoAspectRatio;
			ImGui::SetCursorPosY((contentRegion.y - videoHeight) / 2 + style.WindowPadding.y + ImGui::GetTextLineHeightWithSpacing());
		}
					
		// draw video
		ImTextureID texId = (void*)material;
		ImGui::Image(texId, ImVec2(videoWidth, videoHeight), ImVec2(0, 1), ImVec2(1, 0), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
					
		// mouse hover zoom window: mostly a curiousity which will later be removed, for now it's interesting reference code to keep around
		if (ImGui::IsItemHovered())
		{
			ImVec2 textureScreenPosition = ImGui::GetCursorScreenPos();
			textureScreenPosition.y -= videoHeight;
			ImGui::BeginTooltip();
			float focusSize = 64.0f;
			float focusX = ImGui::GetMousePos().x - textureScreenPosition.x - focusSize * 0.5f; if (focusX < 0.0f) focusX = 0.0f; else if (focusX > videoWidth - focusSize) focusX = videoWidth - focusSize;
			float focusY = (videoHeight - (ImGui::GetMousePos().y - textureScreenPosition.y)) - focusSize * 0.5f; if (focusY < 0.0f) focusY = 0.0f; else if (focusY > videoHeight - focusSize) focusY = videoHeight - focusSize;
			ImVec2 uv0 = ImVec2(focusX / videoWidth, (focusY + focusSize) / videoHeight);
			ImVec2 uv1 = ImVec2((focusX + focusSize) / videoWidth, focusY / videoHeight);
			ImGui::Image(texId, ImVec2(256, 256), uv0, uv1, ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
			ImGui::EndTooltip();
		}
					
		// anchor to the bottom of the window
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y - lineHeight * 2);
		// use the full width of the window for the scroll bar (seeker)
		ImGui::PushItemWidth(-1);

		// seeker scrollbar drawing and behavior
		SeqPlayer *player = scene->player;
		if (isSeeking && ImGui::IsMouseReleased(0))
		{
			printf("seek: %d\n", seekVideoTime);
			player->Seek(SEQ_TIME_FROM_MILLISECONDS(seekVideoTime));
			isSeeking = false;
		}
		seekVideoTime = SEQ_TIME_IN_MILLISECONDS(player->GetPlaybackTime());
		if (ImGui::SliderInt("##seek", &seekVideoTime, 0, SEQ_TIME_IN_MILLISECONDS(player->GetDuration()), ""))
		{
			isSeeking = true;
		}

		// render time
		SeqUtils::GetTimeString(SeqString::Temp->Buffer, SeqString::Temp->BufferLen, player->GetPlaybackTime());
		ImGui::Text(SeqString::Temp->Buffer);
		ImGui::SameLine();
		ImGui::Text("/");
		ImGui::SameLine();
		SeqUtils::GetTimeString(SeqString::Temp->Buffer, SeqString::Temp->BufferLen, player->GetDuration());
		ImGui::Text(SeqString::Temp->Buffer);

		// play control buttons
		if (player->IsPlaying())
		{
			ImGui::SameLine();
			if (ImGui::Button("Pause"))
			{
				player->Pause();
			}
		}
		else
		{
			ImGui::SameLine();
			if (ImGui::Button("Play"))
			{
				player->Play();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Stop"))
		{
			player->Stop();
		}
		if (!player->IsPlaying())
		{
			ImGui::SameLine();
			if (!player->IsSeeking())
			{
				if (ImGui::Button("Step"))
				{
					player->Seek(player->GetPlaybackTime() + SEQ_TIME_FROM_MILLISECONDS(500));
				}
			}
			else
			{
				ImGui::Text("Seeking...");
			}
		}
	}
	ImGui::SameLine();

	// scene select
	SeqScene *previousScene = scene;
	if (SeqWidgets::SceneSelectCombo(&scene))
	{
		previousScene->player->RemoveViewer(0);
		material = scene->player->AddViewer(0);
	}
	ImGui::End();

	if (!isOpen)
		Sequentia::GetProject()->RemoveWindow(this);
}

void SeqUIVideo::Serialize(SeqSerializer *serializer)
{
	serializer->Write(scene->id);
}

void SeqUIVideo::Deserialize(SeqSerializer *serializer)
{
	int sceneId = serializer->ReadInt();
	scene = Sequentia::GetProject()->GetSceneById(sceneId);
}
