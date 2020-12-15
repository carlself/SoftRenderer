#include "gl.h"
#include <utility>

mat<4, 4> ModelView;
mat<4, 4> Projection;
mat<4, 4> Viewport;

void lookat(vec3 eye, vec3 center, vec3 up)
{
    vec3 z = (eye - center).normalize();
    vec3 x = cross(up, z).normalize();
    vec3 y = cross(z, x).normalize();
    mat<4, 4> m = mat<4, 4>::identity();
    for (int i = 0; i < 3; i++)
    {
        m[0][i] = x[i];
        m[1][i] = y[i];
        m[2][i] = z[i];
        m[i][3] = -center[i];
    }

    ModelView = m;
}

void projection(float r)
{
    mat<4, 4> m = mat<4, 4>::identity();
    m[3][2] = -1.0f / r;
    Projection = m;
}

void viewport(int x, int y, int width, int height, int depth)
{
    mat<4, 4> m = mat<4, 4>::identity();

    m[0][3] = x + width / 2.0f;
    m[1][3] = y + height / 2.0f;
    m[2][3] = depth / 2.0;

    m[0][0] = width / 2.0f;
    m[1][1] = height / 2.0f;
    m[2][2] = depth / 2.0;

    Viewport = m;
}

//vec3 barycentric(vec3* pts, vec3 p)
//{
//    vec3 u = cross(vec3(pts[1][0] - pts[0][0], pts[2][0] - pts[0][0], pts[0][0] - p[0]),
//        vec3(pts[1][1] - pts[0][1], pts[2][1] - pts[0][1], pts[0][1] - p[1]));
//    if (std::abs(u[2] < 1e-2)) return vec3(-1, 1, 1);
//    return vec3(1.0 - (u.x + u.y) / u.z, u.x / u.z, u.y / u.z);
//}


vec3 barycentric(vec2 A, vec2 B, vec2 C, vec2 P)
{
    vec3 s[2];
    for (int i = 2; i--; ) {
        s[i][0] = C[i] - A[i];
        s[i][1] = B[i] - A[i];
        s[i][2] = A[i] - P[i];
    }
    vec3 u = cross(s[0], s[1]);
    if (std::abs(u[2]) > 1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
        return vec3(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
    return vec3(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
}

void triangle(vec4* pts, IShader& shader, TGAImage& colorBuffer, TGAImage& depthBuffer)
{
    vec2 bboxmin(colorBuffer.get_width() - 1, colorBuffer.get_height() - 1);
    vec2 bboxmax(0, 0);
    vec2 clamp(colorBuffer.get_width() - 1, colorBuffer.get_height() - 1);
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            double d = (int)(pts[i][j] / pts[i][3]);
            bboxmin[j] = std::max(0.0, std::min(bboxmin[j], d));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], d));
        }
    }

    vec2 p;
    TGAColor color;
    for (p.x = bboxmin.x; p.x <= bboxmax.x; p.x++)
    {
        for (p.y = bboxmin.y; p.y <= bboxmax.y; p.y++)
        {
            vec3 bc = barycentric(proj<2>(pts[0]/pts[0][3]), proj<2>(pts[1] / pts[1][3]),
                proj<2>(pts[2] / pts[2][3]), p);
            float z = pts[0][2] * bc.x + pts[1][2] * bc.y + pts[2][2] * bc.z;

            if (bc.x < 0 || bc.y < 0 || bc.z < 0 || depthBuffer.get(p.x, p.y)[0] > z) continue;
            bool discard = shader.fragment(bc, color);
            if (!discard)
            {
                depthBuffer.set(p.x, p.y, TGAColor(z));
                colorBuffer.set(p.x, p.y, color);
            }            
        }
    }
}