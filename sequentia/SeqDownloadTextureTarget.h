#pragma once
#include <GL/gl3w.h>

struct SeqMaterialInstance;

struct SeqDownloadTextureTarget
{
	SeqMaterialInstance *source;
	GLsizei width;
	GLsizei height;
	GLenum format;
	GLvoid *destination;
};
