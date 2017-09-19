#include <imgui.h>
#include <imgui_internal.h>
#include "SeqProjectHeaders.h";
#include "SeqDialogs.h";
#include "SeqString.h";

int SeqUILibrary::nextWindowId = 0;

SeqUILibrary::SeqUILibrary(SeqProject *project, SeqLibrary *library) :
	project(project),
	library(library),
	windowId(nextWindowId)
{
	nextWindowId++;
	Init();
}

SeqUILibrary::SeqUILibrary(SeqProject *project, SeqLibrary *library, SeqSerializer *serializer) :
	project(project),
	library(library),
	windowId(nextWindowId)
{
	nextWindowId++;
	Init();
}

SeqUILibrary::SeqUILibrary(SeqProject *project, SeqLibrary *library, int windowId) :
	project(project),
	library(library),
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
		for (int i = 0; i < library->LinkCount(); i++)
		{
			bool opened = true;
			if (ImGui::CollapsingHeader(library->GetLink(i).fullPath, &opened))
			{
			}
			if (!opened)
			{
				project->AddAction(SeqActionFactory::CreateRemoveLibraryLinkAction(library->GetLink(i).fullPath));
			}
		}
		if (ImGui::Button("Add file/folder", ImVec2(120, 0)))
		{
			SeqDialogs::ShowRequestProjectPath(project->GetPath(), RequestPathAction::AddToLibrary);
		}
	}
	ImGui::End();
	if (!isOpen)
		project->RemoveLibrary(this);
}

void SeqUILibrary::Serialize(SeqSerializer *serializer)
{

}

void SeqUILibrary::Deserialize(SeqSerializer *serializer)
{

}
