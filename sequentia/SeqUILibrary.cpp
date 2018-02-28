#include <imgui.h>
#include <imgui_internal.h>
#include "Sequentia.h"
#include "SeqUILibrary.h"
#include "SeqProjectHeaders.h"
#include "SeqActionFactory.h"
#include "SeqVideoContext.h"
#include "SeqDialogs.h"
#include "SeqString.h"

SeqUILibrary::SeqUILibrary()
{
	Init();
}

SeqUILibrary::SeqUILibrary(SeqSerializer *serializer)
{
	Init();
	Deserialize(serializer);
}

void SeqUILibrary::Init()
{
	SeqProject *project = Sequentia::GetProject();
	// alloc memory
	SeqString::Temp->Format("Library##%d", project->NextWindowId());
	name = SeqString::Temp->Copy();
	// start listening for project changes
	project->AddActionHandler(this);
}

SeqUILibrary::~SeqUILibrary()
{
	SeqProject *project = Sequentia::GetProject();
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
	SeqProject *project = Sequentia::GetProject();
	SeqLibrary *library = Sequentia::GetLibrary();

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

		for (int i = 0; i < library->LinkCount(); i++)
		{
			SeqLibraryLink *link = library->GetLink(i);
			if (ImGui::Selectable(link->fullPath, library->GetLastLinkFocus() == link, ImGuiSelectableFlags_SpanAllColumns))
			{
				library->SetLastLinkFocus(link);
				Sequentia::SetPreviewLibraryLink(link);
			}
			if (ImGui::IsItemActive()) // is dragging
			{
				if (!Sequentia::IsDragging())
				{
					Sequentia::SetDragClipNew(link);
				}
			}
			AddContextMenu(link);
			ImGui::NextColumn();
			if (link->metaDataLoaded)
			{
				if (link->defaultVideoStream != nullptr)
				{
					SeqVideoStreamInfo *videoInfo = link->defaultVideoStream;
					SeqString::Temp->Format("%ix%i", videoInfo->width, videoInfo->height);
				}
				else
				{
					SeqString::Temp->Set("N/A");
				}
				ImGui::Text(SeqString::Temp->Buffer);
				ImGui::NextColumn();
				SeqVideoContext::GetTimeString(SeqString::Temp->Buffer, SeqString::Temp->BufferLen, link->duration);
				ImGui::Text(SeqString::Temp->Buffer);
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
				project->AddAction(SeqActionFactory::RemoveLibraryLink(library->GetLink(i)->fullPath));
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
			Sequentia::GetProject()->AddAction(SeqActionFactory::RemoveLibraryLink(link->fullPath));
		ImGui::EndPopup();
	}
}

void SeqUILibrary::Serialize(SeqSerializer *serializer)
{

}

void SeqUILibrary::Deserialize(SeqSerializer *serializer)
{

}
