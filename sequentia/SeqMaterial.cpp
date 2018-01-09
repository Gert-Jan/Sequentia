#include "SeqMaterial.h"
#include "imgui.h"

SeqMaterial::SeqMaterial(int textureCount):
	vertShaderHandle(0),
	fragShaderHandle(0),
	textureCount(textureCount)
{
}

SeqMaterial::~SeqMaterial()
{
}

void SeqMaterial::Init(const GLchar *vertShaderSource, const GLchar *fragShaderSource)
{
	vertShaderHandle = glCreateShader(GL_VERTEX_SHADER);
	fragShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vertShaderHandle, 1, &vertShaderSource, 0);
	glShaderSource(fragShaderHandle, 1, &fragShaderSource, 0);
	glCompileShader(vertShaderHandle);
	glCompileShader(fragShaderHandle);
}

void SeqMaterial::Dispose()
{
	if (vertShaderHandle)
	{
		glDeleteShader(vertShaderHandle);
		vertShaderHandle = 0;
	}
	if (fragShaderHandle)
	{
		glDeleteShader(fragShaderHandle);
		fragShaderHandle = 0;
	}
}
