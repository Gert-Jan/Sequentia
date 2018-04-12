#include "SeqMaterialInstance.h";
#include "SeqMaterial.h"
#include "SeqString.h"
#include "imgui.h"

SeqMaterialInstance::SeqMaterialInstance(SeqMaterial *material):
	material(material)
{
}

SeqMaterialInstance::~SeqMaterialInstance()
{
}

void SeqMaterialInstance::Init(float *projectionMatrix)
{
	this->projectionMatrix = projectionMatrix;

	glGenTextures(material->textureCount, &textureHandles[0]);

	programHandle = glCreateProgram();
	glAttachShader(programHandle, material->vertShaderHandle);
	glAttachShader(programHandle, material->fragShaderHandle);
	glLinkProgram(programHandle);

	projMatAttribLoc = glGetUniformLocation(programHandle, "ProjMtx");
	GLint positionAttribLoc = glGetAttribLocation(programHandle, "Position");
	GLint uvAttribLoc = glGetAttribLocation(programHandle, "UV");
	GLint colorAttribLoc = glGetAttribLocation(programHandle, "Color");

	glEnableVertexAttribArray(positionAttribLoc);
	glEnableVertexAttribArray(uvAttribLoc);
	glEnableVertexAttribArray(colorAttribLoc);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
	glVertexAttribPointer(positionAttribLoc, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
	glVertexAttribPointer(uvAttribLoc, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
	glVertexAttribPointer(colorAttribLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF

	for (int i = 0; i < material->textureCount; i++)
	{
		SeqString::Temp->Format("Texture%d", i);
		textureAttribLoc[i] = glGetUniformLocation(programHandle, SeqString::Temp->Buffer);
	}
}

void SeqMaterialInstance::Begin(unsigned int g_VaoHandle)
{
	glUseProgram(programHandle);

	for (int i = 0; i < material->textureCount; i++)
		glUniform1i(textureAttribLoc[i], i);

	glUniformMatrix4fv(projMatAttribLoc, 1, GL_FALSE, projectionMatrix);
	glBindVertexArray(g_VaoHandle);
}

void SeqMaterialInstance::BindTextures()
{
	glUseProgram(programHandle);
	for (int i = 0; i < material->textureCount; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, textureHandles[i]);
	}
}

void SeqMaterialInstance::BindTexture(int index)
{
	glUseProgram(programHandle);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureHandles[index]);
}

void SeqMaterialInstance::Dispose()
{
	if (programHandle)
	{
		if (material->vertShaderHandle) glDetachShader(programHandle, material->vertShaderHandle);
		if (material->fragShaderHandle) glDetachShader(programHandle, material->fragShaderHandle);
		glDeleteProgram(programHandle);
		programHandle = 0;
	}
	if (material->textureCount > 0)
	{
		glDeleteTextures(material->textureCount, &textureHandles[0]);
	}
}
