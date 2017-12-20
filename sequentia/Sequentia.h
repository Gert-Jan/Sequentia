#pragma once

#include "SDL_config.h"

struct SDL_Window;
typedef union SDL_Event SDL_Event;

class SeqProject;
class SeqLibrary;
class SeqRenderer;
class SeqClip;
class SeqClipProxy;
struct SeqLibraryLink;

class Sequentia
{
public:
	static int Run(char *openProject);
	static SeqProject* GetProject();
	static SeqLibrary* GetLibrary();
	static bool IsDragging();
	static SeqClipProxy* GetDragClipProxy();
	static void SetDragClipNew(SeqLibraryLink *link);
	static void SetDragClip(SeqClip *clip, const int64_t grip = 0);

private:
	static void InitImGui();
	static void BeginFrame();
	static bool ImGuiProcessEvent(SDL_Event *event);
	static void ImGuiSetClipboardText(void*, const char *text);
	static const char* ImGuiGetClipboardText(void*);
	static void HandleShortcuts();
	static void HandleMainMenuBar();
	static void HandleDragging();

public:
	static const int TimeBase = 1000000;

private:
	static bool done;
	static SDL_Window *window;
	static SeqProject *project;
	static bool showImGuiDemo;
	static double time;
	static bool mousePressed[3];
	static float mouseWheel;
	static SeqClipProxy *dragClipProxy;
};
