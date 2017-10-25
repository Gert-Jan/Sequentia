#include <imgui.h>
#include <imgui_internal.h>
#include "SeqUILibrary.h";
#include "SeqProjectHeaders.h";
#include "SeqVideoInfo.h";
#include "SeqDialogs.h";
#include "SeqString.h";

SeqUILibrary::SeqUILibrary(SeqProject *project, SeqLibrary *library) :
	project(project),
	library(library)
{
	Init();
}

SeqUILibrary::SeqUILibrary(SeqProject *project, SeqLibrary *library, SeqSerializer *serializer) :
	project(project),
	library(library)
{
	Init();
}

void SeqUILibrary::Init()
{
	// alloc memory
	name = SeqString::Format("Library##%d", project->NextWindowId());
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

SeqWindowType SeqUILibrary::GetWindowType()
{
	return SeqWindowType::Library;
}

void SeqUILibrary::Draw()
{
	bool isWindowNew = false;
	if (!ImGui::FindWindowByName(name))
		isWindowNew = true;
	bool isOpen = true;

	if (ImGui::Begin(name, &isOpen, ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar))
	{
		ImGui::Columns(3, "libraryTable");
		ImGui::Separator();
		ImGui::Text("Name"); ImGui::NextColumn();
		ImGui::Text("Resolution"); ImGui::NextColumn();
		ImGui::Text("Duration"); ImGui::NextColumn();
		ImGui::Separator();

		int selected = -1;
		for (int i = 0; i < library->LinkCount(); i++)
		{
			SeqLibraryLink *link = library->GetLink(i);
			if (ImGui::Selectable(link->fullPath, selected == i, ImGuiSelectableFlags_SpanAllColumns))
				selected = i;
			AddContextMenu(link);
			ImGui::NextColumn();
			if (link->info != nullptr)
			{
				SeqVideoInfo *videoInfo = link->info;
				SeqString::FormatBuffer("%ix%i", videoInfo->width, videoInfo->height);
				ImGui::Text(SeqString::Buffer);
				ImGui::NextColumn();
				SeqVideoInfo::GetTimeString(SeqString::Buffer, SeqString::BufferLen, videoInfo->formatContext->duration);
				ImGui::Text(SeqString::Buffer);
				ImGui::NextColumn();
			}
			else
			{
				ImGui::Text("N/A");
				ImGui::NextColumn();
				ImGui::Text("N/A");
				ImGui::NextColumn();
			}
			bool opened = true;
			if (!opened)
			{
				project->AddAction(SeqActionFactory::CreateRemoveLibraryLinkAction(library->GetLink(i)->fullPath));
			}
		}
		ImGui::Columns(1);
		if (ImGui::Button("Add file/folder", ImVec2(120, 0)))
		{
			SeqDialogs::ShowRequestProjectPath(project->GetPath(), RequestPathAction::AddToLibrary);
		}
	}
	ImGui::End();
	if (!isOpen)
		project->RemoveWindow(this);
}

void SeqUILibrary::AddContextMenu(SeqLibraryLink *link)
{
	if (ImGui::BeginPopupContextItem(link->fullPath))
	{
		if (ImGui::Selectable("Delete"))
			project->AddAction(SeqActionFactory::CreateRemoveLibraryLinkAction(link->fullPath));
		ImGui::EndPopup();
	}
}

void SeqUILibrary::Serialize(SeqSerializer *serializer)
{

}

void SeqUILibrary::Deserialize(SeqSerializer *serializer)
{

}
