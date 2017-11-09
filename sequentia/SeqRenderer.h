#pragma once
#include "SeqMaterial.h";
#include "imgui.h";

struct SDL_Window;
struct ImDrawData;

class SeqRenderer
{
public:
	static void InitGL();
	static void RefreshDeviceObjects();
	static void CreateDeviceObjects();
	static void InvalidateDeviceObjects();
	static void CreateFontsTexture();
	static void Render();
	static void Shutdown();

private:
	static ImVec4 clearColor;
	static const int videoCount = 4;
	static SeqMaterial fontMaterial;
	static SeqMaterial videoMaterial[videoCount];
	static unsigned int vboHandle;
	static unsigned int vaoHandle;
	static unsigned int elementsHandle;
};
