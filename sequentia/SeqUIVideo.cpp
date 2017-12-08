#include <imgui.h>
#include <imgui_internal.h>
#include <SDL.h>
#include "SeqUIVideo.h"
#include "SeqProjectHeaders.h"
#include "SeqWorkerManager.h"
#include "SeqTaskDecodeVideo.h"
#include "SeqVideoInfo.h"
#include "SeqDecoder.h"
#include "SeqString.h"
#include "SeqRenderer.h"
#include "SeqMaterial.h"

extern "C"
{
	#include "libavformat/avformat.h"
}

SeqUIVideo::SeqUIVideo(SeqProject *project, SeqLibrary *library) :
	project(project),
	library(library),
	decoderTask(nullptr),
	previousFrame(nullptr),
	isSeeking(false),
	seekVideoTime(0),
	startVideoTime(0)
{
	Init();
}

SeqUIVideo::SeqUIVideo(SeqProject *project, SeqLibrary *library, SeqSerializer *serializer) :
	project(project),
	library(library),
	decoderTask(nullptr),
	previousFrame(nullptr),
	isSeeking(false),
	seekVideoTime(0),
	startVideoTime(0)
{
	Init();
}
void SeqUIVideo::Init()
{
	// alloc memory
	name = SeqString::Format("Video##%d", project->NextWindowId());
	// start listening for project changes
	project->AddActionHandler(this);
	// prepare material
	material = SeqRenderer::GetVideoMaterial();
}

SeqUIVideo::~SeqUIVideo()
{
	// stop listening for project changes
	project->RemoveActionHandler(this);
	// free memory
	delete[] name;
}

void SeqUIVideo::ActionDone(const SeqAction action)
{
	switch (action.type)
	{
	}
}

void SeqUIVideo::ActionUndone(const SeqAction action)
{
	switch (action.type)
	{
	}
}

SeqWindowType SeqUIVideo::GetWindowType()
{
	return SeqWindowType::Video;
}

void SeqUIVideo::Draw()
{
	const ImGuiStyle style = ImGui::GetStyle();

	// detect if another video was focussed
	if ((decoderTask == nullptr && library->GetLastLinkFocus() != nullptr) ||
		(decoderTask != nullptr && decoderTask->GetLink() != library->GetLastLinkFocus()))
	{
		if (decoderTask != nullptr)
		{
			decoderTask->Stop();
			decoderTask = nullptr;
		}
		if (library->GetLastLinkFocus() != nullptr)
		{
			decoderTask = new SeqTaskDecodeVideo(library->GetLastLinkFocus());
			SeqWorkerManager::Instance()->PerformTask(decoderTask);
			startVideoTime = SDL_GetTicks();
			previousFrame = nullptr;
		}
	}

	// draw the window
	bool isWindowNew = false;
	if (!ImGui::FindWindowByName(name))
		isWindowNew = true;
	bool isOpen = true;

	ImGui::SetNextWindowSizeConstraints(ImVec2(400, 200), ImVec2(FLT_MAX, FLT_MAX));
	if (ImGui::Begin(name, &isOpen, ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar))
	{
		if (decoderTask != nullptr)
		{
			SeqDecoder* decoder = decoderTask->GetDecoder();
			if (decoder->GetStatus() == SeqDecoderStatus::Ready || decoder->GetStatus() == SeqDecoderStatus::Loading)
			{
				Uint32 videoTime = 0;
				if (startVideoTime != 0)
					videoTime = SDL_GetTicks() - startVideoTime;

				AVFrame* frame = decoderTask->GetDecoder()->NextFrame(videoTime);
				if (frame != nullptr && frame->width > 0 && frame->height > 0)
				{
					if (startVideoTime == 0)
						startVideoTime = SDL_GetTicks();

					if (previousFrame == nullptr || previousFrame->width != frame->width || previousFrame->height != frame->height)
						SeqRenderer::CreateVideoTextures(frame, material->textureHandles);
					else
						SeqRenderer::OverwriteVideoTextures(frame, material->textureHandles);

					// scale the video based on window size
					ImVec2 contentRegion = ImGui::GetContentRegionAvail();
					float lineHeight = ImGui::GetItemsLineHeightWithSpacing();
					contentRegion.x -= 2; // when horizontally centering an image this seems to make it just a bit more even
					contentRegion.y -= lineHeight * 2 + style.WindowPadding.y; // seek bar / play control buttons

					float videoWidth = (float)frame->width;
					float videoHeight = (float)frame->height; 
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
					ImGui::Image(texId, ImVec2(videoWidth, videoHeight), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
					
					// mouse hover zoom window: mostly a curiousity which will later be removed, for now it's interesting reference code to keep around
					if (ImGui::IsItemHovered())
					{
						ImVec2 textureScreenPosition = ImGui::GetCursorScreenPos();
						textureScreenPosition.y -= videoHeight;
						ImGui::BeginTooltip();
						float focusSize = 64.0f;
						float focusX = ImGui::GetMousePos().x - textureScreenPosition.x - focusSize * 0.5f; if (focusX < 0.0f) focusX = 0.0f; else if (focusX > videoWidth - focusSize) focusX = videoWidth - focusSize;
						float focusY = ImGui::GetMousePos().y - textureScreenPosition.y - focusSize * 0.5f; if (focusY < 0.0f) focusY = 0.0f; else if (focusY > videoHeight - focusSize) focusY = videoHeight - focusSize;
						ImVec2 uv0 = ImVec2((focusX) /videoWidth, (focusY) / videoHeight);
						ImVec2 uv1 = ImVec2((focusX + focusSize) / videoWidth, (focusY + focusSize) / videoHeight);
						ImGui::Image(texId, ImVec2(256, 256), uv0, uv1, ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
						ImGui::EndTooltip();
					}
					
					// anchor to the bottom of the window
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y - lineHeight * 2);
					// use the full width of the window for the scroll bar (seeker)
					ImGui::PushItemWidth(-1);

					// seeker scrollbar drawing and behavior
					if (isSeeking && ImGui::IsMouseReleased(0))
					{
						printf("seek: %d\n", seekVideoTime);
						decoder->Seek(seekVideoTime);
						startVideoTime += videoTime - seekVideoTime;
						videoTime = seekVideoTime;
						isSeeking = false;
					}
					seekVideoTime = videoTime;
					if (ImGui::SliderInt("##seek", &seekVideoTime, 0, decoderTask->GetLink()->duration / 1000, ""))
					{
						isSeeking = true;
					}

					// render time
					SeqVideoInfo::GetTimeString(SeqString::Buffer, SeqString::BufferLen, videoTime);
					ImGui::Text(SeqString::Buffer);
					ImGui::SameLine();
					ImGui::Text("/");
					ImGui::SameLine();
					SeqVideoInfo::GetTimeString(SeqString::Buffer, SeqString::BufferLen, decoderTask->GetLink()->duration);
					ImGui::Text(SeqString::Buffer);

					// some buttons that will likely later be removed
					ImGui::SameLine();
					if (ImGui::Button("Skip 1s"))
					{
						seekVideoTime = videoTime + 1000;
						printf("skip 1s: %d\n", seekVideoTime);
						decoder->Seek(seekVideoTime);
						startVideoTime += videoTime - seekVideoTime;
						videoTime = seekVideoTime;
					}
					ImGui::SameLine();
					if (ImGui::Button("Skip 10s"))
					{
						seekVideoTime = videoTime + 10000;
						printf("skip 10s: %d\n", seekVideoTime);
						decoder->Seek(seekVideoTime);
						startVideoTime += videoTime - seekVideoTime;
						videoTime = seekVideoTime;
					}
					ImGui::SameLine();
					if (ImGui::Button("Skip 60s"))
					{
						seekVideoTime = videoTime + 60000;
						printf("skip 60s: %d\n", seekVideoTime);
						decoder->Seek(seekVideoTime);
						startVideoTime += videoTime - seekVideoTime;
						videoTime = seekVideoTime;
					}
					previousFrame = frame;
				}
			}
		}
	}
	ImGui::End();

	if (!isOpen)
		project->RemoveWindow(this);
}

void SeqUIVideo::Serialize(SeqSerializer *serializer)
{

}

void SeqUIVideo::Deserialize(SeqSerializer *serializer)
{

}
