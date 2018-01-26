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
	if (ImGui::Combo("##sceneSelect", &selectedScene, SeqString::Temp->Buffer))
	{
		*currentScene = project->GetScene(selectedScene);
		// currentScene changed
		return true;
	}
	// currentScene not changed
	return false;
}