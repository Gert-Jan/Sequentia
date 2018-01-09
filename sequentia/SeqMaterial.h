#pragma once
#include <GL/gl3w.h> 

struct SeqMaterial
{
public:
	SeqMaterial(int textureCount);
	~SeqMaterial();
	void Init(const GLchar *vertShaderSource, const GLchar *fragShaderSource);
	void Dispose();

public:
	GLuint vertShaderHandle;
	GLuint fragShaderHandle;
	int textureCount;
};