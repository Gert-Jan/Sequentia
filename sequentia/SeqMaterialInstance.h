#pragma once
#include <GL/gl3w.h> 

struct SeqMaterial;

struct SeqMaterialInstance
{
public:
	SeqMaterialInstance(SeqMaterial *material);
	~SeqMaterialInstance();
	void Init(float *projectionMatrix);
	void Begin();
	void CreateTexture(int index, GLint width, GLint height, GLint format, const GLvoid *pixels);
	void BindTextures();
	void BindTexture(int index);
	void Dispose();

public:
	SeqMaterial *material;
	GLuint textureHandles[3];
	float *projectionMatrix;
};