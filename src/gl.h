#pragma once
#include "geometry.h"
#include "tgaimage.h"

extern mat<4, 4> ModelView;
extern mat<4, 4> Projection;
extern mat<4, 4> Viewport;

void lookat(vec3 eye, vec3 center, vec3 up);
void projection(float r);
void viewport(int x, int y, int width, int height, int depth);

class IShader
{
public:
	virtual vec4 vertex(int face, int vertex) = 0;
	virtual bool fragment(vec3 bc, TGAColor &color) = 0;
};

void triangle(vec4* vertices, IShader& shader, TGAImage& colorBuffer, TGAImage& depthBuffer);
