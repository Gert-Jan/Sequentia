#pragma once
#include <GL/gl3w.h> 

struct SeqMaterial
{
public:
	SeqMaterial(int textureCount);
	~SeqMaterial();
	void Init(const GLchar *vertShaderSource, const GLchar *fragShaderSource, const GLuint vboHandle, const GLuint elementsHandle);
	void Begin();
	void Dispose();

public:
	GLuint vaoHandle;
	GLuint vertShaderHandle;
	GLuint fragShaderHandle;
	GLuint programHandle;
	GLint textureAttribLoc[3];
	GLint projMatAttribLoc;
	int textureCount;
};