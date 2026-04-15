#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef unsigned char SrcT;
typedef short DstT;

typedef struct {
    int left_x, right_x, y;
    SrcT value;
} RasterSegment;

typedef struct RegionInfo {
    int num_pixels;
    int min_x, min_y, max_x, max_y;
    float gravity_x, gravity_y;
    SrcT source_value;
    DstT label;
    RasterSegment* segments;
    int num_segments;
} RegionInfo;

#define MAX_SEGMENTS 1000000
#define MAX_REGIONS  100000

RasterSegment* segment_list[MAX_SEGMENTS];
int num_segments = 0;

RegionInfo* regions[MAX_REGIONS];
int num_regions = 0;

unsigned char* read_pbm(const char* filename, int* width, int* height) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        exit(1);
    }

    char magic[3];
    if (fscanf(fp, "%2s", magic) != 1) {
        fprintf(stderr, "Cannot read magic number from %s\n", filename);
        exit(1);
    }

    if (strcmp(magic, "P1") != 0 && strcmp(magic, "P4") != 0) {
        fprintf(stderr, "Unsupported PBM format: %s\n", magic);
        exit(1);
    }

   
    int c = fgetc(fp);
    while (c == '#' || isspace(c)) {
        if (c == '#') {
            while (c != '\n' && c != EOF) c = fgetc(fp); // コメント行を飛ばす
        }
        c = fgetc(fp);
    }
    ungetc(c, fp); // 1文字戻す

    // --- 幅・高さの読み取り ---
    if (fscanf(fp, "%d %d", width, height) != 2) {
        fprintf(stderr, "Cannot read image size from %s\n", filename);
        exit(1);
    }

    // 改行やスペースをスキップ
    do { c = fgetc(fp); } while (isspace(c));
    if (c != EOF) ungetc(c, fp);

    if (*width <= 0 || *height <= 0) {
        fprintf(stderr, "Invalid image size: %d x %d\n", *width, *height);
        exit(1);
    }

    unsigned char* data = malloc((*width) * (*height));
    if (!data) {
        perror("malloc");
        exit(1);
    }

    if (strcmp(magic, "P1") == 0) {
        // --- ASCII PBM ---
        for (int i = 0; i < (*width) * (*height); i++) {
            int bit;
            if (fscanf(fp, "%d", &bit) != 1) {
                fprintf(stderr, "Unexpected EOF in ASCII PBM.\n");
                exit(1);
            }
            data[i] = (unsigned char)bit;
        }
    } else {
        // --- Binary PBM (P4) ---
        int row_bytes = ((*width) + 7) / 8;
        for (int y = 0; y < *height; y++) {
            for (int x = 0; x < *width; x++) {
                if (x % 8 == 0)
                    c = fgetc(fp);
                if (c == EOF) {
                    fprintf(stderr, "Unexpected EOF in binary PBM.\n");
                    exit(1);
                }
                data[y * (*width) + x] = (c & (0x80 >> (x % 8))) ? 0 : 1;
            }
        }
    }

    fclose(fp);
    return data;
}


// PGM 書き出し関数
void write_pgm(const char* filename, DstT* img, int width, int height) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        perror("fopen");
        exit(1);
    }
    fprintf(fp, "P5\n%d %d\n255\n", width, height);

    for (int i = 0; i < width * height; i++) {
        unsigned char v = (img[i] > 255) ? 255 : (unsigned char)img[i];
        fwrite(&v, 1, 1, fp);
    }
    fclose(fp);
}


// Raster Segment 登録
void register_segment(int lx, int rx, int y, SrcT val) {
    RasterSegment* rs = (RasterSegment*)malloc(sizeof(RasterSegment));
    rs->left_x = lx;
    rs->right_x = rx;
    rs->y = y;
    rs->value = val;
    segment_list[num_segments++] = rs;
}


// 簡易ラベリング処理（4近傍）
void labeling(SrcT* src, DstT* dst, int width, int height) {
    memset(dst, 0, sizeof(DstT) * width * height);
    num_segments = 0;
    num_regions = 0;

    // Phase 1: scanline → segments
    for (int y = 0; y < height; y++) {
        int lx = 0;
        SrcT cur = 0;
        for (int x = 0; x < width; x++) {
            if (src[y*width + x] != cur) {
                if (cur != 0) register_segment(lx, x - 1, y, cur);
                cur = src[y*width + x];
                lx = x;
            }
        }
        if (cur != 0) register_segment(lx, width - 1, y, cur);
    }

    // Phase 2: naive connection (4-neighbor)
    for (int i = 0; i < num_segments; i++) {
        RasterSegment* rs = segment_list[i];
        if (dst[rs->y * width + rs->left_x] != 0) continue; // already labeled

        DstT label = num_regions + 1;
        RegionInfo* r = (RegionInfo*)malloc(sizeof(RegionInfo));
        r->num_pixels = 0;
        r->min_x = rs->left_x;
        r->max_x = rs->right_x;
        r->min_y = r->max_y = rs->y;
        r->source_value = rs->value;

        // flood fill by stack
        RasterSegment* stack[MAX_SEGMENTS];
        int sp = 0;
        stack[sp++] = rs;

        while (sp > 0) {
            RasterSegment* s = stack[--sp];
            for (int x = s->left_x; x <= s->right_x; x++) {
                dst[s->y * width + x] = label;
            }
            r->num_pixels += (s->right_x - s->left_x + 1);
            if (s->left_x < r->min_x) r->min_x = s->left_x;
            if (s->right_x > r->max_x) r->max_x = s->right_x;
            if (s->y < r->min_y) r->min_y = s->y;
            if (s->y > r->max_y) r->max_y = s->y;

            // check vertical neighbors
            for (int dy = -1; dy <= 1; dy += 2) {
                int ny = s->y + dy;
                if (ny < 0 || ny >= height) continue;
                for (int j = 0; j < num_segments; j++) {
                    RasterSegment* nb = segment_list[j];
                    if (nb->y == ny && nb->value == s->value &&
                        nb->right_x >= s->left_x && nb->left_x <= s->right_x) {
                        if (dst[ny * width + nb->left_x] == 0) {
                            stack[sp++] = nb;
                        }
                    }
                }
            }
        }
        regions[num_regions++] = r;
    }
}


// メイン関数
int main(void) {
    int width, height;
    unsigned char* src = read_pbm("zukei.pbm", &width, &height);
    DstT* dst = (DstT*)malloc(sizeof(DstT) * width * height);

    labeling(src, dst, width, height);

    printf("Detected regions: %d\n", num_regions);
    write_pgm("labeled.pgm", dst, width, height);
    printf("Output saved as labeled.pgm\n");

    free(src);
    free(dst);
    for (int i = 0; i < num_segments; i++) free(segment_list[i]);
    for (int i = 0; i < num_regions; i++) free(regions[i]);
    return 0;
}
