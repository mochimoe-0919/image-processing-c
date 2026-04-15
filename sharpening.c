#include <stdio.h>
#include <stdlib.h>

#define MAX_H 1200
#define MAX_W 2000

int INimage[MAX_H][MAX_W];
int OUTimage[MAX_H][MAX_W];

void Filter(int filter[8][8], int fx, int fy, float fc, int ixm, int iym)
{
    int i, j, k, l, w;
    float wg;

    for (i = 0; i < iym; i++)
        for (j = 0; j < ixm; j++)
            OUTimage[i][j] = 0;

    for (i = 0; i < (iym - fy + 1); i++)
        for (j = 0; j < (ixm - fx + 1); j++)
        {
            w = 0;
            for (k = 0; k < fy; k++)
                for (l = 0; l < fx; l++)
                    w += filter[k][l] * INimage[i + k][j + l];
            wg = (float)w * fc;
            w = (int)(wg + 0.5);
            if (w > 255) w = 255;
            if (w < 0) w = 0;
            OUTimage[i + fy / 2][j + fx / 2] = w;
        }
}

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

int main()
{
    FILE *fp_r = fopen("image2.pgm", "r");
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
        }
    fclose(fp_r);

    // --- シャープ化フィルタ ---
    int fx = 3, fy = 3;
    float fc = 1.0;
    int filter[8][8] = {0};
    filter[0][1] = -1; filter[1][0] = -1;
    filter[1][1] = 5;
    filter[1][2] = -1; filter[2][1] = -1;

    Filter(filter, fx, fy, fc, width, height);

    FILE *fp_w = fopen("output.pgm", "w");
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
