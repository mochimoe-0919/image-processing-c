#include <stdio.h>
#include <stdlib.h>

#define MAX_H 1200
#define MAX_W 2000

int INimage[MAX_H][MAX_W];
int OUTimage[MAX_H][MAX_W];

// --- 平滑化フィルタ関数 ---
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

// --- P2 PGM安全読み込み用関数 ---
int read_next_number(FILE *fp)
{
    int n;
    char line[256];
    while (1)
    {
        if (fscanf(fp, "%d", &n) == 1)
            return n;
        // 数字でなければ1行読み飛ばす（コメントや空白行）
        if (!fgets(line, sizeof(line), fp))
            break;
    }
    return -1; // 読み取り失敗
}

int main()
{
    FILE *fp_r, *fp_w;
    int width, height, maxval;
    char header[3];
    int i, j;

    // 入力 
    fp_r = fopen("image2.pgm", "r");
    if (!fp_r)
    {
        printf("image2.pgm が開けませんでした。\n");
        return 1;
    }

    width  = read_next_number(fp_r);
    height = read_next_number(fp_r);
    maxval = read_next_number(fp_r);


    // PGMデータ読み込み
    for (i = 0; i < height; i++)
        for (j = 0; j < width; j++)
        {
            int val = read_next_number(fp_r);
            if (val < 0) val = 0;
            if (val > 255) val = 255;
            INimage[i][j] = val;
        }
    fclose(fp_r);

    // 平滑化フィルタ設定 
    int fx = 3, fy = 3;
    float fc = 1.0 / 9.0;
    int filter[8][8];
    for (i = 0; i < fy; i++)
        for (j = 0; j < fx; j++)
            filter[i][j] = 1;

    Filter(filter, fx, fy, fc, width, height);

    // 出力
    fp_w = fopen("output.pgm", "w");
    fprintf(fp_w, "P2\n%d %d\n255\n", width, height);
    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
            fprintf(fp_w, "%d ", OUTimage[i][j]);
        fprintf(fp_w, "\n");
    }
    fclose(fp_w);

  
    return 0;
}
