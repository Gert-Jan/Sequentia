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
	if (ImGui::Begin("Sequencer", isOpen, ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar))
	{
		const static ImVec2 textSize = ImGui::CalcTextSize("01234567889:.");
		const float headerHeight = textSize.y + 3;
		DrawChannelSettings(headerHeight);
		DrawSequencerRuler(headerHeight);
		DrawChannels();
	}
	ImGui::End();
}

void SeqUISequencer::DrawChannelSettings(float rulerHeight)
{
	const ImGuiStyle style = ImGui::GetStyle();
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	const ImVec2 size = ImVec2(settingsPanelWidth, ImGui::GetContentRegionAvail().y);
	
	// draw channel settings
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	const ImU32 color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_::ImGuiCol_TextDisabled]);
	const ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_::ImGuiCol_ComboBg]);

	ImVec2 cursor = origin;
	cursor.y += rulerHeight;
	ImGui::PushClipRect(cursor, ImVec2(cursor.x + size.x, cursor.y + size.y - rulerHeight - style.ScrollbarSize), false);
	for (int i = 0; i < project->GetChannelCount(); i++)
	{
		float channelHeight = channelHeights->Get(i);
		// only draw if visible
		if (cursor.y - origin.y + channelHeight >= scrollY &&
			cursor.y - origin.y < scrollY + size.y)
		{
			draw_list->AddRectFilled(ImVec2(cursor.x, cursor.y - scrollY), ImVec2(cursor.x + size.x, cursor.y - scrollY + channelHeight), bgColor, rounding, 0x1 | 0x8);
			draw_list->AddRect(ImVec2(cursor.x, cursor.y - scrollY), ImVec2(cursor.x + size.x, cursor.y - scrollY + channelHeight), color, rounding, 0x1 | 0x8, lineThickness);
			ImGui::SetCursorScreenPos(ImVec2(cursor.x + 4, cursor.y - scrollY + 4));
			ImGui::Text("%s %d", project->GetChannel(i).name, i);
		}
		cursor.y += channelHeight + channelVerticalSpacing;
	}
	ImGui::PopClipRect();

	// setup the position for ruler drawing
	ImGui::SetCursorScreenPos(ImVec2(origin.x + settingsPanelWidth, origin.y));
}

void SeqUISequencer::DrawSequencerRuler(float height)
{
	const ImGuiStyle style = ImGui::GetStyle();

	const ImVec2 origin = ImVec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y);
	const float width = ImGui::GetContentRegionAvailWidth() - style.ScrollbarSize;
	const float thickness = 1.0f;
	const ImU32 color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_::ImGuiCol_TextDisabled]);

	// clip so we won't draw on top of the settings panel
	ImGui::PushClipRect(origin, ImVec2(origin.x + width, origin.y + height), false);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	// underline
	draw_list->AddLine(ImVec2(origin.x, origin.y + height - 2), ImVec2(origin.x + width, origin.y + height - 2), color, thickness);

	// seconds lines
	int seconds = ceil(position);
	float firstOffset = -(position - floor(position)) * pixelsPerSecond / zoom;
	float scaledWidth = (origin.x + width) / zoom;
	float scaledStep = pixelsPerSecond / zoom;
	for (float x = origin.x + firstOffset; x < origin.x + scaledWidth; x += scaledStep)
	{
		draw_list->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + height - 1), color, thickness);
		ImGui::SetCursorScreenPos(ImVec2(x + style.FramePadding.x, origin.y));
		ImGui::Text("%02d\:%02d\.%d", (seconds - (seconds % 60)), seconds % 60, 0);
		seconds++;
	}
	ImGui::PopClipRect();

	// setup the position for channel drawing
	ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + height));
}

void SeqUISequencer::DrawChannels()
{
	const ImGuiStyle style = ImGui::GetStyle();
	const ImVec2 contentSize = ImVec2(project->GetLength() * pixelsPerSecond / zoom, TotalChannelHeight());
	const ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x - style.ScrollbarSize, ImGui::GetContentRegionAvail().y - style.ScrollbarSize);
	ImGui::SetNextWindowContentSize(contentSize);
	ImGui::BeginChild("channels", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
	{
		position = ImGui::GetScrollX() / pixelsPerSecond * zoom;
		scrollY = ImGui::GetScrollY();

		const ImVec2 origin = ImGui::GetCursorScreenPos();

		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		const ImU32 color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_::ImGuiCol_TextDisabled]);
		const ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_::ImGuiCol_ComboBg]);

		ImVec2 cursor = origin;
		for (int i = 0; i < project->GetChannelCount(); i++)
		{
			float channelHeight = channelHeights->Get(i);
			// only draw if visible
			if (cursor.y - origin.y + channelHeight >= scrollY && 
				cursor.y - origin.y < scrollY + size.y)
			{
				draw_list->AddRectFilled(ImVec2(cursor.x + ImGui::GetScrollX(), cursor.y), ImVec2(cursor.x + contentSize.x, cursor.y + channelHeight), bgColor, rounding, 0x2 | 0x4);
				draw_list->AddRect(ImVec2(cursor.x, cursor.y), ImVec2(cursor.x + contentSize.x, cursor.y + channelHeight), color, rounding, 0x2 | 0x4, lineThickness);
				//ImGui::SetCursorScreenPos(ImVec2(cursor.x + ImGui::GetScrollX() + 4, cursor.y + 4));
				//ImGui::Text("%s %d", project->GetChannel(i).name, i);
			}
			cursor.y += channelHeight + channelVerticalSpacing;
		}

		static ImVec2 scrollPosTextSize = ImGui::CalcTextSize("ScrollPos: 00000.000, 00000.000");
		ImGui::SetCursorPos(ImVec2(ImGui::GetScrollX() + size.x - scrollPosTextSize.x, ImGui::GetScrollY() + size.y - scrollPosTextSize.y));
		ImGui::Text("ScrollPos: %.3f, %.3f", ImGui::GetScrollX(), ImGui::GetScrollY());
	}
	ImGui::EndChild();
}

int SeqUISequencer::TotalChannelHeight()
{
	int totalHeight = 0;
	for (int i = 0; i < channelHeights->Count(); i++)
		totalHeight += channelHeights->Get(i) + channelVerticalSpacing;
	// remove spacing on the bottom most channel
	totalHeight -= channelVerticalSpacing;
	return totalHeight;
}
