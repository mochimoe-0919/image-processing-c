#include <stdio.h>
#include <stdlib.h>

#define TRUE  1
#define FALSE 0

typedef struct {
    int r; // 輝度値（PBMでは0=黒, 1=白）
} Pixel;

typedef struct Points {
    int x, y;
    struct Points *next;
} Points;

typedef struct {
    int width;
    int height;
    int **data;
} ImageData;

// 指定座標の画素値取得
void getPixel(ImageData *img, int x, int y, Pixel *col)
{
    if (x < 0 || x >= img->width || y < 0 || y >= img->height)
        col->r = 1; // 範囲外は白
    else
        col->r = img->data[y][x];
}

// 8方向移動ベクトル
int drct_x[8] = { -1, 0, 1, 1, 1, 0, -1, -1 };
int drct_y[8] = { -1, -1, -1, 0, 1, 1, 1, 0 };

// 輪郭追跡（座標リスト付き）
Points* traceContour(ImageData *img, int **out, int start_x, int start_y)
{
    int x = start_x, y = start_y;
    int sx = x, sy = y;
    int dist = 0;
    int steps = 0;
    Pixel col;

    Points *head = NULL, *tail = NULL;

    do {
        out[y][x] = 0; // 輪郭を黒に描画

        // 座標をリストに追加
        Points *p = (Points*)malloc(sizeof(Points));
        p->x = x;
        p->y = y;
        p->next = NULL;
        if (!head) head = tail = p;
        else { tail->next = p; tail = p; }

        int found = FALSE;
        for (int i = 0; i < 8; i++) {
            int j = (dist + i) % 8;
            int xx = x + drct_x[j];
            int yy = y + drct_y[j];
            getPixel(img, xx, yy, &col);
            if (col.r == 0) {
                x = xx;
                y = yy;
                dist = (j + 6) % 8; // 次の探索方向
                found = TRUE;
                break;
            }
        }
        if (!found) break;
        if (++steps > img->width * img->height) break; // 安全策
    } while (x != sx || y != sy);

    return head; // 座標リストの先頭
}

int main(void)
{
    FILE *fpi = NULL, *fpo = NULL, *fcoord = NULL;
    char buf[256];
    int width, height;

    char fnamein[] = "zukei.pbm";
    char fnameout[] = "zukei-contours.pbm";
    char coordfile[] = "zukei-contours.csv";

    // --- 入力PBM ---
    fpi = fopen(fnamein, "r");
    if (!fpi) { printf("Error: cannot open %s\n", fnamein); return -1; }

    fgets(buf, sizeof(buf), fpi); // "P1"
    do { fgets(buf, sizeof(buf), fpi); } while (buf[0] == '#');
    if (sscanf(buf, "%d %d", &width, &height) != 2) { printf("Invalid PBM size\n"); return -1; }

    ImageData img;
    img.width = width;
    img.height = height;
    img.data = (int **)malloc(height * sizeof(int *));
    for (int y = 0; y < height; y++) {
        img.data[y] = (int *)malloc(width * sizeof(int));
        for (int x = 0; x < width; x++) {
            if (fscanf(fpi, "%d", &img.data[y][x]) != 1) { printf("Invalid PBM data\n"); return -1; }
        }
    }
    fclose(fpi);

    // --- 出力画像を白で初期化 ---
    int **out = (int **)malloc(height * sizeof(int *));
    for (int y = 0; y < height; y++) {
        out[y] = (int *)malloc(width * sizeof(int));
        for (int x = 0; x < width; x++) out[y][x] = 1;
    }

    // --- 座標リスト出力用 ---
    fcoord = fopen(coordfile, "w");
    if (!fcoord) { printf("Cannot write %s\n", coordfile); return -1; }
    fprintf(fcoord, "x,y\n"); // CSVヘッダ

    // --- 全画素を走査して黒（物体）を探す ---
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            if (img.data[y][x] == 0 && out[y][x] == 1) {
                if (img.data[y-1][x] == 1 || img.data[y+1][x] == 1 ||
                    img.data[y][x-1] == 1 || img.data[y][x+1] == 1) {

                    Points *contour = traceContour(&img, out, x, y);

                    // 座標リストをCSVに出力
                    for (Points *p = contour; p != NULL; p = p->next) {
                        fprintf(fcoord, "%d,%d\n", p->x, p->y);
                    }

                    // メモリ解放
                    Points *p = contour;
                    while (p) {
                        Points *tmp = p;
                        p = p->next;
                        free(tmp);
                    }
                }
            }
        }
    }
    fclose(fcoord);

    // --- PBM出力 ---
    fpo = fopen(fnameout, "w");
    if (!fpo) { printf("Cannot write %s\n", fnameout); return -1; }

    fprintf(fpo, "P1\n");
    fprintf(fpo, "%d %d\n", width, height);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            fprintf(fpo, "%d ", out[y][x] ? 0 : 1);
        }
        fprintf(fpo, "\n");
    }
    fclose(fpo);

    // --- メモリ解放 ---
    for (int y = 0; y < height; y++) {
        free(img.data[y]);
        free(out[y]);
    }
    free(img.data);
    free(out);

    return 0;
}
