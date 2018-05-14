#include "SeqMaterial.h"
#include "SeqString.h"
#include "imgui.h"
#include "stdio.h"

SeqMaterial::SeqMaterial(int textureCount):
	vaoHandle(0),
	vertShaderHandle(0),
	fragShaderHandle(0),
	programHandle(0),
	projMatAttribLoc(0),
	textureAttribLoc{ 0, 0, 0 },
	textureCount(textureCount)
{
}

SeqMaterial::~SeqMaterial()
{
}

void SeqMaterial::Init(const GLchar *vertShaderSource, const GLchar *fragShaderSource, const GLuint vboHandle, const GLuint elementsHandle)
{
	glGenVertexArrays(1, &vaoHandle);
	glBindVertexArray(vaoHandle);
	glBindBuffer(GL_ARRAY_BUFFER, vboHandle);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementsHandle);

	vertShaderHandle = glCreateShader(GL_VERTEX_SHADER);
	fragShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vertShaderHandle, 1, &vertShaderSource, 0);
	glShaderSource(fragShaderHandle, 1, &fragShaderSource, 0);
	glCompileShader(vertShaderHandle);
	glCompileShader(fragShaderHandle);

	if (vertShaderHandle > 0 && fragShaderHandle > 0)
	{
		programHandle = glCreateProgram();
		glAttachShader(programHandle, vertShaderHandle);
		glAttachShader(programHandle, fragShaderHandle);
		glLinkProgram(programHandle);

		GLint isLinked = 0;
		glGetProgramiv(programHandle, GL_LINK_STATUS, &isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(programHandle, GL_INFO_LOG_LENGTH, &maxLength);
			GLchar *log = new GLchar[maxLength];
			glGetProgramInfoLog(programHandle, maxLength, &maxLength, log);
			printf("GL Linking error: %s\n", log);
		}

		projMatAttribLoc = glGetUniformLocation(programHandle, "ProjMtx");

		GLint positionAttribLoc = glGetAttribLocation(programHandle, "Position");
		GLint uvAttribLoc = glGetAttribLocation(programHandle, "UV");
		GLint colorAttribLoc = glGetAttribLocation(programHandle, "Color");

		glEnableVertexAttribArray(positionAttribLoc);
		if (uvAttribLoc > -1)
			glEnableVertexAttribArray(uvAttribLoc);
		if (colorAttribLoc > -1)
			glEnableVertexAttribArray(colorAttribLoc);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
		glVertexAttribPointer(positionAttribLoc, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
		if (uvAttribLoc > -1)
			glVertexAttribPointer(uvAttribLoc, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
		if (colorAttribLoc > -1)
			glVertexAttribPointer(colorAttribLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF

		for (int i = 0; i < textureCount; i++)
		{
			SeqString::Temp->Format("Texture%d", i);
			textureAttribLoc[i] = glGetUniformLocation(programHandle, SeqString::Temp->Buffer);
		}

		// bind the program's samplers to the first number of texture units.
		glUseProgram(programHandle);
		for (int i = 0; i < textureCount; i++)
		{
			glUniform1i(textureAttribLoc[i], i);
		}
	}
}

void SeqMaterial::Begin()
{
	if (programHandle > 0)
	{
		glUseProgram(programHandle);
		glBindVertexArray(vaoHandle);
	}
}

void SeqMaterial::Dispose()
{
	if (vaoHandle)
	{
		glDeleteVertexArrays(1, &vaoHandle);
	}
	if (programHandle)
	{
		if (vertShaderHandle) glDetachShader(programHandle, vertShaderHandle);
		if (fragShaderHandle) glDetachShader(programHandle, fragShaderHandle);
		glDeleteProgram(programHandle);
		programHandle = 0;
	}
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
