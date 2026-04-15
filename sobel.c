#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_H 1200
#define MAX_W 2000

int INimage[MAX_H][MAX_W];
int OUTimage[MAX_H][MAX_W];

// Sobelフィルタ
int fil1[9] = {
     1, 0, -1,
     2, 0, -2,
     1, 0, -1
};
int fil2[9] = {
     1,  2,  1,
     0,  0,  0,
    -1, -2, -1
};

// P2 PGM安全読み込み用
int read_next_number(FILE *fp)
{
    int n;
    char line[256];
    while (1)
    {
        if (fscanf(fp, "%d", &n) == 1)
            return n;
        if (!fgets(line, sizeof(line), fp))
            break;
    }
    return -1;
}

// Sobel演算（グレースケール用）
void sobel(int width, int height)
{
    for (int y = 1; y < height-1; y++)
    {
        for (int x = 1; x < width-1; x++)
        {
            int gx = 0, gy = 0;
            int s = 0;
            for (int yy = 0; yy < 3; yy++)
                for (int xx = 0; xx < 3; xx++)
                {
                    int val = INimage[y+yy-1][x+xx-1];
                    gx += val * fil1[s];
                    gy += val * fil2[s];
                    s++;
                }
            int g = (int)(sqrt((double)(gx*gx + gy*gy)));
            if (g > 255) g = 255;
            OUTimage[y][x] = g;
        }
    }
}

int main()
{
    FILE *fp_r = fopen("sharp.pgm", "r");
    if (!fp_r) return 1;

    int width  = read_next_number(fp_r);
    int height = read_next_number(fp_r);
    int maxval = read_next_number(fp_r);

    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
        {
            int val = read_next_number(fp_r);
            if (val < 0) val = 0;
            if (val > 255) val = 255;
            INimage[i][j] = val;
            OUTimage[i][j] = 0;
        }
    fclose(fp_r);

    sobel(width, height);

    FILE *fp_w = fopen("output_sobel.pgm", "w");
    fprintf(fp_w, "P2\n%d %d\n255\n", width, height);
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
            fprintf(fp_w, "%d ", OUTimage[i][j]);
        fprintf(fp_w, "\n");
    }
    fclose(fp_w);

   
    return 0;
}
