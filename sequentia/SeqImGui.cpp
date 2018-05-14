#include "SeqImGui.h"

void SeqImGui::BeginRender(ImVec2 size)
{
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiSetCond_Always);
	ImGui::SetNextWindowSize(size, ImGuiSetCond_Always);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	// this is kind of ugly, but with alpha == 0 the window gets deactivated automatically in ImGui::Begin...
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.00001);
	ImGui::Begin("PlayerRenderer", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs);
	ImGui::PushClipRect(ImVec2(0, 0), size, false);
}

void SeqImGui::EndRender()
{
	ImGui::PopClipRect();
	ImGui::End();
	ImGui::PopStyleVar(3);
}