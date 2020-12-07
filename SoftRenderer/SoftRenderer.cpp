// SoftRenderer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "tgaimage.h"
#include <iostream>
#include "model.h"
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);
const vec3 light_dir = vec3(0.0, 0.0, -1.0);
const int width = 800;
const int height = 800;

TGAImage *diffuse;

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

//void triangle(vec2 t0, vec2 t1, vec2 t2, TGAImage &image, const TGAColor& color)
//{
//    if (t0.y > t1.y) std::swap(t0, t1);
//    if (t0.y > t2.y) std::swap(t0, t2);
//    if (t1.y > t2.y) std::swap(t1, t2);
//    if (t0.y == t2.y) return;
//    float totalHeight = t2.y - t0.y;
//    float segmentHeight = t1.y - t0.y;
//    for (int y = t0.y; y < t1.y; y++)
//    {
//        float alpha = (y - t0.y) / totalHeight;
//        float beta = (y - t0.y) / segmentHeight;
//
//        line(t0.x + (t2.x - t0.x) * alpha, y, t0.x + (t1.x - t0.x) * beta, y, image, color);
//    }
//    segmentHeight = t2.y - t1.y;
//    for (int y = t1.y; y < t2.y; y++)
//    {
//        float alpha = (y - t0.y) / totalHeight;
//        float beta = (y - t1.y) / segmentHeight;
//
//        line(t0.x + (t2.x - t0.x) * alpha, y, t1.x + (t2.x - t1.x) * beta, y, image, color);
//    }
//}

vec3 barycentric(vec3* pts, vec3 p)
{
    vec3 u = cross(vec3(pts[1][0] - pts[0][0], pts[2][0] - pts[0][0], pts[0][0] - p[0]),
        vec3(pts[1][1] - pts[0][1], pts[2][1] - pts[0][1], pts[0][1] - p[1]));
    if (std::abs(u[2] < 1e-2)) return vec3(-1, 1, 1);
    return vec3(1.0 - (u.x + u.y) / u.z, u.x / u.z, u.y / u.z);
}

vec3 barycentric(vec3 A, vec3 B, vec3 C, vec3 P)
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
void triangle(vec3* pts,vec2*uvs, float *zbuffer, TGAImage& image, TGAColor color)
{
    vec2 bboxmin(image.get_width() - 1, image.get_height() - 1);
    vec2 bboxmax(0, 0);
    vec2 clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            bboxmin[j] = std::max(0.0, std::min(bboxmin[j], pts[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
        }
    }

    vec3 p;
    for (p.x = bboxmin.x; p.x <= bboxmax.x; p.x++)
    {
        for (p.y = bboxmin.y; p.y <= bboxmax.y; p.y++)
        {
            vec3 bc = barycentric(pts, p); //barycentric(pts[0], pts[1],pts[2], p);
            if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue;
            p.z = 0;
            vec2 uv;
            for (int i = 0; i < 3; i++)
            {
                p.z += bc[i] * pts[i][2];
                uv.x += bc[i] * uvs[i].x;
                uv.y += bc[i] * uvs[i].y;
            }

            if (zbuffer[int(p.x + p.y * width)] < p.z)
            {
                TGAColor diffuseColor = diffuse->get((int)(uv.x * diffuse->get_width()), (int)(uv.y * diffuse->get_height()));
                zbuffer[int(p.x + p.y * width)] = p.z;
                image.set(p.x, p.y, diffuseColor);
            }
        }
    }
}

vec3 world2screen(vec3 v) {
    return vec3(int((v.x + 1.) * width / 2. + .5), int((v.y + 1.) * height / 2. + .5), v.z);
}

mat<4, 1> ToHomogeneous(vec3 v)
{
    mat<4, 1> mat;
    mat[0][0] = v.x;
    mat[1][0] = v.y;
    mat[2][0] = v.z;
    mat[3][0] = 1.0f;

    return mat;
}

vec3 FromHomogeneous(mat<4, 1> m)
{
    return vec3(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
}

int main(int argc, char** argv)
{
    Model *model;
    if (argc == 2)
    {
        model = new Model(argv[1]);
    }
    else
    {
        model = new Model("obj/african_head/african_head.obj");
    }
    float *zbuffer = new float[width * height];
    for (int i = width * height; i--; i > 0)
        zbuffer[i] = -std::numeric_limits<float>::max();

    mat<4, 4> perspective = mat<4, 4>::identity();
    perspective[3][2] = -1.0f / 15.0f;
    diffuse = new TGAImage();
    diffuse->read_tga_file("obj/african_head/african_head_diffuse.tga");
    diffuse->flip_vertically();
    TGAImage image(width, height, TGAImage::RGB);
    for (int i = 0; i < model->nfaces(); i++)
    {
        vec3 screen_cords[3];
        vec3 world_coords[3];
        vec2 uvs[3];
        for (int j = 0; j < 3; j++)
        {
            vec3 v = model->vert(i, j);
            uvs[j] = model->uv(i, j);
            mat<4, 1> projected = perspective * ToHomogeneous(v);
            vec3 proejctedV = FromHomogeneous(projected);
            screen_cords[j] = world2screen(proejctedV);
            world_coords[j] = v;
        }
        vec3 normal = cross(world_coords[2] - world_coords[0], world_coords[1] - world_coords[0]);
        normal.normalize();
        float intensitiy = normal* light_dir;
        if (intensitiy > 0)
        {
            triangle(screen_cords, uvs, zbuffer, image, TGAColor(intensitiy * 255, intensitiy * 255, intensitiy * 255, 255));
        }
    }
    //line(vec2(20, 34), vec2(744, 400), image, red);
    //line(vec2(120, 434), vec2(444, 400), image, green);
    //line(vec2(330, 463), vec2(594, 200), image, blue);

    //line(vec2(10, 10), vec2(790, 10), image, white);
    image.write_tga_file("output.tga");
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
