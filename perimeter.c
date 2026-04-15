#include <stdio.h>
#include <stdlib.h>

typedef struct Points {
    int x, y;
    struct Points *next;
} Points;

// 8方向（反時計回り）
int drct_x[8] = {1, 1, 0, -1, -1, -1, 0, 1};
int drct_y[8] = {0, -1, -1, -1, 0, 1, 1, 1};

// 画素取得
int getPixel(int **img, int width, int height, int x, int y) {
    if (x < 0 || x >= width || y < 0 || y >= height) return 1;
    return img[y][x];
}

// Moore近傍法で輪郭追跡
Points* traceContour(int **img, int **out, int width, int height, int sx, int sy, int *length) {
    int cx = sx, cy = sy;
    int dir = 0;
    int steps = 0;
    *length = 0;

    Points *head = NULL, *tail = NULL;

    do {
        img[cy][cx] = 2;  // 追跡済みとしてマーク
        out[cy][cx] = 0;  // 出力画像に描画

        Points *p = (Points*)malloc(sizeof(Points));
        p->x = cx; p->y = cy; p->next = NULL;
        if (!head) head = tail = p;
        else { tail->next = p; tail = p; }

        (*length)++;

        int found = 0;
        for (int i = 0; i < 8; i++) {
            int ndir = (dir + i) % 8;
            int nx = cx + drct_x[ndir];
            int ny = cy + drct_y[ndir];

            if (getPixel(img, width, height, nx, ny) == 0) {
                cx = nx;
                cy = ny;
                dir = (ndir + 6) % 8;
                found = 1;
                break;
            }
        }

        if (!found) break;
        if (++steps > width * height * 4) break; // 無限ループ防止
    } while (!(cx == sx && cy == sy));

    return head;
}

int main(void) {
    FILE *fpi=NULL, *fpo=NULL, *fcoord=NULL;
    char buf[256];
    int width,height;

    char fnamein[]="zukei.pbm";
    char fnameout[]="zukei-out.pbm";
    char coordfile[]="zukei-contours.csv";

    // --- PBM読み込み ---
    fpi = fopen(fnamein,"r");
    if(!fpi){ printf("Cannot open %s\n",fnamein); return -1; }

    fgets(buf,sizeof(buf),fpi);
    if(buf[0]!='P' || buf[1]!='1'){ fclose(fpi); printf("Not PBM format!\n"); return -1; }

    do{ fgets(buf,sizeof(buf),fpi);} while(buf[0]=='#');
    sscanf(buf,"%d %d",&width,&height);

    int **img = (int**)malloc(height*sizeof(int*));
    for(int y=0;y<height;y++){
        img[y]=(int*)malloc(width*sizeof(int));
        for(int x=0;x<width;x++) fscanf(fpi,"%d",&img[y][x]);
    }
    fclose(fpi);

    // --- 出力画像白で初期化 ---
    int **out = (int**)malloc(height*sizeof(int*));
    for(int y=0;y<height;y++){
        out[y]=(int*)malloc(width*sizeof(int));
        for(int x=0;x<width;x++) out[y][x]=1;
    }

    // --- CSV出力 ---
    fcoord = fopen(coordfile,"w");
    if(!fcoord){ printf("Cannot write %s\n",coordfile); return -1; }
    fprintf(fcoord,"ObjectID,ContourLength\n");

    int objectID=1;

    // --- 輪郭追跡 ---
    for(int y=1;y<height-1;y++){
        for(int x=1;x<width-1;x++){
            if(img[y][x]==0){ // 黒画素（未処理）
                int border=0;
                for(int dy=-1;dy<=1;dy++){
                    for(int dx=-1;dx<=1;dx++){
                        if(dx==0 && dy==0) continue;
                        if(getPixel(img,width,height,x+dx,y+dy)==1)
                            border=1;
                    }
                }
                if(border){
                    int length=0;
                    Points *contour = traceContour(img, out, width, height, x, y, &length);

                    // --- 追跡済み画素を再訪問防止としてマーク ---
                    for(Points *p=contour; p!=NULL; p=p->next){
                        img[p->y][p->x] = 2;
                    }

                    // --- フィルタ：大きな輪郭のみ出力 ---
                    if(length > 1000){
                        fprintf(fcoord,"%d,%d\n",objectID++,length);
                    }

                    // --- メモリ解放 ---
                    Points *p = contour;
                    while(p){Points *tmp=p; p=p->next; free(tmp);}
                }
            }
        }
    }
    fclose(fcoord);

    // --- 出力 ---
    fpo=fopen(fnameout,"w");
    fprintf(fpo,"P1\n%d %d\n",width,height);
    for(int y=0;y<height;y++){
        for(int x=0;x<width;x++)
            fprintf(fpo,"%d ",out[y][x]);
        fprintf(fpo,"\n");
    }
    fclose(fpo);

    for(int y=0;y<height;y++){free(img[y]);free(out[y]);}
    free(img);free(out);

    printf("輪郭追跡完了！出力ファイル: %s\n", coordfile);
    return 0;
}
