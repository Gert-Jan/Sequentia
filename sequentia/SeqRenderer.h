#pragma once
#include "SeqMaterial.h"
#include "imgui.h"

struct SDL_Window;
struct ImDrawData;
struct AVFrame;

struct SeqMaterialInstance;

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
	static void RemoveMaterialInstance(SeqMaterialInstance *materialInstance);
	static SeqMaterialInstance* CreateVideoMaterialInstance();
	static void CreateFontsMaterialInstance();
	static SeqMaterialInstance* CreatePlayerMaterialInstance();
	static void CreateVideoTextures(AVFrame* frame, GLuint texId[3]);
	static void OverwriteVideoTextures(AVFrame* frame, GLuint texId[3]);
	static void BindFramebuffer(const ImDrawList* drawList, const ImDrawCmd* command);
	static void SwitchFramebuffer(const ImDrawList* drawList, const ImDrawCmd* command);

private:
	static void SetImGuiViewport();
	static void SetPlayerViewport();

private:
	static ImVec4 clearColor;
	static unsigned int vboHandle;
	static unsigned int vaoHandle;
	static unsigned int elementsHandle;
	static unsigned int frameBufferHandle;

	static int framebufferWidth, framebufferHeight;

	static SeqList<SeqMaterialInstance*> *materialsImGui;
	static SeqList<SeqMaterialInstance*> *materialsPlayer;
	static SeqMaterial fontMaterial;
	static SeqMaterial videoMaterial;
	static SeqMaterial playerMaterial;
};
