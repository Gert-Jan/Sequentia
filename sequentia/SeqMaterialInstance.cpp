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
}

void SeqMaterialInstance::CreateTexture(int index, GLint width, GLint height, GLint format, const GLvoid *pixels)
{
	glBindTexture(GL_TEXTURE_2D, textureHandles[index]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
}

void SeqMaterialInstance::Begin()
{
	if (material->programHandle > 0)
	{
		glUseProgram(material->programHandle);
		glUniformMatrix4fv(material->projMatAttribLoc, 1, GL_FALSE, projectionMatrix);
	}
}

void SeqMaterialInstance::BindTextures()
{
	material->Begin();
	for (int i = 0; i < material->textureCount; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, textureHandles[i]);
	}
}

void SeqMaterialInstance::BindTexture(int index)
{
	material->Begin();
	glActiveTexture(GL_TEXTURE0 + index);
	glBindTexture(GL_TEXTURE_2D, textureHandles[index]);
}

void SeqMaterialInstance::Dispose()
{
	glDeleteTextures(material->textureCount, &textureHandles[0]);
}
