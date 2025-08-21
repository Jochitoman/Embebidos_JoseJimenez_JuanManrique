#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Estructura RGB (ignoramos alpha si existe)
typedef struct {
    uint8_t r, g, b;
} Pixel;

// Cabecera archivo BMP (14 bytes)
#pragma pack(push,1)
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} BITMAPFILEHEADER;

// Cabecera info BMP (40 bytes estandar)
typedef struct {
    uint32_t size;
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t compression;
    uint32_t sizeImage;
    int32_t  xPelsPerMeter;
    int32_t  yPelsPerMeter;
    uint32_t clrUsed;
    uint32_t clrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

// ---------------- FUNCIONES ----------------

// Cargar BMP en memoria (24 bpp)
Pixel **loadBMP(const char *filename, int *outW, int *outH,
                BITMAPFILEHEADER *fh, BITMAPINFOHEADER *ih) {
    FILE *f = fopen(filename, "rb");
    if (!f) { perror("Error abriendo BMP"); return NULL; }

    fread(fh, sizeof(*fh), 1, f);
    fread(ih, sizeof(*ih), 1, f);

    if (fh->type != 0x4D42) {
        printf("No es BMP válido.\n"); fclose(f); return NULL;
    }
    if (ih->bitCount != 24) {
        printf("Solo soporta BMP de 24 bits\n"); fclose(f); return NULL;
    }

    int w = ih->width, h = ih->height;
    *outW = w; *outH = h;

    Pixel **img = malloc(h * sizeof(Pixel*));
    for (int y = 0; y < h; y++) img[y] = malloc(w * sizeof(Pixel));

    fseek(f, fh->offset, SEEK_SET);
    int bpp = ih->bitCount / 8;  
    int rowSize = ((ih->bitCount * w + 31) / 32) * 4;
    uint8_t *rowBuf = malloc(rowSize);

    int bottomUp = (ih->height > 0);
    for (int y = 0; y < h; y++) {
        fread(rowBuf, 1, rowSize, f);
        int destY = bottomUp ? (h - 1 - y) : y;
        for (int x = 0; x < w; x++) {
            uint8_t b = rowBuf[x * bpp + 0];
            uint8_t g = rowBuf[x * bpp + 1];
            uint8_t r = rowBuf[x * bpp + 2];
            img[destY][x] = (Pixel){r,g,b};
        }
    }

    free(rowBuf); fclose(f);
    return img;
}

// Guardar matriz en BMP (24 bpp)
void saveBMP(const char *filename, Pixel **img, int w, int h,
            BITMAPFILEHEADER fh, BITMAPINFOHEADER ih) {
    FILE *f = fopen(filename, "wb");
    if (!f) { perror("Error guardando BMP"); return; }

    int rowSize = ((24 * w + 31) / 32) * 4;
    int imageSize = rowSize * h;
    fh.size = 54 + imageSize;
    ih.sizeImage = imageSize;
    ih.bitCount = 24;

    fwrite(&fh, sizeof(fh), 1, f);
    fwrite(&ih, sizeof(ih), 1, f);

    uint8_t *rowBuf = calloc(1, rowSize);
    for (int y = h-1; y >= 0; y--) {
        for (int x = 0; x < w; x++) {
            rowBuf[x*3+0] = img[y][x].b;
            rowBuf[x*3+1] = img[y][x].g;
            rowBuf[x*3+2] = img[y][x].r;
        }
        fwrite(rowBuf, 1, rowSize, f);
    }
    free(rowBuf); fclose(f);
}

// Escala de grises
Pixel **toGray(Pixel **img, int w, int h) {
    Pixel **out = malloc(h * sizeof(Pixel*));
    for (int y = 0; y < h; y++) {
        out[y] = malloc(w * sizeof(Pixel));
        for (int x = 0; x < w; x++) {
            int g = (int)(0.3*img[y][x].r + 0.59*img[y][x].g + 0.11*img[y][x].b);
            out[y][x] = (Pixel){g,g,g};
        }
    }
    return out;
}

// Convolución 3x3
Pixel **convolution(Pixel **img, int w, int h, int kernel[3][3], int divisor) {
    Pixel **out = malloc(h * sizeof(Pixel*));
    for (int y = 0; y < h; y++) {
        out[y] = malloc(w * sizeof(Pixel));
        for (int x = 0; x < w; x++) {
            if (x==0 || y==0 || x==w-1 || y==h-1) { 
                out[y][x] = img[y][x];
            } else {
                int sumR=0,sumG=0,sumB=0;
                for (int ky=-1; ky<=1; ky++)
                    for (int kx=-1; kx<=1; kx++) {
                        int val = kernel[ky+1][kx+1];
                        Pixel p = img[y+ky][x+kx];
                        sumR += p.r * val;
                        sumG += p.g * val;
                        sumB += p.b * val;
                    }
                if (divisor==0) divisor=1;
                int r = sumR/divisor, g = sumG/divisor, b = sumB/divisor;
                if(r<0)r=0;if(r>255)r=255;
                if(g<0)g=0;if(g>255)g=255;
                if(b<0)b=0;if(b>255)b=255;
                out[y][x]=(Pixel){r,g,b};
            }
        }
    }
    return out;
}

// Liberar imagen
void freeImage(Pixel **img, int h) {
    for (int y=0;y<h;y++) free(img[y]);
    free(img);
}

// ---------------- MAIN ----------------
int main() {
    char fname[100];
    printf("Ingrese nombre de archivo BMP: ");
    scanf("%s", fname);

    int w,h;
    BITMAPFILEHEADER fh;
    BITMAPINFOHEADER ih;
    Pixel **img = loadBMP(fname, &w, &h, &fh, &ih);
    if (!img) return 1;

    int opc;
    do {
        printf("\nMenu:\n");
        printf("1. Escala de grises\n");
        printf("2. Convolucion\n");
        printf("0. Salir\n");
        printf("Opcion: ");
        scanf("%d", &opc);

        if (opc==1) {
            Pixel **gris = toGray(img,w,h);
            saveBMP("gris.bmp", gris,w,h,fh,ih);
            printf("Guardado en gris.bmp\n");
            freeImage(gris,h);
        }
        else if (opc==2) {
            int k[3][3]; int divisor=0;
            printf("Ingrese kernel 3x3:\n");
            for (int i=0;i<3;i++)
                for (int j=0;j<3;j++) {
                    scanf("%d",&k[i][j]);
                    divisor += k[i][j];
                }
            Pixel **conv = convolution(img,w,h,k,divisor);
            saveBMP("convolucion.bmp", conv,w,h,fh,ih);
            printf("Guardado en convolucion.bmp\n");
            freeImage(conv,h);
        }
        else if (opc!=0) {
            printf("Opcion no valida.\n");
        }

    } while(opc != 0);

    freeImage(img,h);
    return 0;
}
