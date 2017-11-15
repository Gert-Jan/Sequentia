#pragma once
#include "SeqMaterial.h";
#include "imgui.h";

struct SDL_Window;
struct ImDrawData;
struct AVFrame;

template<class T>
class SeqList;

class SeqRenderer
{
public:
	static void InitGL();
	static void RefreshDeviceObjects();
	static void CreateDeviceObjects();
	static void InvalidateDeviceObjects();
	static void Render();
	static void Shutdown();
	static SeqMaterial* GetVideoMaterial();
	static void CreateFontsTexture();
	static void CreateVideoTextures(AVFrame* frame, GLuint texId[3]);
	static void OverwriteVideoTextures(AVFrame* frame, GLuint texId[3]);

private:
	static ImVec4 clearColor;
	static unsigned int vboHandle;
	static unsigned int vaoHandle;
	static unsigned int elementsHandle;

	static SeqList<SeqMaterial*>* materials;
	static SeqMaterial fontMaterial;
	static SeqMaterial videoMaterial;
};
