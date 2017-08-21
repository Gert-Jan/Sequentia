#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include "SeqProject.h";
#include "SeqUILibrary.h";

int SeqUILibrary::nextWindowId = 0;

SeqUILibrary::SeqUILibrary(SeqProject *project) :
	project(project),
	windowId(nextWindowId)
{
	nextWindowId++;
	Init();
}

SeqUILibrary::SeqUILibrary(SeqProject *project, int windowId) :
	project(project),
	windowId(windowId)
{
	nextWindowId = ImMax(nextWindowId, windowId + 1);
	Init();
}

SeqUILibrary::~SeqUILibrary()
{
	delete[] name;
}

void SeqUILibrary::Init()
{
	// set name
	ImGuiContext *g = ImGui::GetCurrentContext();
	int size = ImFormatString(g->TempBuffer, IM_ARRAYSIZE(g->TempBuffer), "Library##%d", windowId);
	name = new char[size + 1];
	strcpy(name, g->TempBuffer);
	// start listening for project changes
	project->AddActionHandler(this);
}

void SeqUILibrary::ActionDone(const SeqAction action)
{
	switch (action.type)
	{
	}
}

void SeqUILibrary::ActionUndone(const SeqAction action)
{
	switch (action.type)
	{
	}
}

void SeqUILibrary::Draw()
{
	bool isWindowNew = false;
	if (!ImGui::FindWindowByName(name))
		isWindowNew = true;
	bool isOpen = true;

	if (ImGui::Begin(name, &isOpen, ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar))
	{
	}
	ImGui::End();
	if (!isOpen)
		project->RemoveLibrary(this);
}
