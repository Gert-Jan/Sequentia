#pragma once

#include <imgui.h>
#include "SeqProject.h";
#include "SeqUISequencer.h";

SeqUISequencer::SeqUISequencer(SeqProject *project) :
	project(project)
{
	channelHeights = new SeqList<int>();
	for (int i = 0; i < project->GetChannelCount(); i++)
		channelHeights->Add(initialChannelHeight);
	project->AddActionHandler(this);
}

SeqUISequencer::~SeqUISequencer()
{
	delete channelHeights; 
}

void SeqUISequencer::ActionDone(const SeqAction action)
{
	switch (action.type)
	{
		case SeqActionType::AddChannel:
			channelHeights->Add(initialChannelHeight);
			break;
	}
}

void SeqUISequencer::ActionUndone(const SeqAction action)
{
	switch (action.type)
	{
		case SeqActionType::AddChannel:
			channelHeights->RemoveAt(channelHeights->Count() - 1);
			break;
	}
}

void SeqUISequencer::Draw()
{
	bool *isOpen = false;
	ImVec2 textSize = ImGui::CalcTextSize("01234567889:.");
	if (ImGui::Begin("Sequencer", isOpen, 0))
	{
		DrawSequencerRuler(textSize.y + 1);
		DrawChannels();
	}
	ImGui::End();
}

void SeqUISequencer::DrawSequencerRuler(float height)
{
	const ImGuiStyle style = ImGui::GetStyle();
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	const float width = ImGui::GetContentRegionAvailWidth() - style.ScrollbarSize;
	const float thickness = 1.0f;
	const ImU32 color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_::ImGuiCol_TextDisabled]);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	// underline
	draw_list->AddLine(ImVec2(origin.x, origin.y + height), ImVec2(origin.x + width, origin.y + height), color, thickness);

	// second lines
	int seconds = ceil(position);
	float firstOffset = -(position - floor(position)) * pixelsPerSecond / zoom;
	float scaledWidth = (origin.x + width) / zoom;
	float scaledStep = pixelsPerSecond / zoom;
	for (float x = origin.x + firstOffset; x < origin.x + scaledWidth; x += scaledStep)
	{
		draw_list->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + height), color, thickness);
		ImGui::SetCursorScreenPos(ImVec2(x + style.FramePadding.x, origin.y));
		ImGui::Text("%02d\:%02d\.%d", (seconds - (seconds % 60)), seconds % 60, 0);
		seconds++;
	}

	ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + height));
}

void SeqUISequencer::DrawChannels()
{
	ImGui::SetNextWindowContentSize(ImVec2(project->GetLength() * pixelsPerSecond / zoom, TotalChannelHeight()));
	ImGui::BeginChild("channels", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
	{
		position = ImGui::GetScrollX() / pixelsPerSecond * zoom;

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImGui::SetCursorPos(ImVec2(ImGui::GetScrollX() + 100, ImGui::GetScrollY() + 100));
		ImGui::Text("ScrollPos: %.3f, %.3f", ImGui::GetScrollX(), ImGui::GetScrollY());

		const ImVec2 p = ImGui::GetCursorScreenPos();
		const ImU32 color = ImColor(255, 0, 0);
		const float thickness = 1.0f;
		draw_list->AddLine(ImVec2(p.x, p.y), ImVec2(p.x + 200, p.y + 200), color, thickness);
	}
	ImGui::EndChild();
}

int SeqUISequencer::TotalChannelHeight()
{
	int totalHeight = 0;
	for (int i = 0; i < channelHeights->Count(); i++)
		totalHeight += channelHeights->Get(i);
	return totalHeight;
}
