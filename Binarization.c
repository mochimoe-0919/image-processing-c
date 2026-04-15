// pgm ファイルから画像データの読込とファイルへの書き出し

#include <stdio.h>
#include <string.h>

int main(void){
  FILE *fpi=NULL;  // 入力画像
  FILE *fpo=NULL;  // 出力画像

  char buf[258];
  int width, height; // 画像の横長と縦長
  int maxvalue;  // 画像の最大輝度値
  int i, xSize, ySize;
  char fnamein[]="image1-2-3.pgm"; 
  char fnameout[]="image1-2-3-out.pgm"; 


  //ファイル fnamein から画像データを読込む

  fpi=fopen(fnamein, "r");

  if(fpi==NULL)
    printf("Reading file open error ! %s\n", fnamein);
   
  do  {
       fgets(buf, 256, fpi);
  } while(buf[0] == '#');  // skip over comments

  if(buf[0] != 'P' || buf[1] != '2') {
    fclose(fpi);
    printf(" \n The input image is not pgm format! \n\n");
    return -1;
  }   

  do   {
       fgets(buf , 256 , fpi) ;
   } while(buf [0 ] == '#') ;  // skip over comments

  sscanf(buf , "%d %d" , &width , &height) ; // 画像サイズを読み込む
  fscanf(fpi , "%d" , &maxvalue) ; //最大の輝度値を読み込み

  int img[height][width];

  for(ySize=0; ySize<height; ySize++) {
    for(xSize=0; xSize<width; xSize++) {
      fscanf(fpi, "%d", &img[ySize][xSize]);
    }
  }   //    画像中(x,y)の位置にある画素をimg[ySize][xSize]で参照できる。

  fclose(fpi); 

     double p;
    printf("p:");
    scanf("%lf", &p);

    int allData = width * height;

    int targetedArea = allData * p;

    //0~255の各値が出た回数をカウント
    int countColor[256];
    for (int i = 0; i < 256; i++)
    {
        countColor[i] = 0;
    }
    for (int y = 0; y < ySize; y++)
    {
        for (int x = 0; x < xSize; x++)
        {
            countColor[img[y][x]]++;
        }
    }

    //降順に, 出た回数の総和をとる
    int tmp = 0;
    int border;
    for (int i = 255; i >= 0; i--)
    {
        tmp += countColor[i];

        //総和が全体画素数のp割以上になったら，そこをしきい値とする．
        if (tmp >= targetedArea)
        {
            border = i;
            break;
        }
    }

    //しきい値以上かそうでないかで2値化する．
    for (int y = 0; y < ySize; ++y)
    {
        for (int x = 0; x < xSize; ++x)
        {
            if (img[y][x] <= border)
            {
                img[y][x] = 0;
            }
            else
            {
                img[y][x] = 255;
            }
        }
    }


 //画像データをファイル fnameout に出力する

  fpo=fopen(fnameout, "w");

  if(fpo==NULL)
    printf("Reading file open error ! %s\n", fnameout); 

  fprintf(fpo, "P2\n") ;
  fprintf(fpo, "# %s\n", fnameout);
  fprintf(fpo, "%d %d", width, height);
  fprintf(fpo, " \n");
  fprintf(fpo, "%d", maxvalue);
  fprintf(fpo, " \n") ;

  int count=0; 

  for(ySize=0; ySize<height; ySize++) {
    for(xSize=0; xSize<width; xSize++) {
      fprintf(fpo, "%3d ", img[ySize][xSize]);
      count++;
      if(count%18 == 0){
	fprintf(fpo, "\n");  //1行18個画素
      }
    }
  }
  fclose(fpo) ;  // 出力画像をクローズする
}
