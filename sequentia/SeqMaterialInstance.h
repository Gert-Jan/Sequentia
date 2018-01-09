#pragma once
#include <GL/gl3w.h> 

struct SeqMaterial;

struct SeqMaterialInstance
{
public:
	SeqMaterialInstance(SeqMaterial *material);
	~SeqMaterialInstance();
	void Init();
	void Begin(const float projMat[4][4], unsigned int g_VaoHandle);
	void BindTextures();
	void Dispose();

public:
	SeqMaterial *material;
	GLuint programHandle;
	GLuint textureHandles[3];
	GLint textureAttribLoc[3];
	GLint projMatAttribLoc;
};