#include <imgui.h>

#include "SeqDialogs.h"
#include "SeqProjectHeaders.h"
#include "SeqAction.h"
#include "SeqActionFactory.h"
#include "SeqString.h"
#include "SeqPath.h"

bool SeqDialogs::showError = false;
bool SeqDialogs::showRequestProjectPath = false;
bool SeqDialogs::showWarningOverwrite = false;

SeqDialogOption SeqDialogs::result = SeqDialogOption::OK;
RequestPathAction SeqDialogs::requestPathAction = RequestPathAction::Open;
SeqString *SeqDialogs::path = new SeqString(256);
SeqString *SeqDialogs::message = new SeqString(1024);

void SeqDialogs::ShowError(char *errorMessage, ...)
{
	va_list args;
	va_start(args, errorMessage);
	message->Format(errorMessage, args);
	va_end(args);
	showError = true;
}

void SeqDialogs::ShowRequestProjectPath(char *currentPath, RequestPathAction action)
{
	path->Set(currentPath);
	requestPathAction = action;
	showRequestProjectPath = true;
}

void SeqDialogs::Draw(SeqProject *project)
{
	if (showError)
	{
		if (ShowMessage("Error##General", SeqDialogOption::OK))
		{
			showError = false;
		}
	}

	if (showRequestProjectPath)
	{
		switch (requestPathAction)
		{
		case RequestPathAction::Save:
			if (ShowFileBrowseDialog("Save As"))
			{
				if (result == SeqDialogOption::OK)
				{
					if (SeqPath::FileExists(path->Buffer))
					{
						message->Format("You are about to overwrite %s\nAre you sure?", path);
						showWarningOverwrite = true;
					}
					else
					{
						project->SetPath(path->Copy());
						project->Save();
					}
				}
				showRequestProjectPath = false;
			}
			break;
		case RequestPathAction::Open:
			if (ShowFileBrowseDialog("Open"))
			{
				if (result == SeqDialogOption::OK)
				{
					if (SeqPath::FileExists(path->Buffer))
					{
						project->SetPath(path->Copy());
						project->Open();
					}
					else
					{
						ShowError("File not found.");
					}
				}
				showRequestProjectPath = false;
			}
			break;
		case RequestPathAction::AddToLibrary:
			if (ShowFileBrowseDialog("Add to library"))
			{
				if (result == SeqDialogOption::OK)
				{
					project->AddAction(SeqActionFactory::AddLibraryLink(path->Copy()));
				}
				showRequestProjectPath = false;
			}
			break;
		default:
			showRequestProjectPath = false;
			break;
		}
	}
	
	if (showWarningOverwrite)
	{
		if (ShowMessage("Warning##Overwrite", SeqDialogOption::Yes | SeqDialogOption::No))
		{
			if (result == SeqDialogOption::Yes)
			{
				project->SetPath(path->Copy());
				project->Save();
				showWarningOverwrite = false;
			}
			else if (result == SeqDialogOption::No)
			{
				showWarningOverwrite = false;
			}
		}
	}
}

bool SeqDialogs::ShowFileBrowseDialog(const char *title)
{
	// draw browse dialog
	bool isDone = false;
	ImGui::OpenPopup(title);
	if (ImGui::BeginPopupModal(title, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Paste the full path to the project file.");
		ImGui::Separator();

		ImGui::Text("Filepath");
		ImGui::PushItemWidth(-1);
		ImGui::InputText("", path->Buffer, path->BufferLen);
		ImGui::PopItemWidth();
		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			path->Set(SeqPath::Normalize(path->Buffer));
			result = SeqDialogOption::OK;
			isDone = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			result = SeqDialogOption::Cancel;
			isDone = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	return isDone;
}

bool SeqDialogs::ShowMessage(const char *title, int options)
{
	// draw message dialog
	bool isDone = false;
	ImGui::OpenPopup(title);
	if (ImGui::BeginPopupModal(title, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text(message->Buffer);
		isDone = isDone || ShowMessageButton("OK", options, SeqDialogOption::OK);
		isDone = isDone || ShowMessageButton("Cancel", options, SeqDialogOption::Cancel);
		isDone = isDone || ShowMessageButton("Yes", options, SeqDialogOption::Yes);
		isDone = isDone || ShowMessageButton("No", options, SeqDialogOption::No);
		ImGui::EndPopup();
	}
	return isDone;
}

bool SeqDialogs::ShowMessageButton(const char *label, int options, SeqDialogOption option)
{
	bool isClicked = false;
	if (options & option)
	{
		if (ImGui::Button(label, ImVec2(120, 0)))
		{
			isClicked = true;
			result = option;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
	}
	return isClicked;
}
