#pragma once

struct SDL_Window;
typedef union SDL_Event SDL_Event;

class SeqProject;
class SeqRenderer;

class Sequentia
{
public:
	static int Run(char *openProject);

private:
	static void InitImGui();
	static void BeginFrame();
	static bool ImGuiProcessEvent(SDL_Event *event);
	static void ImGuiSetClipboardText(void*, const char *text);
	static const char* ImGuiGetClipboardText(void*);

private:
	static bool done;
	static SDL_Window *window;
	static SeqProject *project;
	static bool showImGuiDemo;
	static double time;
	static bool mousePressed[3];
	static float mouseWheel;
};