#include <imgui.h>
#include <imgui_internal.h>
#include "SeqProjectHeaders.h";
#include "SeqSerializer.h";
#include "SeqString.h";
#include "SeqList.h";

int SeqUISequencer::nextWindowId = 0;

SeqUISequencer::SeqUISequencer(SeqProject *project):
	project(project),
	windowId(nextWindowId)
{
	nextWindowId++;
	Init();
}

SeqUISequencer::SeqUISequencer(SeqProject *project, SeqSerializer *serializer):
	project(project),
	windowId(nextWindowId)
{
	nextWindowId++;
	Init();
	Deserialize(serializer);
}

SeqUISequencer::SeqUISequencer(SeqProject *project, int windowId):
	project(project),
	windowId(windowId)
{
	nextWindowId = ImMax(nextWindowId, windowId + 1);
	Init();
}

void SeqUISequencer::Init()
{
	// alloc memory
	name = SeqString::Format("Sequencer##%d", windowId);
	channelHeights = new SeqList<int>();
	for (int i = 0; i < project->GetChannelCount(); i++)
		channelHeights->Add(initialChannelHeight);
	// start listening for project changes
	project->AddActionHandler(this);
}

SeqUISequencer::~SeqUISequencer()
{
	// stop listening for project changes
	project->RemoveActionHandler(this);
	// free memory
	delete[] name;
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
	bool isWindowNew = false;
	if (!ImGui::FindWindowByName(name))
		isWindowNew = true;
	bool isOpen = true;
	
	if (ImGui::Begin(name, &isOpen, ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar))
	{
		const static ImVec2 textSize = ImGui::CalcTextSize("01234567889:.");
		const float headerHeight = textSize.y + 3;
		ImGui::Columns(2, "panel");
		DrawChannelSettings(headerHeight, isWindowNew);
		ImGui::NextColumn();
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() - 4, ImGui::GetCursorPosY() + 1));
		DrawSequencerRuler(headerHeight);
		DrawChannels();
		ImGui::Columns(1);
	}
	ImGui::End();
	if (!isOpen)
		project->RemoveSequencer(this);
}

void SeqUISequencer::DrawChannelSettings(float rulerHeight, bool isWindowNew)
{
	const ImGuiStyle style = ImGui::GetStyle();
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	const ImVec2 size = ImVec2(settingsPanelWidth, ImGui::GetContentRegionAvail().y);
	
	// draw channel settings
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImU32 color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_::ImGuiCol_TextDisabled]);
	const ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_::ImGuiCol_ComboBg]);
	
	// default panel width
	if (isWindowNew)
		ImGui::SetColumnOffset(1, settingsPanelWidth + 7);

	// debug text
	ImGui::Text("%s", name);

	ImVec2 cursor = origin;
	cursor.y += rulerHeight + 1;
	ImGui::PushClipRect(cursor, ImVec2(cursor.x + size.x, cursor.y + size.y - rulerHeight - style.ScrollbarSize), false);
	// draw settings panels
	for (int i = 0; i < project->GetChannelCount(); i++)
	{
		float channelHeight = channelHeights->Get(i);
		// only draw if visible
		if (cursor.y - origin.y + channelHeight >= scrollY &&
			cursor.y - origin.y < scrollY + size.y)
		{
			drawList->AddRectFilled(ImVec2(cursor.x, cursor.y - scrollY), ImVec2(cursor.x + size.x, cursor.y - scrollY + channelHeight), bgColor, rounding, 0x1 | 0x8);
			drawList->AddRect(ImVec2(cursor.x, cursor.y - scrollY), ImVec2(cursor.x + size.x, cursor.y - scrollY + channelHeight), color, rounding, 0x1 | 0x8, lineThickness);
			ImGui::SetCursorScreenPos(ImVec2(cursor.x + 4, cursor.y - scrollY + 4));
			ImGui::Text("%s %d", project->GetChannel(i).name, i);
		}
		cursor.y += channelHeight + channelVerticalSpacing;
	}
	// draw and handle row resizing
	// code based on ImGui::Column()
	cursor = origin;
	cursor.y += rulerHeight;
	ImGuiContext* g = ImGui::GetCurrentContext();
	for (int i = 0; i < project->GetChannelCount(); i++)
	{
		float channelHeight = channelHeights->Get(i);
		cursor.y += channelHeight;
		// if visible
		if (cursor.y - origin.y >= scrollY && 
			cursor.y - origin.y < scrollY + size.y)
		{
			const ImGuiID row_id = ImGui::GetID("row") + ImGuiID(i);
			ImGui::KeepAliveID(row_id);
			const ImRect row_rect(ImVec2(cursor.x, cursor.y - scrollY - 4), ImVec2(cursor.x + size.x, cursor.y - scrollY + 4));

			bool hovered, held;
			ImGui::ButtonBehavior(row_rect, row_id, &hovered, &held);
			if (hovered || held)
				g->MouseCursor = ImGuiMouseCursor_ResizeNS;

			// Draw before resize so our items positioning are in sync with the line being drawn
			const ImU32 col = ImGui::GetColorU32(held ? ImGuiCol_ColumnActive : hovered ? ImGuiCol_ColumnHovered : ImGuiCol_Column);
			drawList->AddLine(ImVec2(cursor.x + rounding, cursor.y - scrollY), ImVec2(cursor.x + size.x, cursor.y - scrollY), col);

			if (held)
			{
				if (g->ActiveIdIsJustActivated)
					g->ActiveIdClickOffset.y -= 4;   // Store from center of row line (we used a 8 high rect for row clicking)
				
				ImGuiWindow* window = ImGui::GetCurrentWindowRead();
				float newHeight = ImClamp(g->IO.MousePos.y - (cursor.y - channelHeight - scrollY) - g->ActiveIdClickOffset.y, minChannelHeight, maxChannelHeight);
				channelHeights->Set(i, newHeight);
			}
		}
		cursor.y += channelVerticalSpacing;
	}
	ImGui::PopClipRect();
	// clamp width
	settingsPanelWidth = ImGui::GetColumnOffset(1) - 7;
	if (settingsPanelWidth < minSettingsPanelWidth)
	{
		settingsPanelWidth = minSettingsPanelWidth;
		ImGui::SetColumnOffset(1, settingsPanelWidth);
	}
	if (settingsPanelWidth > maxSettingsPanelWidth)
	{
		settingsPanelWidth = maxSettingsPanelWidth;
		ImGui::SetColumnOffset(1, settingsPanelWidth);
	}
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

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	// underline
	drawList->AddLine(ImVec2(origin.x, origin.y + height - 2), ImVec2(origin.x + width, origin.y + height - 2), color, thickness);

	// seconds lines
	int seconds = ceil(position);
	float firstOffset = -(position - floor(position)) * pixelsPerSecond / zoom;
	float scaledWidth = (origin.x + width) / zoom;
	float scaledStep = pixelsPerSecond / zoom;
	for (float x = origin.x + firstOffset; x < origin.x + scaledWidth; x += scaledStep)
	{
		drawList->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + height - 1), color, thickness);
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
	const ImVec2 contentSize = ImVec2(project->GetLength() * pixelsPerSecond / zoom, TotalChannelHeight() + 100);
	const ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x - style.ScrollbarSize, ImGui::GetContentRegionAvail().y - style.ScrollbarSize);
	ImGui::SetNextWindowContentSize(contentSize);
	ImGui::BeginChild("channels", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
	{
		position = ImGui::GetScrollX() / pixelsPerSecond * zoom;
		scrollY = ImGui::GetScrollY();

		const ImVec2 origin = ImGui::GetCursorScreenPos();

		ImDrawList* drawList = ImGui::GetWindowDrawList();

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
				drawList->AddRectFilled(ImVec2(cursor.x + ImGui::GetScrollX(), cursor.y), ImVec2(cursor.x + contentSize.x, cursor.y + channelHeight), bgColor, rounding, 0x2 | 0x4);
				drawList->AddRect(ImVec2(cursor.x, cursor.y), ImVec2(cursor.x + contentSize.x, cursor.y + channelHeight), color, rounding, 0x2 | 0x4, lineThickness);
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

void SeqUISequencer::Serialize(SeqSerializer *serializer)
{
	serializer->Write(position);
	serializer->Write(zoom);
	serializer->Write(scrollY);
	serializer->Write(settingsPanelWidth);
	serializer->Write(channelHeights->Count());
	for (int i = 0; i < channelHeights->Count(); i++)
		serializer->Write(channelHeights->Get(i));
}

void SeqUISequencer::Deserialize(SeqSerializer *serializer)
{
	position = serializer->ReadDouble();
	zoom = serializer->ReadFloat();
	scrollY = serializer->ReadFloat();
	settingsPanelWidth = serializer->ReadFloat();
	int channelSettingsCount = serializer->ReadInt();
	for (int i = 0; i < channelSettingsCount; i++)
		channelHeights->ReplaceAt(serializer->ReadInt(), i);
}