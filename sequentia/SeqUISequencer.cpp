#include <imgui.h>
#include <imgui_internal.h>
#include "SeqUISequencer.h"
#include "Sequentia.h"
#include "SeqProjectHeaders.h"
#include "SeqPlayer.h"
#include "SeqActionFactory.h"
#include "SeqSerializer.h"
#include "SeqString.h"
#include "SeqList.h"
#include "SeqTime.h"
#include "SeqWidgets.h"

ImU32 SeqUISequencer::clipBackgroundColor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_::ImGuiCol_MenuBarBg]);
ImU32 SeqUISequencer::lineColor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_::ImGuiCol_TextDisabled]);
ImU32 SeqUISequencer::backgroundColor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_::ImGuiCol_ComboBg]);
ImU32 SeqUISequencer::errorColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1, 0, 0, 1));

SeqUISequencer::SeqUISequencer(SeqScene *scene):
	scene(scene),
	sceneSettings(nullptr)
{
	Init();
}

SeqUISequencer::SeqUISequencer(SeqScene *scene, SeqSerializer *serializer):
	scene(scene),
	sceneSettings(nullptr)
{
	Init();
	Deserialize(serializer);
	sceneSettings = GetSceneUISettings(this->scene);
	scroll.x = TimeToPixels(position);
	overrideScrollX = true;
}

void SeqUISequencer::Init()
{
	SeqProject *project = Sequentia::GetProject();
	// name
	SeqString::Temp->Format("Sequencer##%d", project->NextWindowId());
	name = SeqString::Temp->Copy();
	// scene settingss
	sceneUISettings = new SeqList<SeqSceneUISettings>();
	for (int i = 0; i < project->SceneCount(); i++)
	{
		SeqScene *scene = project->GetScene(i);
		sceneUISettings->Add(SeqSceneUISettings());
		SeqSceneUISettings *settings = sceneUISettings->GetPtr(sceneUISettings->Count() - 1);
		settings->scene = scene;
		settings->channelHeights = new SeqList<int>();
		for (int j = 0; j < scene->ChannelCount(); j++)
		{
			settings->channelHeights->Add(initialChannelHeight);
		}
	}
	sceneSettings = GetSceneUISettings(scene);
	// start listening for project changes
	project->AddActionHandler(this);
}

SeqUISequencer::~SeqUISequencer()
{
	// stop listening for project changes
	SeqProject *project = Sequentia::GetProject();
	project->RemoveActionHandler(this);
	// free memory
	delete[] name;
	for (int i = 0; i < sceneUISettings->Count(); i++)
	{
		delete sceneUISettings->GetPtr(i)->channelHeights;
	}
	delete sceneUISettings;
}

void SeqUISequencer::PreExecuteAction(const SeqActionType type, const SeqActionExecution execution, const void *actionData)
{
	switch (type)
	{
		case SeqActionType::AddChannel:
			if (execution == SeqActionExecution::Do)
			{
				SeqActionAddChannel *data = (SeqActionAddChannel*)actionData;
				SeqProject *project = Sequentia::GetProject();
				SeqScene *scene = project->GetSceneById(data->sceneId);
				SeqSceneUISettings *settings = GetSceneUISettings(scene);
				settings->channelHeights->Add(initialChannelHeight);
			}
			else
			{
				SeqActionAddChannel *data = (SeqActionAddChannel*)actionData;
				SeqProject *project = Sequentia::GetProject();
				SeqScene *scene = project->GetSceneById(data->sceneId);
				SeqSceneUISettings *settings = GetSceneUISettings(scene);
				settings->channelHeights->RemoveAt(settings->channelHeights->Count() - 1);
			}
			break;
	}
}

SeqWindowType SeqUISequencer::GetWindowType()
{
	return SeqWindowType::Sequencer;
}

void SeqUISequencer::Draw()
{
	bool isWindowNew = false;
	if (!ImGui::FindWindowByName(name))
		isWindowNew = true;
	bool isOpen = true;
	
	if (ImGui::Begin(name, &isOpen, ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar))
	{
		const float headerHeight = ImGui::GetTextLineHeightWithSpacing() + 5;
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
		Sequentia::GetProject()->RemoveWindow(this);
}

void SeqUISequencer::DrawChannelSettings(float rulerHeight, bool isWindowNew)
{
	SeqProject *project = Sequentia::GetProject();
	const ImGuiStyle style = ImGui::GetStyle();
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	const ImVec2 size = ImVec2(settingsPanelWidth, ImGui::GetContentRegionAvail().y);
	
	// draw channel settings
	ImDrawList *drawList = ImGui::GetWindowDrawList();
	
	// default panel width
	if (isWindowNew)
		ImGui::SetColumnOffset(1, settingsPanelWidth + 7);

	// scene selector
	ImGui::PushItemWidth(size.x - 4);
	if (SeqWidgets::SceneSelectCombo(&scene))
	{
		sceneSettings = GetSceneUISettings(scene);
	}
	ImGui::PopItemWidth();

	// draw settings panels
	ImVec2 cursor = origin;
	cursor.y += rulerHeight + 1;
	ImGui::PushClipRect(cursor, ImVec2(cursor.x + size.x, cursor.y + size.y - rulerHeight - style.ScrollbarSize), false);

	for (int i = 0; i < scene->ChannelCount(); i++)
	{
		int channelHeight = sceneSettings->channelHeights->Get(i);
		// only draw if visible
		if (cursor.y - origin.y + channelHeight >= scroll.y &&
			cursor.y - origin.y < scroll.y + size.y)
		{
			drawList->AddRectFilled(ImVec2(cursor.x, cursor.y - scroll.y), ImVec2(cursor.x + size.x, cursor.y - scroll.y + channelHeight), backgroundColor, rounding, 0x1 | 0x8);
			drawList->AddRect(ImVec2(cursor.x, cursor.y - scroll.y), ImVec2(cursor.x + size.x, cursor.y - scroll.y + channelHeight), lineColor, rounding, 0x1 | 0x8, lineThickness);
			ImGui::SetCursorScreenPos(ImVec2(cursor.x + 4, cursor.y - scroll.y + 4));
			ImGui::Text("%s %d", scene->GetChannel(i)->name, i);
		}
		cursor.y += channelHeight + channelVerticalSpacing;
	}
	// draw and handle row resizing
	// code based on ImGui::Column()
	cursor = origin;
	cursor.y += rulerHeight;
	ImGuiContext *g = ImGui::GetCurrentContext();
	for (int i = 0; i < scene->ChannelCount(); i++)
	{
		int channelHeight = sceneSettings->channelHeights->Get(i);
		cursor.y += channelHeight;
		// if visible
		if (cursor.y - origin.y >= scroll.y && 
			cursor.y - origin.y < scroll.y + size.y)
		{
			const ImGuiID row_id = ImGui::GetID("row") + ImGuiID(i);
			ImGui::KeepAliveID(row_id);
			const ImRect row_rect(ImVec2(cursor.x, cursor.y - scroll.y - 4), ImVec2(cursor.x + size.x, cursor.y - scroll.y + 4));
			
			bool hovered, held;
			ImGui::ButtonBehavior(row_rect, row_id, &hovered, &held);
			if (hovered || held)
				g->MouseCursor = ImGuiMouseCursor_ResizeNS;

			// Draw before resize so our items positioning are in sync with the line being drawn
			const ImU32 col = ImGui::GetColorU32(held ? ImGuiCol_ColumnActive : hovered ? ImGuiCol_ColumnHovered : ImGuiCol_Column);
			drawList->AddLine(ImVec2(cursor.x + rounding, cursor.y - scroll.y), ImVec2(cursor.x + size.x, cursor.y - scroll.y), col);

			if (held)
			{
				if (g->ActiveIdIsJustActivated)
					g->ActiveIdClickOffset.y -= 4;   // Store from center of row line (we used a 8 high rect for row clicking)

				float newHeight = ImClamp(g->IO.MousePos.y - (cursor.y - channelHeight - scroll.y) - g->ActiveIdClickOffset.y, minChannelHeight, maxChannelHeight);
				sceneSettings->channelHeights->Set(i, (int)newHeight);
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
	ImGuiContext *imContext = ImGui::GetCurrentContext();
	ImDrawList *drawList = ImGui::GetWindowDrawList();
	const ImGuiStyle style = ImGui::GetStyle();
	ImGuiWindow *window = ImGui::GetCurrentWindow();
	const ImGuiID id = window->GetID("ruler");

	const ImVec2 origin = ImVec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y);
	const float width = ImGui::GetContentRegionAvailWidth() - style.ScrollbarSize;
	const float thickness = 1.0f;
	const ImRect totalRect(origin, ImVec2(origin.x + width, origin.y + height));

	if (!ImGui::ItemAdd(totalRect, &id))
	{
		return;
	}

	if (imContext->ActiveId != id && !Sequentia::IsDragging() && 
		ImGui::IsItemHovered() && ImGui::IsMouseDown(0))
	{
		ImGui::SetActiveID(id, window);
		Sequentia::DragMode = SeqDragMode::Ruler;
	}

	if (imContext->ActiveId == id)
	{
		if (ImGui::IsMouseDown(0))
		{
			// check if the mouse is in the window (if not pos.x/y == -9999.00)
			if (ImGui::GetMousePos().y >= 0)
			{
				float relativeMouseX = ImGui::GetMousePos().x - origin.x;
				if (imContext->ActiveIdIsJustActivated)
				{
					dragStartPosition = position + PixelsToTime(relativeMouseX);
					imContext->DragLastMouseDelta = ImVec2(0.f, 0.f);
				}
				const ImVec2 newDragMouseDelta = ImGui::GetMouseDragDelta(0, 1.0f);
				const float newZoom = zoom + (newDragMouseDelta.y - imContext->DragLastMouseDelta.y) / (100.f / zoom);
				zoom = ImClamp(newZoom, 0.0001f, 1000.f);
				position = dragStartPosition - PixelsToTime(relativeMouseX);
				if (position < 0 || width > TimeToPixels(scene->GetLength()))
				{
					position = 0;
					dragStartPosition = position + PixelsToTime(relativeMouseX);
				}
				scroll.x = TimeToPixels(position);
				overrideScrollX = true;
				imContext->DragLastMouseDelta = newDragMouseDelta;
			}
		}
		else
		{
			ImGui::ClearActiveID();
			Sequentia::DragMode = SeqDragMode::None;
		}
	}

	// clip so we won't draw on top of the settings panel
	ImGui::PushClipRect(totalRect.Min, totalRect.Max, false);

	// underline
	drawList->AddLine(ImVec2(origin.x, origin.y + height - 2), ImVec2(origin.x + width, origin.y + height - 2), lineColor, thickness);

	// seconds lines
	float secondWidth = pixelsPerSecond / zoom; // width of a second in pixels
	int seconds = SEQ_TIME_FLOOR_IN_SECONDS(position); // first full second left of the view position
	int secondStep = (int)ceil(80.f / secondWidth); // calc min second step so steps won't be too small for time labels
	if (secondStep > 1 && secondStep % 2 != 0) // only have even numbered or single second steps
		secondStep += 1;
	seconds = seconds - (seconds % secondStep); // start only at full secondStep intervals, not in-between
	float firstOffset = -TimeToPixels(position % (secondStep * 1000000));
	float step = secondStep * secondWidth;
	for (float x = origin.x + firstOffset; x < origin.x + width; x += step)
	{
		drawList->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + height - 1), lineColor, thickness);
		ImGui::SetCursorScreenPos(ImVec2(x + style.FramePadding.x, origin.y));
		int hours, mins, secs, us;
		secs = seconds;
		us = 0;
		mins = secs / 60;
		secs %= 60;
		hours = mins / 60;
		mins %= 60;
		ImGui::Text("%02d:%02d:%02d.%d", hours, mins, secs, us);
		seconds += secondStep;
	}

	// draw the play position
	float x = origin.x + SEQ_TIME_IN_SECONDS((double)(scene->player->GetPlaybackTime() - position)) * secondWidth;
	drawList->AddRectFilled(ImVec2(x - 2, origin.y), ImVec2(x + 3, origin.y + height), lineColor);

	// stop drawing the ruler
	ImGui::PopClipRect();

	// setup the position for channel drawing
	ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + height));
}

void SeqUISequencer::DrawChannels()
{
	const ImGuiStyle style = ImGui::GetStyle();
	const ImVec2 contentSize = ImVec2(TimeToPixels(scene->GetLength()), (float)TotalChannelHeight() + 100);
	const ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x - style.ScrollbarSize, ImGui::GetContentRegionAvail().y - style.ScrollbarSize);
	ImGui::SetNextWindowContentSize(contentSize);
	ImGui::BeginChild("channels", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
	{
		// scroll x was manually set from code
		if (overrideScrollX)
		{
			ImGui::SetScrollX(scroll.x);
			overrideScrollX = false;
		}
		position = PixelsToTime(ImGui::GetScrollX());
		scroll.y = ImGui::GetScrollY();
		scroll.x = ImGui::GetScrollX();

		const ImVec2 origin = ImGui::GetCursorScreenPos();

		ImVec2 cursor = origin;
		for (int i = 0; i < scene->ChannelCount(); i++)
		{
			int channelHeight = sceneSettings->channelHeights->Get(i);
			// only draw if visible
			if (cursor.y - origin.y + channelHeight >= scroll.y && 
				cursor.y - origin.y < scroll.y + size.y)
			{
				DrawChannel(scene->GetChannel(i), cursor, size, contentSize, channelHeight);
			}
			cursor.y += channelHeight + channelVerticalSpacing;
		}

		static ImVec2 scrollPosTextSize = ImGui::CalcTextSize("ScrollPos: 00000.000, 00000.000 zoom: 0000.000");
		ImGui::SetCursorPos(ImVec2(ImGui::GetScrollX() + size.x - scrollPosTextSize.x, ImGui::GetScrollY() + size.y - scrollPosTextSize.y));
		ImGui::Text("ScrollPos: %.3f, %.3f zoom: %.3f", ImGui::GetScrollX(), ImGui::GetScrollY(), zoom);

		// draw play time line
		ImDrawList *drawList = ImGui::GetWindowDrawList();
		float secondWidth = pixelsPerSecond / zoom;
		float x = origin.x + SEQ_TIME_IN_SECONDS((double)scene->player->GetPlaybackTime()) * secondWidth;
		drawList->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + size.y), lineColor);
	}
	ImGui::EndChild();
}

void SeqUISequencer::DrawChannel(SeqChannel *channel, ImVec2 cursor, ImVec2 availableSize, ImVec2 contentSize, float height)
{
	ImDrawList *drawList = ImGui::GetWindowDrawList();

	const ImRect totalRect(ImVec2(cursor.x, cursor.y), ImVec2(cursor.x + availableSize.x, cursor.y + height));

	bool isHovering = false;
	SeqSelection *dragClipSelection = Sequentia::GetDragClipSelection();
	if (dragClipSelection != nullptr)
	{
		if (ImGui::IsMouseHoveringRect(totalRect.Min, totalRect.Max))
		{
			if (ImGui::IsMouseDown(0))
			{
				isHovering = true;
				if (dragClipSelection->GetParent() != channel)
					dragClipSelection->SetParent(channel);
				dragClipSelection->location.SetPosition((PixelsToTime(ImGui::GetMousePos().x - cursor.x)) - dragClipSelection->grip);
			}
			else
			{
				if (dragClipSelection->IsNewClip())
				{
					Sequentia::GetProject()->AddAction(SeqActionFactory::AddClipToChannel(dragClipSelection));
				}
				else if (dragClipSelection->IsMoved())
				{
					Sequentia::GetProject()->AddAction(SeqActionFactory::MoveClip(dragClipSelection));
				}
			}
		}
		else if (dragClipSelection->GetParent() == channel)
		{
			dragClipSelection->SetParent(nullptr);
		}
	}

	drawList->AddRectFilled(ImVec2(cursor.x + ImGui::GetScrollX(), cursor.y), ImVec2(cursor.x + contentSize.x, cursor.y + height), backgroundColor, rounding, 0x2 | 0x4);
	drawList->AddRect(ImVec2(cursor.x, cursor.y), ImVec2(cursor.x + contentSize.x, cursor.y + height), lineColor, rounding, 0x2 | 0x4, lineThickness);
	
	// calc left and right side of the visible part of the channel
	const double start = position;
	const double end = position + PixelsToTime(availableSize.x);
	// draw clips
	for (int i = 0; i < channel->ClipCount(); i++)
	{
		SeqClip *clip = channel->GetClip(i);
		if (!clip->isHidden)
		{
			const double left = (double)clip->location.leftTime;
			const double right = (double)clip->location.rightTime;
			// culling
			if (left > end)
				break; // clips are sorted by left position, so we can exit the loop here
			if (left >= start || right >= start)
			{
				const ImVec2 clipPosition = ImVec2(cursor.x + TimeToPixels(left), cursor.y);
				const ImVec2 clipSize = ImVec2(cursor.x + TimeToPixels(right) - clipPosition.x, height);
				bool isHovered = false;
				if (ClipInteraction(clip, clipPosition, clipSize, &isHovered))
					DrawClip(clip, clipPosition, clipSize, isHovered && !Sequentia::IsDragging());
			}
		}
	}
	// draw clip selections
	for (int i = 0; i < channel->ClipSelectionCount(); i++)
	{
		SeqSelection *selection = channel->GetClipSelection(i);
		const double left = (double)selection->location.leftTime;
		const double right = (double)selection->location.rightTime;
		// culling
		if (left > end)
			break; // clip selections are sorted by left position, so we can exit the loop here
		if (left >= start || right >= start)
		{
			const ImVec2 clipPosition = ImVec2(cursor.x + TimeToPixels(left), cursor.y);
			const ImVec2 clipSize = ImVec2(cursor.x + TimeToPixels(right) - clipPosition.x, height);
			bool isError = selection->GetClip()->mediaType != channel->type;
			DrawClip(selection->GetClip(), clipPosition, clipSize, true, isError);
		}
	}
}

bool SeqUISequencer::ClipInteraction(SeqClip *clip, const ImVec2 position, const ImVec2 size, bool *isHovered)
{
	bool clipExists = true;
	const ImVec2 tl = ImVec2(position.x, position.y);
	const ImVec2 br = ImVec2(position.x + size.x, position.y + size.y);
	SeqChannel *channel = clip->GetParent();
	SeqString::Temp->Format("context_channel%d_clip%d", channel == nullptr ? -1 : channel->actionId, clip->actionId);

	// handle popup menu if has been opened in the past
	if (ImGui::BeginPopupContextVoid(SeqString::Temp->Buffer))
	{
		if (ImGui::Selectable("Delete"))
		{
			Sequentia::GetProject()->AddAction(SeqActionFactory::RemoveClipFromChannel(clip));
			clipExists = false;
		}
		ImGui::EndPopup();
	}
	else // if there is no context menu active
	{
		*isHovered = ImGui::IsMouseHoveringRect(tl, br);
		if (*isHovered)
		{
			// clip hovered
			if (ImGui::IsMouseClicked(1))
			{
				// start context menu popup
				ImGui::OpenPopupEx(SeqString::Temp->Buffer, false);
			}
			if (ImGui::IsMouseDown(0) && !Sequentia::IsDragging())
			{
				// start dragging clip
				Sequentia::SetDragClip(clip, PixelsToTime(ImGui::GetMousePos().x - position.x));
			}
		}
	}

	return clipExists;
}

void SeqUISequencer::DrawClip(SeqClip *clip, const ImVec2 position, const ImVec2 size, const bool isHovered, const bool isError)
{
	ImDrawList *drawList = ImGui::GetWindowDrawList();
	const ImVec2 tl = ImVec2(position.x, position.y);
	const ImVec2 br = ImVec2(position.x + size.x, position.y + size.y);
	ImGui::ItemSize(size);
	if (isError)
		drawList->AddRectFilled(tl, br, errorColor);
	else
		drawList->AddRectFilled(tl, br, clipBackgroundColor);
	float lineThickness = isHovered ? 2.0f : 1.0f;
	drawList->AddRect(tl, br, lineColor, 0.0f, -1, lineThickness);
	ImGui::SetCursorScreenPos(position);
	ImGui::Text("%s", clip->GetLabel());
}

SeqSceneUISettings* SeqUISequencer::GetSceneUISettings(SeqScene *scene)
{
	for (int i = 0; i < sceneUISettings->Count(); i++)
	{
		SeqSceneUISettings* settings = sceneUISettings->GetPtr(i);
		if (settings->scene == scene)
			return settings;
	}
	return nullptr;
}

int SeqUISequencer::TotalChannelHeight()
{
	int totalHeight = 0;
	for (int i = 0; i < sceneSettings->channelHeights->Count(); i++)
		totalHeight += sceneSettings->channelHeights->Get(i) + channelVerticalSpacing;
	// remove spacing on the bottom most channel
	totalHeight -= channelVerticalSpacing;
	return totalHeight;
}

int64_t SeqUISequencer::PixelsToTime(float pixels)
{
	return SEQ_TIME(pixels / (pixelsPerSecond / zoom));
}

float SeqUISequencer::TimeToPixels(int64_t time)
{
	return time * (pixelsPerSecond / zoom) / SEQ_TIME_BASE;
}

void SeqUISequencer::Serialize(SeqSerializer *serializer)
{
	serializer->Write(scene->id);
	serializer->Write(position);
	serializer->Write(zoom);
	serializer->Write(scroll.y);
	serializer->Write(settingsPanelWidth);
	serializer->Write(sceneUISettings->Count());
	for (int i = 0; i < sceneUISettings->Count(); i++)
	{
		SeqSceneUISettings *settings = sceneUISettings->GetPtr(i);
		serializer->Write(settings->scene->id);
		serializer->Write(settings->channelHeights->Count());
		for (int j = 0; j < settings->channelHeights->Count(); j++)
			serializer->Write(settings->channelHeights->Get(j));
	}
}

void SeqUISequencer::Deserialize(SeqSerializer *serializer)
{
	SeqProject *project = Sequentia::GetProject();
	int sceneId = serializer->ReadInt();
	scene = project->GetSceneById(sceneId);
	position = serializer->ReadDouble();
	zoom = serializer->ReadFloat();
	scroll.y = serializer->ReadFloat();
	settingsPanelWidth = serializer->ReadFloat();
	int channelSettingsCount = serializer->ReadInt();
	for (int i = 0; i < channelSettingsCount; i++)
	{
		int sceneId = serializer->ReadInt();
		SeqScene *scene = project->GetSceneById(sceneId);
		SeqSceneUISettings *settings = GetSceneUISettings(scene);
		int channelHeightCount = serializer->ReadInt();
		for (int j = 0; j < channelHeightCount; j++)
		{
			settings->channelHeights->ReplaceAt(serializer->ReadInt(), j);
		}
	}
}
