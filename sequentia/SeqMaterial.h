#pragma once
#include <GL/gl3w.h> 

struct SeqMaterial
{
public:
	SeqMaterial();
	~SeqMaterial();
	void Init(const GLchar *vertShaderSource, const GLchar *fragShaderSource, int textureCount);
	void Begin(const float projMat[4][4], unsigned int g_VaoHandle);
	void BindTextures();
	void Dispose();

public:
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
};