// SoftRenderer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "tgaimage.h"
#include <iostream>
#include "model.h"
#include "gl.h"
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);
const vec3 light_dir = vec3(1.0, 1.0, 1.0);
const int width = 800;
const int height = 800;
const int depth = 255;

vec3 eye(1,1, 3);
vec3 center(0, 0, 0);

Model* model;
TGAImage* gridImage;
//void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color)
//{
//    for (float t = 0; t < 1; t += 0.01)
//    {
//        int x = x0 + (x1 - x0) * t;
//        int y = y0 + (y1 - y0) * t;
//        image.set(x, y, color);
//    }
//}

//void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color)
//{
//    for (int x = x0; x < x1; x++)
//    {
//        float t = ((float)(x - x0) / (x1 - x0));
//        int  y = y0 + (y1 - y0) * t;
//        image.set(x, y, color);
//    }
//}

void line(int x0, int y0, int x1, int y1, TGAImage& image, const TGAColor& color)
{
    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1))
    {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }

    if (x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dx = x1 - x0;
    int dy = y1 - y0;
    int derror = std::abs(dy);
    int error = 0;
    int y = y0;

    for (int x = x0; x < x1; x++)
    {
        float t = (x - x0) / (float)(x1 - x0);
        int y = y0 + (y1 - y0) * t;
        if (steep)
        {
            image.set(y, x, color);
        }
        else
        {
            image.set(x, y, color);
        }

        error += derror;
        if (error > dx)
        {
            y += (y1 > y0) ? 1 : -1;
            error -= dx * 2;
        }
    }
}


void line(vec2 v1, vec2 v2, TGAImage& image, const TGAColor& color)
{
    line(v1.x, v1.y, v2.x, v2.y, image, color);
}

class Shader : public IShader
{
public:
    //vec3 varying_intensities;
    mat<2, 3> varying_uvs;
    mat<3, 3> varing_norm;
    mat<3, 3> varying_ndc;
    vec4 vertex(int face, int vertex) override
    {
        vec3 v = model->vert(face, vertex);
        vec4 gl_vertex = Viewport * Projection * ModelView * embed<4>(v);
        varying_uvs.set_col(vertex, model->uv(face, vertex));
        varing_norm.set_col(vertex, proj<3>((Projection * ModelView).invert_transpose() * embed<4>(model->normal(face, vertex), 0.0f)));
        varying_ndc.set_col(vertex, proj<3>(gl_vertex / gl_vertex[3]));
        return gl_vertex;
    }

    bool fragment(vec3 bc, TGAColor &color) override
    {
        vec2 uv = varying_uvs *bc;
        vec3 bn = (varing_norm * bc).normalize();

        mat<3, 3> A = {varying_ndc.col(1) - varying_ndc.col(0), varying_ndc.col(2) - varying_ndc.col(0), bn };
        mat<3, 3> AI = A.invert();
        vec3 tangent = AI * vec3(varying_uvs[0][1] - varying_uvs[0][0], varying_uvs[0][2] - varying_uvs[0][0], 0.0);
        vec3 bitangent = AI * vec3(varying_uvs[1][1] - varying_uvs[1][0], varying_uvs[1][2] - varying_uvs[1][0], 0.0);

        mat<3, 3> m;
        m.set_col(0, tangent.normalize());
        m.set_col(1, bitangent.normalize());
        m.set_col(2, bn);

        vec3 n = proj<3>(ModelView * embed<4>((m * model->normal(uv)), 0.0)).normalize();
        float diff = std::max(0.0, n * light_dir);
        color = model->diffuse(uv) * diff;

       return false;
    }
};

int main(int argc, char** argv)
{
    if (argc == 2)
    {
        model = new Model(argv[1]);
    }
    else
    {
        model = new Model("obj/african_head/african_head.obj");
    }

    lookat(eye, center, vec3(0, 1, 0));
    projection((eye - center).norm());
    viewport(0, 0, width, height, depth);

    Shader shader;;

    gridImage = new TGAImage();
    gridImage->read_tga_file("obj/grid.tga");
    TGAImage image(width, height, TGAImage::RGB);
    TGAImage depth(width, height, TGAImage::GRAYSCALE);
    for (int i = 0; i < model->nfaces(); i++)
    {
        vec4 screen_cords[3];

        for (int j = 0; j < 3; j++)
        {
            screen_cords[j] = shader.vertex(i, j);

        }
        triangle(screen_cords, shader, image, depth);
    }

    image.write_tga_file("output.tga");
    depth.write_tga_file("depth.tga");
    std::cout << "Hello World!\n";
    return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
