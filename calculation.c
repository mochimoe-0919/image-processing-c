#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

//  Moore近傍法（境界限定）で輪郭追跡 
Points* traceContour(int **mask, int **out, int width, int height, 
    int sx, int sy, double *perimeter) {
    int cx = sx, cy = sy;
    int dir = 7;
    int startx = sx, starty = sy;
    int first = 1;

    Points *head = NULL, *tail = NULL;
    *perimeter = 0.0;

    while (1) {
        mask[cy][cx] = 2;
        out[cy][cx] = 0;

        Points *p = (Points*)malloc(sizeof(Points));
        p->x = cx; p->y = cy; p->next = NULL;
        if (!head) head = tail = p;
        else { tail->next = p; tail = p; }

        int found = 0;
        for (int i = 0; i < 8; i++) {
            int ndir = (dir + i) % 8;
            int nx = cx + drct_x[ndir];
            int ny = cy + drct_y[ndir];
            if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue;

            // 0（対象）で、かつ隣接に1（背景）がある＝境界画素
            if (mask[ny][nx] == 0) {
                int is_border = 0;
                for (int dy=-1; dy<=1; dy++)
                    for (int dx=-1; dx<=1; dx++)
                        if (dx || dy)
                            if (getPixel(mask,width,height,nx+dx,ny+dy)==1)
                                is_border = 1;

                if (!is_border) continue; // 内部ピクセルは無視

                double step 
                = (abs(drct_x[ndir]) + abs(drct_y[ndir]) == 2) ? 1.41421356 : 1.0;
                *perimeter += step;

                cx = nx;
                cy = ny;
                dir = (ndir + 6) % 8;
                found = 1;
                break;
            }
        }

        if (!found) break;
        if (!first && cx == startx && cy == starty) break;
        first = 0;
    }

    return head;
}

int main(void) {
    FILE *fpi=NULL, *fpo=NULL, *fcoord=NULL;
    char buf[256];
    int width,height, maxval;

    char fnamein[]="labeled.pgm";
    char fnameout[]="labeled-out.pgm";
    char coordfile[]="object_features.csv";

    // --- PGM読み込み ---
    fpi = fopen(fnamein,"rb");
    if(!fpi){ printf("Cannot open %s\n",fnamein); return -1; }

    fgets(buf,sizeof(buf),fpi);
    if(buf[0]!='P' || buf[1]!='5'){ fclose(fpi); printf("Not PGM (binary P5) format!\n");
         return -1; }

    // コメント行スキップ
    do { fgets(buf,sizeof(buf),fpi); } while(buf[0]=='#');
    sscanf(buf,"%d %d",&width,&height);

    fgets(buf,sizeof(buf),fpi);
    sscanf(buf,"%d",&maxval);

    unsigned char *data = (unsigned char*)malloc(width*height);
    fread(data,1,width*height,fpi);
    fclose(fpi);

    // 2次元配列 
    int **img = (int**)malloc(height*sizeof(int*));
    for(int y=0;y<height;y++){
        img[y]=(int*)malloc(width*sizeof(int));
        for(int x=0;x<width;x++){
            img[y][x] = (int)data[y*width + x];
        }
    }
    free(data);

    // 出力画像白で初期化
    int **out = (int**)malloc(height*sizeof(int*));
    for(int y=0;y<height;y++){
        out[y]=(int*)malloc(width*sizeof(int));
        for(int x=0;x<width;x++) out[y][x]=maxval;
    }

    // CSV出力 
    fcoord = fopen(coordfile,"w");
    if(!fcoord){ printf("Cannot write %s\n",coordfile); return -1; }
    fprintf(fcoord,"ObjectID,CenterX,CenterY,Area,ContourLength,Circularity\n");

    // 最大ラベル 
    int maxLabel = 0;
    for(int y=0;y<height;y++)
        for(int x=0;x<width;x++)
            if(img[y][x] > maxLabel) maxLabel = img[y][x];

    // 各ラベル処理 
    for(int label=1; label<=maxLabel; label++){
        int area = 0;
        long sumx=0, sumy=0;

        // 面積と重心 
        for(int y=0;y<height;y++){
            for(int x=0;x<width;x++){
                if(img[y][x]==label){
                    area++;
                    sumx += x;
                    sumy += y;
                }
            }
        }

        if(area==0) continue;

        double cx = (double)sumx / area;
        double cy = (double)sumy / area;

        // --- バイナリマスク作成 ---
        int **mask = (int**)malloc(height*sizeof(int*));
        for(int y=0;y<height;y++){
            mask[y]=(int*)malloc(width*sizeof(int));
            for(int x=0;x<width;x++){
                mask[y][x] = (img[y][x]==label)?0:1;
            }
        }

        double perimeter = 0.0;
        Points *contour = NULL;

        // 輪郭探索開始点を探す 
        for(int y=1;y<height-1 && !contour;y++){
            for(int x=1;x<width-1;x++){
                if(mask[y][x]==0){
                    int border=0;
                    for(int dy=-1;dy<=1;dy++)
                        for(int dx=-1;dx<=1;dx++)
                            if(dx||dy)
                                if(getPixel(mask,width,height,x+dx,y+dy)==1)
                                    border=1;
                    if(border){
                        contour 
                        = traceContour(mask, out, width, height, x, y, &perimeter);
                        break;
                    }
                }
            }
        }

        // 円形度 
        double circularity = (perimeter==0)?0.0:(4.0*M_PI*area)/(perimeter*perimeter);

        fprintf(fcoord,"%d,%.2f,%.2f,%d,%.2f,%.4f\n",
            label,cx,cy,area,perimeter,circularity);

        // --- メモリ解放 ---
        Points *p=contour;
        while(p){Points *tmp=p; p=p->next; free(tmp);}
        for(int y=0;y<height;y++) free(mask[y]);
        free(mask);
    }

    fclose(fcoord);

    // 出力画像 
    fpo=fopen(fnameout,"wb");
    fprintf(fpo,"P5\n%d %d\n%d\n",width,height,maxval);
    unsigned char *outdata = (unsigned char*)malloc(width*height);
    for(int y=0;y<height;y++)
        for(int x=0;x<width;x++)
            outdata[y*width+x] = (unsigned char)out[y][x];
    fwrite(outdata,1,width*height,fpo);
    fclose(fpo);
    free(outdata);

    for(int y=0;y<height;y++){free(img[y]); free(out[y]);}
    free(img); free(out);
    
    return 0;
}
