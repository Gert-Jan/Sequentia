#include <imgui.h>
#include <imgui_internal.h>
#include "SeqUIVideo.h";
#include "SeqProjectHeaders.h";
#include "SeqWorkerManager.h";
#include "SeqTaskDecodeVideo.h";
#include "SeqDecoder.h";
#include "SeqString.h";
#include "SeqRenderer.h";
#include "SeqMaterial.h";

extern "C"
{
	#include "libavformat/avformat.h"
}

SeqUIVideo::SeqUIVideo(SeqProject *project, SeqLibrary *library):
	project(project),
	library(library),
	decoderTask(nullptr),
	previousFrame(nullptr)
{
	Init();
}

SeqUIVideo::SeqUIVideo(SeqProject *project, SeqLibrary *library, SeqSerializer *serializer) :
	project(project),
	library(library),
	decoderTask(nullptr),
	previousFrame(nullptr)
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
		}
	}

	// draw the window
	bool isWindowNew = false;
	if (!ImGui::FindWindowByName(name))
		isWindowNew = true;
	bool isOpen = true;

	if (ImGui::Begin(name, &isOpen, ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar))
	{
		if (decoderTask != nullptr)
		{
			SeqDecoder* decoder = decoderTask->GetDecoder();
			if (decoder->GetStatus() == SeqDecoderStatus::Ready || decoder->GetStatus() == SeqDecoderStatus::Loading)
			{
				AVFrame* frame = decoderTask->GetDecoder()->NextFrame();
				if (frame != nullptr && frame->width > 0 && frame->height > 0)
				{
					if (previousFrame == nullptr || previousFrame->width != frame->width || previousFrame->height != frame->height)
						SeqRenderer::CreateVideoTextures(frame, material->textureHandles);
					else
						SeqRenderer::OverwriteVideoTextures(frame, material->textureHandles);

					float tex_w = (float)frame->width;
					float tex_h = (float)frame->height;
					tex_w = 320;
					tex_h = 180;

					ImTextureID texId = (void*)material;
					ImGui::Text("%.0fx%.0f", tex_w, tex_h);
					ImGui::Image(texId, ImVec2(tex_w, tex_h), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
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
