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
	static void MessageCallbackGL(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
	static void RefreshDeviceObjects();
	static void CreateDeviceObjects();
	static void InvalidateDeviceObjects();
	static void Render();
	static void Shutdown();
	static void RemoveMaterialInstance(SeqMaterialInstance *materialInstance);
	static void CreateFontsMaterialInstance();
	static SeqMaterialInstance* CreateVideoMaterialInstance();
	static SeqMaterialInstance* CreatePlayerMaterialInstance();
	static SeqMaterialInstance* CreateMaterialInstance(SeqMaterial *material, float *projectionMatrix);
	static void CreateVideoTextures(AVFrame* frame, SeqMaterialInstance* instance);
	static void OverwriteVideoTextures(AVFrame* frame, GLuint texId[3]);
	static void BindFramebuffer(const ImDrawList* drawList, const ImDrawCmd* command);
	static void SwitchFramebuffer(const ImDrawList* drawList, const ImDrawCmd* command);
	static void DownloadTexture(const ImDrawList* drawList, const ImDrawCmd* command);
	static void FillDefaultProjectionMatrix(float *target);
	static void SetProjectionMatrixDimensions(float *target, float width, float height);
	static void SetImGuiViewport();
	static void SetViewport(ImVec4 rect);

public:
	static SeqMaterial exportYMaterial;
	static SeqMaterial exportUMaterial;
	static SeqMaterial exportVMaterial;
	static SeqMaterial onlyTextureMaterial;

private:
	static ImVec4 clearColor;
	static unsigned int vboHandle;
	static unsigned int elementsHandle;
	static unsigned int frameBufferHandle;

	static int framebufferHeight;
	static float displayMatrix[4][4];
	static float projectOutputMatrix[4][4];

	static SeqList<SeqMaterialInstance*> *materials;
	static SeqList<SeqMaterialInstance*> *deleteMaterials;
	static SeqMaterial fontMaterial;
	static SeqMaterial videoMaterial;
	static SeqMaterial playerMaterial;
};
