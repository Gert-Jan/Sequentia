// ImGui SDL2 binding with OpenGL3
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include <GL/gl3w.h> 

struct SDL_Window;
typedef union SDL_Event SDL_Event;

IMGUI_API bool ImGui_ImplSdlGL3_Init(SDL_Window* window);
IMGUI_API void ImGui_ImplSdlGL3_Shutdown();
IMGUI_API void ImGui_ImplSdlGL3_NewFrame(SDL_Window* window);
IMGUI_API bool ImGui_ImplSdlGL3_ProcessEvent(SDL_Event* event);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void ImGui_ImplSdlGL3_InvalidateDeviceObjects();
IMGUI_API bool ImGui_ImplSdlGL3_CreateDeviceObjects();

struct Material
{
	GLuint programHandle;
	GLuint vertShaderHandle;
	GLuint fragShaderHandle;
	int textureCount;
	GLuint textureHandles[3];

	int textureAttribLoc[3];
	int projMatAttribLoc;
	int positionAttribLoc;
	int uvAttribLoc;
	int colorAttribLoc;

	void Init(const GLchar* vertShaderSource, const GLchar* fragShaderSource)
	{
		programHandle = glCreateProgram();
		vertShaderHandle = glCreateShader(GL_VERTEX_SHADER);
		fragShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(vertShaderHandle, 1, &vertShaderSource, 0);
		glShaderSource(fragShaderHandle, 1, &fragShaderSource, 0);
		glCompileShader(vertShaderHandle);
		glCompileShader(fragShaderHandle);
		glAttachShader(programHandle, vertShaderHandle);
		glAttachShader(programHandle, fragShaderHandle);
		glLinkProgram(programHandle);

		ImGuiTextBuffer textureName;
		for (int i = 0; i < 3; i++)
		{
			textureName.clear();
			textureName.append("Texture%i", i);
			textureAttribLoc[i] = glGetUniformLocation(programHandle, textureName.c_str());
		}
		projMatAttribLoc = glGetUniformLocation(programHandle, "ProjMtx");
		positionAttribLoc = glGetAttribLocation(programHandle, "Position");
		uvAttribLoc = glGetAttribLocation(programHandle, "UV");
		colorAttribLoc = glGetAttribLocation(programHandle, "Color");

		glEnableVertexAttribArray(positionAttribLoc);
		glEnableVertexAttribArray(uvAttribLoc);
		glEnableVertexAttribArray(colorAttribLoc);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
		glVertexAttribPointer(positionAttribLoc, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
		glVertexAttribPointer(uvAttribLoc, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
		glVertexAttribPointer(colorAttribLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF
	}

	void Begin(const float projMat[4][4], unsigned int g_VaoHandle)
	{
		glUseProgram(programHandle);

		for (int i = 0; i < textureCount; i++)
			glUniform1i(textureAttribLoc[i], i);
		glUniformMatrix4fv(projMatAttribLoc, 1, GL_FALSE, &projMat[0][0]);
		glBindVertexArray(g_VaoHandle);
	}

	void BindTextures()
	{
		glUseProgram(programHandle);
		for (int i = 0; i < textureCount; i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, textureHandles[i]);
		}
	}

	void Dispose()
	{
		if (vertShaderHandle)
		{
			if (programHandle) glDetachShader(programHandle, vertShaderHandle);
			glDeleteShader(vertShaderHandle);
			vertShaderHandle = 0;
		}
		if (fragShaderHandle)
		{
			if (programHandle) glDetachShader(programHandle, fragShaderHandle);
			glDeleteShader(fragShaderHandle);
			fragShaderHandle = 0;
		}
		if (programHandle)
		{
			glDeleteProgram(programHandle);
			programHandle = 0;
		}
		if (textureCount  > 0)
		{
			for (int i = 0; i < textureCount; i++)
				glDeleteTextures(1, &textureHandles[i]);
			ImGui::GetIO().Fonts->TexID = 0;
			textureCount = 0;
		}
	}
};
