#include "SeqRenderer.h"
#include "Sequentia.h"
#include "SeqProject.h"
#include "SeqMaterialInstance.h"
#include "SeqList.h"
#include "imgui.h"
#include <GL/gl3w.h>
extern "C"
{
	#include "libavformat/avformat.h"
}

ImVec4 SeqRenderer::clearColor = ImColor(114, 144, 154);
unsigned int SeqRenderer::vboHandle = 0;
unsigned int SeqRenderer::vaoHandle = 0;
unsigned int SeqRenderer::elementsHandle = 0;
unsigned int SeqRenderer::frameBufferHandle = 0;
int SeqRenderer::framebufferWidth = 1280;
int SeqRenderer::framebufferHeight = 720;
SeqList<SeqMaterialInstance*>* SeqRenderer::materialsImGui = new SeqList<SeqMaterialInstance*>();
SeqList<SeqMaterialInstance*>* SeqRenderer::materialsPlayer = new SeqList<SeqMaterialInstance*>();
SeqList<SeqMaterialInstance*>* SeqRenderer::deleteMaterials = new SeqList<SeqMaterialInstance*>();
SeqMaterial SeqRenderer::fontMaterial = SeqMaterial(1);
SeqMaterial SeqRenderer::videoMaterial = SeqMaterial(3);
SeqMaterial SeqRenderer::playerMaterial = SeqMaterial(1);

void SeqRenderer::InitGL()
{
	av_register_all();
	gl3wInit();
	RefreshDeviceObjects();
}

void SeqRenderer::RefreshDeviceObjects()
{
	if (materialsImGui->Count() == 0)
		CreateDeviceObjects();
}

void SeqRenderer::CreateDeviceObjects()
{
	// Backup GL state
	GLint last_texture, last_array_buffer, last_vertex_array;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

	const GLchar *vertex_shader =
		"#version 330\n"
		"uniform mat4 ProjMtx;\n"
		"in vec2 Position;\n"
		"in vec2 UV;\n"
		"in vec4 Color;\n"
		"out vec2 Frag_UV;\n"
		"out vec4 Frag_Color;\n"
		"void main()\n"
		"{\n"
		"	Frag_UV = UV;\n"
		"	Frag_Color = Color;\n"
		"	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
		"}\n";

	const GLchar *fragment_shader_default =
		"#version 330\n"
		"uniform sampler2D Texture0;\n"
		"in vec2 Frag_UV;\n"
		"in vec4 Frag_Color;\n"
		"out vec4 Out_Color;\n"
		"void main()\n"
		"{\n"
		"	Out_Color = Frag_Color * texture( Texture0, Frag_UV.st);\n"
		"}\n";

	const GLchar *fragment_shader_video =
		"#version 330\n"
		"uniform sampler2D Texture0;\n"
		"uniform sampler2D Texture1;\n"
		"uniform sampler2D Texture2;\n"
		"in vec2 Frag_UV;\n"
		"in vec4 Frag_Color;\n"
		"out vec4 Out_Color;\n"
		"void main()\n"
		"{\n"
		/*
		"  mat3 toRGB = mat3(\n"
		"    1,  0      ,  1.28033,\n"
		"    1, -0.21482, -0.38059,\n"
		"    1,  2.12798,  0      );\n"
		*/
		// BT.709 - https://en.wikipedia.org/wiki/YUV
		"  mat3 toRGB = mat3(\n"
		"    1.0    ,  1.0    , 1.0    ,\n"
		"    0.0    , -0.21482, 2.12798,\n"
		"    1.28033, -0.38059, 0.0    );\n"
		// BT.601 - https://en.wikipedia.org/wiki/YUV
		/*
		"  mat3 toRGB = mat3(\n"
		"    1.0    ,  1.0    , 1.0    ,\n"
		"    0.0    , -0.39465, 2.03211,\n"
		"    1.13983, -0.58060, 0.0    );\n"
		*/
		// BT.709 - https://github.com/webmproject/webm-tools/blob/master/vpx_ios/VPXExample/nv12_fragment_shader.glsl
		/*
		"  mat3 toRGB = mat3(\n";
		"    1.0,     1.0,    1.0   ,\n"
		"    0.0,    -0.1870, 1.8556,\n"
		"    1.5701, -0.4664, 0.0   );\n"
		*/
		// BT.601 - https://github.com/eile/bino/blob/master/src/video_output_color.fs.glsl
		/*
		"  mat3 toRGB = mat3(\n"
		"    1.0  ,  1.0     , 1.0   ,\n"
		"    0.0  , -0.344136, 1.772 ,\n"
		"    1.402, -0.714136, 0.0   );\n"
		*/
		"  vec3 yuv = vec3(\n"
		"    texture(Texture0, Frag_UV.st).r,\n"
		"    texture(Texture1, Frag_UV.st).r - 0.5,\n"
		"    texture(Texture2, Frag_UV.st).r - 0.5);\n"
		"  vec3 rgb = toRGB * yuv;\n"
		"  Out_Color = Frag_Color * vec4(rgb, 1.0);"
		//"	Out_Color = Frag_Color * (texture(Texture0, Frag_UV.st).r * vec4(1, 0, 0, 0) + texture(Texture1, Frag_UV.st).r * vec4(0, 1, 0, 0) + texture(Texture2, Frag_UV.st).r * vec4(0, 0, 1, 0) + vec4(0, 0, 0, 1));\n"
		"}\n";

	glGenBuffers(1, &vboHandle);
	glGenBuffers(1, &elementsHandle);
	glGenFramebuffers(1, &frameBufferHandle);

	glGenVertexArrays(1, &vaoHandle);
	glBindVertexArray(vaoHandle);
	glBindBuffer(GL_ARRAY_BUFFER, vboHandle);

	fontMaterial.Init(vertex_shader, fragment_shader_default);
	videoMaterial.Init(vertex_shader, fragment_shader_video);
	playerMaterial.Init(vertex_shader, fragment_shader_default);

	CreateFontsMaterialInstance();

	// Restore modified GL state
	glBindTexture(GL_TEXTURE_2D, last_texture);
	glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
	glBindVertexArray(last_vertex_array);
}

void SeqRenderer::InvalidateDeviceObjects()
{
	if (vaoHandle)
		glDeleteVertexArrays(1, &vaoHandle);
	if (vboHandle)
		glDeleteBuffers(1, &vboHandle);
	if (elementsHandle)
		glDeleteBuffers(1, &elementsHandle);
	vaoHandle = vboHandle = elementsHandle = 0;

	fontMaterial.Dispose();
	ImGui::GetIO().Fonts->TexID = 0;
	for (int i = 0; i < materialsImGui->Count(); i++)
		materialsImGui->Get(i)->Dispose();
	for (int i = 0; i < materialsPlayer->Count(); i++)
		materialsPlayer->Get(i)->Dispose();
	videoMaterial.Dispose();
	playerMaterial.Dispose();
}

void SeqRenderer::Render()
{
	ImGuiIO& io = ImGui::GetIO();
	SeqProject *project = Sequentia::GetProject();

	glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
	glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
	glClear(GL_COLOR_BUFFER_BIT);

	ImGui::Render();
	ImDrawData* drawData = ImGui::GetDrawData();

	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	SetImGuiViewport();
	if (framebufferWidth == 0 || framebufferHeight == 0)
		return;
	drawData->ScaleClipRects(io.DisplayFramebufferScale);

	// Backup GL state
	GLint lastActiveTexture; glGetIntegerv(GL_ACTIVE_TEXTURE, &lastActiveTexture);
	glActiveTexture(GL_TEXTURE0);
	GLint lastProgram; glGetIntegerv(GL_CURRENT_PROGRAM, &lastProgram);
	GLint lastTexture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTexture);
	GLint lastArrayBuffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &lastArrayBuffer);
	GLint lastElementArrayBuffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &lastElementArrayBuffer);
	GLint lastVertexArray; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &lastVertexArray);
	GLint lastBlendSourceRGB; glGetIntegerv(GL_BLEND_SRC_RGB, &lastBlendSourceRGB);
	GLint lastBlendDestRGB; glGetIntegerv(GL_BLEND_DST_RGB, &lastBlendDestRGB);
	GLint lastBlendSourceAlpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, &lastBlendSourceAlpha);
	GLint lastBlendDestAlpha; glGetIntegerv(GL_BLEND_DST_ALPHA, &lastBlendDestAlpha);
	GLint lastBlendEquationRGB; glGetIntegerv(GL_BLEND_EQUATION_RGB, &lastBlendEquationRGB);
	GLint lastBlendEquationAlpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &lastBlendEquationAlpha);
	GLint lastViewport[4]; glGetIntegerv(GL_VIEWPORT, lastViewport);
	GLint lastScissorBox[4]; glGetIntegerv(GL_SCISSOR_BOX, lastScissorBox);
	GLboolean lastEnableBlend = glIsEnabled(GL_BLEND);
	GLboolean lastEnableCullFace = glIsEnabled(GL_CULL_FACE);
	GLboolean lastEnableDepthTest = glIsEnabled(GL_DEPTH_TEST);
	GLboolean lastEnableScissorTest = glIsEnabled(GL_SCISSOR_TEST);

	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);

	// Setup viewport, orthographic projection matrix
	const float orthoProjectionImGui[4][4] =
	{
		{ 2.0f / io.DisplaySize.x, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 2.0f / -io.DisplaySize.y, 0.0f, 0.0f },
		{ 0.0f, 0.0f, -1.0f, 0.0f },
		{ -1.0f, 1.0f, 0.0f, 1.0f },
	};
	const float orthoProjectionPlayer[4][4] =
	{
		{ 2.0f / project->width, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 2.0f / -project->height, 0.0f, 0.0f },
		{ 0.0f, 0.0f, -1.0f, 0.0f },
		{ -1.0f, 1.0f, 0.0f, 1.0f },
	};

	for (int i = 0; i < materialsImGui->Count(); i++)
		materialsImGui->Get(i)->Begin(orthoProjectionImGui, vaoHandle);
	for (int i = 0; i < materialsPlayer->Count(); i++)
		materialsPlayer->Get(i)->Begin(orthoProjectionPlayer, vaoHandle);

	for (int n = 0; n < drawData->CmdListsCount; n++)
	{
		const ImDrawList* cmdList = drawData->CmdLists[n];
		const ImDrawIdx* indexBufferOffset = 0;

		glBindBuffer(GL_ARRAY_BUFFER, vboHandle);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmdList->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmdList->VtxBuffer.Data, GL_STREAM_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementsHandle);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmdList->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmdList->IdxBuffer.Data, GL_STREAM_DRAW);

		for (int cmdIndex = 0; cmdIndex < cmdList->CmdBuffer.Size; cmdIndex++)
		{
			const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmdIndex];
			if (pcmd->UserCallback)
			{
				pcmd->UserCallback(cmdList, pcmd);
			}
			else
			{
				SeqMaterialInstance* mat = (SeqMaterialInstance*)pcmd->TextureId;
				mat->BindTextures();
				glScissor((int)pcmd->ClipRect.x, (int)(framebufferHeight - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
				glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, indexBufferOffset);
			}
			indexBufferOffset += pcmd->ElemCount;
		}
	}

	// Restore modified GL state
	glUseProgram(lastProgram);
	glBindTexture(GL_TEXTURE_2D, lastTexture);
	glActiveTexture(lastActiveTexture);
	glBindVertexArray(lastVertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, lastArrayBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lastElementArrayBuffer);
	glBlendEquationSeparate(lastBlendEquationRGB, lastBlendEquationAlpha);
	glBlendFuncSeparate(lastBlendSourceRGB, lastBlendDestRGB, lastBlendSourceAlpha, lastBlendDestAlpha);
	if (lastEnableBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
	if (lastEnableCullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
	if (lastEnableDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
	if (lastEnableScissorTest) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
	glViewport(lastViewport[0], lastViewport[1], (GLsizei)lastViewport[2], (GLsizei)lastViewport[3]);
	glScissor(lastScissorBox[0], lastScissorBox[1], (GLsizei)lastScissorBox[2], (GLsizei)lastScissorBox[3]);

	// Often materials are removed while they are still used. After rendering one frame we should be safe to actually delete those materials.
	while (deleteMaterials->Count() > 0)
	{
		const int index = deleteMaterials->Count() - 1;
		SeqMaterialInstance *materialInstance = deleteMaterials->Get(index);
		materialInstance->Dispose();
		delete materialInstance;
		deleteMaterials->RemoveAt(index);
	}
}

void SeqRenderer::Shutdown()
{
	InvalidateDeviceObjects();
}

void SeqRenderer::RemoveMaterialInstance(SeqMaterialInstance *materialInstance)
{
	materialsImGui->Remove(materialInstance);
	materialsPlayer->Remove(materialInstance);
	deleteMaterials->Add(materialInstance);
}

SeqMaterialInstance* SeqRenderer::CreateVideoMaterialInstance()
{
	SeqMaterialInstance *videoMaterialInstance = new SeqMaterialInstance(&videoMaterial);
	videoMaterialInstance->Init();
	materialsPlayer->Add(videoMaterialInstance);
	return videoMaterialInstance;
}

void SeqRenderer::CreateFontsMaterialInstance()
{
	// Create material instance
	SeqMaterialInstance *fontMaterialInstance = new SeqMaterialInstance(&fontMaterial);
	fontMaterialInstance->Init();
	materialsImGui->Add(fontMaterialInstance);

	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	unsigned char *pixels;
	int width, height;
	// Load as RGBA 32-bits for OpenGL3 demo because it is more likely to be compatible with user's existing shader.
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	// Store current state
	GLint lastTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTexture);

	// Upload texture to graphics system
	glBindTexture(GL_TEXTURE_2D, fontMaterialInstance->textureHandles[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// Store our identifier
	io.Fonts->TexID = (void *)fontMaterialInstance;

	// Restore state
	glBindTexture(GL_TEXTURE_2D, lastTexture);
}

SeqMaterialInstance* SeqRenderer::CreatePlayerMaterialInstance()
{
	SeqProject *project = Sequentia::GetProject();

	// Create material instance
	SeqMaterialInstance *playerMaterialInstance = new SeqMaterialInstance(&playerMaterial);
	playerMaterialInstance->Init();
	materialsImGui->Add(playerMaterialInstance);

	// Store current state
	GLint lastTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTexture);

	// Create framebuffer texture
	glBindTexture(GL_TEXTURE_2D, playerMaterialInstance->textureHandles[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, project->width, project->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	// Restore state
	glBindTexture(GL_TEXTURE_2D, lastTexture);

	return playerMaterialInstance;
}

void SeqRenderer::CreateVideoTextures(AVFrame* frame, GLuint texId[3])
{
	// Store current state
	GLint lastTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTexture);

	// Create textures
	glBindTexture(GL_TEXTURE_2D, texId[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->linesize[0], frame->height, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[0]);

	glBindTexture(GL_TEXTURE_2D, texId[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->linesize[1], frame->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[1]);

	glBindTexture(GL_TEXTURE_2D, texId[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->linesize[2], frame->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[2]);

	// Restore state
	glBindTexture(GL_TEXTURE_2D, lastTexture);
}

void SeqRenderer::OverwriteVideoTextures(AVFrame* frame, GLuint texId[3])
{
	// Store current state
	GLint lastTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTexture);

	// Overwrite textures
	glBindTexture(GL_TEXTURE_2D, texId[0]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->linesize[0], frame->height, GL_RED, GL_UNSIGNED_BYTE, frame->data[0]);
	glBindTexture(GL_TEXTURE_2D, texId[1]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->linesize[1], frame->height / 2, GL_RED, GL_UNSIGNED_BYTE, frame->data[1]);
	glBindTexture(GL_TEXTURE_2D, texId[2]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->linesize[2], frame->height / 2, GL_RED, GL_UNSIGNED_BYTE, frame->data[2]);

	// Restore state
	glBindTexture(GL_TEXTURE_2D, lastTexture);
}

void SeqRenderer::BindFramebuffer(const ImDrawList* drawList, const ImDrawCmd* command)
{
	SeqMaterialInstance *framebufferOutput = (SeqMaterialInstance *)command->UserCallbackData;
	
	if (framebufferOutput == nullptr)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		SetImGuiViewport();
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);
		framebufferOutput->BindTextures();
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, framebufferOutput->textureHandles[0], 0);
		SetPlayerViewport();
	}
}

void SeqRenderer::SwitchFramebuffer(const ImDrawList* drawList, const ImDrawCmd* command)
{
	SeqProject *project = Sequentia::GetProject();
	SeqMaterialInstance *framebufferOutput = (SeqMaterialInstance *)command->UserCallbackData;

	// copy the previous frame buffer to the new output frame buffer
	framebufferOutput->BindTextures();
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, project->width, project->height, 0);
	
	// bind the new output to the framebuffer
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, framebufferOutput->textureHandles[0], 0);
}

void SeqRenderer::SetImGuiViewport()
{
	ImGuiIO& io = ImGui::GetIO();
	framebufferWidth = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
	framebufferHeight = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
	glViewport(0, 0, framebufferWidth, framebufferHeight);
}

void SeqRenderer::SetPlayerViewport()
{
	SeqProject *project = Sequentia::GetProject();
	framebufferWidth = project->width;
	framebufferHeight = project->height;
	glViewport(0, 0, framebufferWidth, framebufferHeight);
}
