#pragma once
#include <GL/gl3w.h> 

struct SeqMaterial;

struct SeqMaterialInstance
{
public:
	SeqMaterialInstance(SeqMaterial *material);
	~SeqMaterialInstance();
	void Init(float *projectionMatrix);
	void Begin(unsigned int g_VaoHandle);
	void BindTextures();
	void BindTexture(int index);
	void Dispose();

public:
	SeqMaterial *material;
	GLuint programHandle;
	GLuint textureHandles[3];
	GLint textureAttribLoc[3];
	GLint projMatAttribLoc;
	float *projectionMatrix;
};