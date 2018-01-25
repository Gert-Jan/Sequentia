#include "SeqWidgets.h"
#include "SeqString.h"
#include "Sequentia.h"
#include "SeqProjectHeaders.h"
#include <imgui.h>

bool SeqWidgets::SceneSelectCombo(SeqScene **currentScene)
{
	SeqProject *project = Sequentia::GetProject();
	SeqString::Temp->Clean();
	int selectedScene = 0;
	SeqScene *scene = nullptr;
	int textCursor = 0;
	for (int i = 0; i < project->SceneCount(); i++)
	{
		scene = project->GetScene(i);
		textCursor = SeqString::Temp->AppendAt(scene->name, textCursor);
		textCursor++;
		if (*currentScene == scene)
			selectedScene = i;
	}
	scene = Sequentia::GetPreviewScene();
	SeqString::Temp->AppendAt(scene->name, textCursor);
	if (*currentScene == scene)
		selectedScene = project->SceneCount();
	if (ImGui::Combo("##sceneSelect", &selectedScene, SeqString::Temp->Buffer))
	{
		if (selectedScene == project->SceneCount())
			*currentScene = Sequentia::GetPreviewScene();
		else
			*currentScene = project->GetScene(selectedScene);
		// currentScene changed
		return true;
	}
	// currentScene not changed
	return false;
}