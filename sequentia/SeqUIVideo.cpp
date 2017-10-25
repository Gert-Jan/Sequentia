#include <imgui.h>
#include <imgui_internal.h>
#include "SeqUIVideo.h";
#include "SeqProjectHeaders.h";
#include "SeqString.h";

SeqUIVideo::SeqUIVideo(SeqProject *project):
	project(project)
{
	Init();
}

SeqUIVideo::SeqUIVideo(SeqProject *project, SeqSerializer *serializer):
	project(project)
{
	Init();
}

void SeqUIVideo::Init()
{
	// alloc memory
	name = SeqString::Format("Video##%d", project->NextWindowId());
	// start listening for project changes
	project->AddActionHandler(this);
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
	bool isWindowNew = false;
	if (!ImGui::FindWindowByName(name))
		isWindowNew = true;
	bool isOpen = true;

	if (ImGui::Begin(name, &isOpen, ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar))
	{
		
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
