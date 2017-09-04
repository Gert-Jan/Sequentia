#include "SeqDialogs.h";
#include "SeqProject.h";
#include "SeqString.h";
#include "SeqPath.h";
#include "SeqUtils.h";
#include <imgui.h>

bool SeqDialogs::showError = false;
bool SeqDialogs::showRequestProjectPath = false;
bool SeqDialogs::showWarningOverwrite = false;

char *SeqDialogs::projectPath = nullptr;
char *SeqDialogs::errorMessage = nullptr;
int SeqDialogs::errorNumber = 0;

void SeqDialogs::Draw(SeqProject *project)
{
	if (showError)
	{
		// draw error dialog
		ImGui::OpenPopup("Error##General");
		if (ImGui::BeginPopupModal("Error##General", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text(errorMessage, strerror(errorNumber));

			if (ImGui::Button("OK", ImVec2(120, 0)))
			{
				showError = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	if (showRequestProjectPath)
	{
		// draw Save As dialog
		ImGui::OpenPopup("Save As");
		if (ImGui::BeginPopupModal("Save As", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Paste the full path to the new project file.");
			ImGui::Separator();

			ImGui::Text("Filepath");
			ImGui::PushItemWidth(-1);
			ImGui::InputText("", SeqString::Buffer, SEQ_COUNT(SeqString::Buffer));
			ImGui::PopItemWidth();
			if (ImGui::Button("OK", ImVec2(120, 0)))
			{
				// TODO: validate input
				if (SeqPath::FileExists(projectPath))
				{
					showWarningOverwrite = true;
				}
				else
				{
					project->SetPath(SeqPath::Normalize(SeqString::CopyBuffer()));
					project->Save();
				}
				showRequestProjectPath = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				showRequestProjectPath = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	if (showWarningOverwrite)
	{
		// draw overwrite warning dialog
		ImGui::OpenPopup("Warning##Overwrite");
		if (ImGui::BeginPopupModal("Warning##Overwrite", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("You are about to overwrite the following project file:");
			ImGui::Text("%s", projectPath);

			if (ImGui::Button("OK", ImVec2(120, 0)))
			{
				project->SetPath(projectPath);
				project->Save();

				showWarningOverwrite = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				delete project;
				showWarningOverwrite = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}
}

void SeqDialogs::ShowRequestProjectPath(char *currentPath)
{
	SeqString::SetBuffer(currentPath, strlen(currentPath));
	projectPath = currentPath;
	showRequestProjectPath = true;
}

void SeqDialogs::ShowError(char *message, int error)
{
	errorMessage = message;
	errorNumber = error;
	showError = true;
}