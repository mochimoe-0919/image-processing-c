#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// 画素取得 
int getPixel(int **img, int width, int height, int x, int y) {
    if (x < 0 || x >= width || y < 0 || y >= height) return 1;
    return img[y][x];
}

// 周囲長（8近傍で距離補正付き） 
double calcPerimeter8(int **mask, int width, int height) {
    double perim = 0.0;
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            if (mask[y][x] == 0) {
                // 8方向の境界チェック
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        if (getPixel(mask, width, height, x + dx, y + dy) == 1) {
                            // 斜め移動は √2、それ以外は 1
                            if (abs(dx) + abs(dy) == 2)
                                perim += 1.414213562;
                            else
                                perim += 1.0;
                        }
                    }
                }
            }
        }
    }
    // 周囲の各境界を二重に数えているため2で割る
    return perim / 2.0;
}

int main(void) {
    FILE *fpi=NULL, *fpo=NULL, *fcoord=NULL;
    char buf[256];
    int width,height, maxval;

    char fnamein[]="labeled.pgm";
    char fnameout[]="most_circular.pgm";
    char coordfile[]="object_features.csv";

    // PGM読み込み 
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

    // CSV出力 
    fcoord = fopen(coordfile,"w");
    if(!fcoord){ printf("Cannot write %s\n",coordfile); return -1; }
    fprintf(fcoord,"ObjectID,CenterX,CenterY,Area,ContourLength,Circularity\n");

    // 最大ラベル取得 
    int maxLabel = 0;
    for(int y=0;y<height;y++)
        for(int x=0;x<width;x++)
            if(img[y][x] > maxLabel) maxLabel = img[y][x];

    // 特徴計算 
    double best_circ = -1.0;
    int best_label = -1;

    for(int label=1; label<=maxLabel; label++){
        int area = 0;
        long sumx=0, sumy=0;

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

        // バイナリマスク生成 
        int **mask = (int**)malloc(height*sizeof(int*));
        for(int y=0;y<height;y++){
            mask[y]=(int*)malloc(width*sizeof(int));
            for(int x=0;x<width;x++){
                mask[y][x] = (img[y][x]==label)?0:1;
            }
        }

        double perimeter = calcPerimeter8(mask, width, height);
        double circularity = (perimeter==0)?0.0:(4.0*M_PI*area)/(perimeter*perimeter);

        fprintf(fcoord,"%d,%.2f,%.2f,%d,%.2f,%.4f\n",
            label,cx,cy,area,perimeter,circularity);

        if (circularity > best_circ) {
            best_circ = circularity;
            best_label = label;
        }

        for(int y=0;y<height;y++) free(mask[y]);
        free(mask);
    }
    fclose(fcoord);

    printf("最も円形なラベル: %d (円形度=%.4f)\n", best_label, best_circ);

    // 出力画像生成
    fpo=fopen(fnameout,"wb");
    fprintf(fpo,"P5\n%d %d\n%d\n",width,height,maxval);
    unsigned char *outdata = (unsigned char*)malloc(width*height);
    for(int y=0;y<height;y++){
        for(int x=0;x<width;x++){
            if(img[y][x]==best_label) outdata[y*width+x]=0;  // 黒で描画
            else outdata[y*width+x]=maxval;                  // 白背景
        }
    }
    fwrite(outdata,1,width*height,fpo);
    fclose(fpo);
    free(outdata);

    // 後処理
    for(int y=0;y<height;y++) free(img[y]);
    free(img);

    return 0;
}
