#include "SeqMaterial.h";
#include "SeqString.h";
#include "imgui.h"

SeqMaterial::SeqMaterial() :
	programHandle(0),
	vertShaderHandle(0),
	fragShaderHandle(0),
	textureCount(0),
	projMatAttribLoc(0),
	uvAttribLoc(0),
	colorAttribLoc(0)
{
}

SeqMaterial::~SeqMaterial()
{
}

void SeqMaterial::Init(const GLchar* vertShaderSource, const GLchar* fragShaderSource, int textureCount)
{
	this->textureCount = textureCount;
	glGenTextures(textureCount, &textureHandles[0]);

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

	for (int i = 0; i < textureCount; i++)
	{
		SeqString::FormatBuffer("Texture%d", i);
		textureAttribLoc[i] = glGetUniformLocation(programHandle, SeqString::Buffer);
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

void SeqMaterial::Begin(const float projMat[4][4], unsigned int g_VaoHandle)
{
	glUseProgram(programHandle);

	for (int i = 0; i < textureCount; i++)
		glUniform1i(textureAttribLoc[i], i);
	glUniformMatrix4fv(projMatAttribLoc, 1, GL_FALSE, &projMat[0][0]);
	glBindVertexArray(g_VaoHandle);
}

void SeqMaterial::BindTextures()
{
	glUseProgram(programHandle);
	for (int i = 0; i < textureCount; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, textureHandles[i]);
	}
}

void SeqMaterial::Dispose()
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
		glDeleteTextures(textureCount, &textureHandles[0]);
		ImGui::GetIO().Fonts->TexID = 0;
		textureCount = 0;
	}
};