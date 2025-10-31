#define STB_IMAGE_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>
#include "stb_image.h"
#include "detection.h"


static void save_ppm(const char *filename, const unsigned char *data, int w, int h, int c) // Saved cell
{
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int i = 0; i < w * h; ++i)
    {
        const unsigned char *p = &data[i * c];
        unsigned char rgb[3] = { p[0], (c > 1 ? p[1] : p[0]), (c > 2 ? p[2] : p[0]) };
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}


static void find_content_box(const unsigned char *img, int W, int H, int C,int *x0, int *y0, int *x1, int *y1) // Cut useless part
{
    int minx = W;
    int miny = H;
    int maxx = -1;
    int maxy = -1;
    for (int y = 0; y < H; ++y)
    {
        const unsigned char *row = &img[y * W * C];
        for (int x = 0; x < W; ++x) 
        {
            const unsigned char *p = &row[x * C];
            int lum = (int)p[0] + (C > 1 ? p[1] : p[0]) + (C > 2 ? p[2] : p[0]);
            if (lum < 3 * 240)
            {
                if (x < minx) minx = x;
                if (y < miny) miny = y;
                if (x > maxx) maxx = x;
                if (y > maxy) maxy = y;
            }
        }
    }

    if (maxx < 0)
    {
        *x0 = 0; *y0 = 0; *x1 = W; *y1 = H;
        return;
    }
    int inset = 1;
    *x0 = (minx + inset < W ? minx + inset : minx);
    *y0 = (miny + inset < H ? miny + inset : miny);
    *x1 = (maxx - inset + 1 > 0 ? maxx - inset + 1 : maxx + 1);
    *y1 = (maxy - inset + 1 > 0 ? maxy - inset + 1 : maxy + 1);
}


void detect_and_cut(const char *path, int rows, int cols) // 
{
    int W, H, Corig;
    unsigned char *img = stbi_load(path, &W, &H, &Corig, 3);
    int C = 3;
    if (!img)
    {
        printf("Cant load the image : %s\n", path);
        return;
    }
    printf("Image : %s (%dx%d, %d canaux)\n", path, W, H, C);
    int gx0;
    int gy0;
    int gx1;
    int gy1;
    find_content_box(img, W, H, C, &gx0, &gy0, &gx1, &gy1);
    int GW = gx1 - gx0;
    int GH = gy1 - gy0;
    if (GW <= 0 || GH <= 0)
    {
        stbi_image_free(img);
        return;
    }

   
    #ifdef _WIN32     // Create the folder ./cells
    mkdir("cells");
    #else
    mkdir("cells", 0777);
    #endif
    
    double step_x = (double)GW / cols;
    double step_y = (double)GH / rows;
    int count = 0;
    int *x_cord = malloc((cols + 1) * sizeof(int));
    int *y_cord = malloc((rows + 1) * sizeof(int));
    if (!x_cord || !y_cord)
    {
       printf("Ereur : allocation.\n");
       stbi_image_free(img);
       return;
    }
    for (int c_ = 0; c_ <= cols; ++c_)
        x_cord[c_] = gx0 + (int)round(c_ * step_x);
    
    for (int r = 0; r <= rows; ++r)
        y_cord[r] = gy0 + (int)round(r * step_y);
    x_cord[cols] = gx1;
    y_cord[rows] = gy1;

    for (int r = 0; r < rows; ++r)
    {
       int y0 = y_cord[r];
       int y1 = y_cord[r + 1];
       int CH = y1 - y0;

       for (int c_ = 0; c_ < cols; ++c_)
       {
            int x0 = x_cord[c_];
            int x1 = x_cord[c_ + 1];
            int CW = x1 - x0;

            if (CW <= 0 || CH <= 0) continue;
            unsigned char *cell = malloc((size_t)CW * CH * C);
            if (!cell) 
            {
                printf("No memory.\n");
                free(x_cord); free(y_cord);
                stbi_image_free(img);
                return;
            }
            const unsigned char *src = &img[(y0 * W + x0) * C];
            for (int yy = 0; yy < CH; ++yy)
            {
                memcpy(&cell[yy * CW * C], &src[yy * W * C], (size_t)CW * C);
            }

            char filename[64];
            sprintf(filename, "cells/cell_%02d_%02d.ppm", r, c_);
            save_ppm(filename, cell, CW, CH, C);
            free(cell);
            count+=1;
       }
    }

    free(x_cord);
    free(y_cord);
    printf("%d cells saved in ./cells\n", count);
}
int main()
{
    detect_and_cut("grid_test1.png", 12, 12);
    return 0;
}