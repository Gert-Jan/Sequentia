#include <imgui.h>
#include <imgui_internal.h>
#include "SeqProjectHeaders.h";
#include "SeqString.h";

int SeqUILibrary::nextWindowId = 0;

SeqUILibrary::SeqUILibrary(SeqProject *project) :
	project(project),
	windowId(nextWindowId)
{
	nextWindowId++;
	Init();
}

SeqUILibrary::SeqUILibrary(SeqProject *project, int windowId) :
	project(project),
	windowId(windowId)
{
	nextWindowId = ImMax(nextWindowId, windowId + 1);
	Init();
}

void SeqUILibrary::Init()
{
	// alloc memory
	name = SeqString::Format("Library##%d", windowId);
	// start listening for project changes
	project->AddActionHandler(this);
}

SeqUILibrary::~SeqUILibrary()
{
	// stop listening for project changes
	project->RemoveActionHandler(this);
	// free memory
	delete[] name;
}

void SeqUILibrary::ActionDone(const SeqAction action)
{
	switch (action.type)
	{
	}
}

void SeqUILibrary::ActionUndone(const SeqAction action)
{
	switch (action.type)
	{
	}
}

void SeqUILibrary::Draw()
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
		project->RemoveLibrary(this);
}
