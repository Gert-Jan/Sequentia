#include <imgui.h>

#include "SeqDialogs.h";
#include "SeqProjectHeaders.h";
#include "SeqString.h";
#include "SeqPath.h";
#include "SeqUtils.h";

bool SeqDialogs::showError = false;
bool SeqDialogs::showRequestProjectPath = false;
bool SeqDialogs::showWarningOverwrite = false;

SeqDialogOption SeqDialogs::result = OK;
RequestPathAction SeqDialogs::requestPathAction = Open;
char *SeqDialogs::path = nullptr;
char *SeqDialogs::message = nullptr;

void SeqDialogs::ShowError(char *errorMessage, ...)
{
	va_list args;
	va_start(args, errorMessage);
	message = SeqString::Format(errorMessage, args);
	va_end(args);
	showError = true;
}

void SeqDialogs::ShowRequestProjectPath(char *currentPath, RequestPathAction action)
{
	SeqString::SetBuffer(currentPath, strlen(currentPath));
	path = currentPath;
	requestPathAction = action;
	showRequestProjectPath = true;
}

void SeqDialogs::Draw(SeqProject *project)
{
	if (showError)
	{
		if (ShowMessage("Error##General", OK))
		{
			showError = false;
		}
	}

	if (showRequestProjectPath)
	{
		switch (requestPathAction)
		{
		case Save:
			if (ShowFileBrowseDialog("Save As"))
			{
				if (result == OK)
				{
					if (SeqPath::FileExists(path))
					{
						message = SeqString::Format("You are about to overwrite %s\nAre you sure?", path);
						showWarningOverwrite = true;
					}
					else
					{
						project->SetPath(path);
						project->Save();
					}
				}
				showRequestProjectPath = false;
			}
			break;
		case Open:
			if (ShowFileBrowseDialog("Open"))
			{
				if (result == OK)
				{
					if (SeqPath::FileExists(path))
					{
						project->SetPath(path);
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
		case AddToLibrary:
			if (ShowFileBrowseDialog("Add to library"))
			{
				if (result == OK)
				{
					project->AddAction(SeqActionFactory::CreateAddLibraryLinkAction(path));
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
		if (ShowMessage("Warning##Overwrite", Yes | No))
		{
			if (result == Yes)
			{
				project->SetPath(path);
				project->Save();
				showWarningOverwrite = false;
			}
			else if (result = No)
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
		ImGui::InputText("", SeqString::Buffer, SEQ_COUNT(SeqString::Buffer));
		ImGui::PopItemWidth();
		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			path = SeqPath::Normalize(SeqString::CopyBuffer());
			result = OK;
			isDone = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			result = Cancel;
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
		ImGui::Text(message);
		isDone = isDone || ShowMessageButton("OK", options, OK);
		isDone = isDone || ShowMessageButton("Cancel", options, Cancel);
		isDone = isDone || ShowMessageButton("Yes", options, Yes);
		isDone = isDone || ShowMessageButton("No", options, No);
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