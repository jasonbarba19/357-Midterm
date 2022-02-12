#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/mman.h>

typedef unsigned char BYTE;

typedef unsigned short WORD; 
typedef unsigned int DWORD; 
typedef unsigned int LONG; 

struct tagBITMAPFILEHEADER 
{
    WORD bfType;  
    DWORD bfSize;  
    WORD bfReserved1;  
    WORD bfReserved2; 
    DWORD bfOffBits;  
}; 

struct tagBITMAPINFOHEADER 
{ 
    DWORD biSize;  
    LONG biWidth; 
    LONG biHeight; 
    WORD biPlanes; 
    WORD biBitCount; 
    DWORD biCompression;
    DWORD biSizeImage;   
    LONG biXPelsPerMeter;  
    LONG biYPelsPerMeter;   
    DWORD biClrUsed;   
    DWORD biClrImportant;  
};

typedef struct col
{
    int r, g, b;

} col;

typedef struct compressedformat
{
    int width, height;
    int rowbyte_quarter[4]; //for parallel algorithms! That’s the location in bytes 
                            //splitting pts in bytes of the compressed image
                            //which exactly splits the result image after decompression into 4 equal parts!
    int palettecolors;
    col *colors; 
} compressedformat;

typedef struct chunk
{
    BYTE color_index; 
    short count;
} chunk;

int k = 0;

int main()
{   
    FILE *comp_file = fopen("compressed.bin", "rb");
    if (comp_file == NULL){
        printf("\nInvalid input file.\n");
        return 1;
    }
    int i = 0;
    int j = 0;
    //int k = 0;
    int x = 0;
    int num = 0;
    int val1 = 0;
    int val2 = 0;
    col color;
    BYTE *pixeldata = (BYTE*)mmap(NULL, 4320000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 0, 0);
    struct tagBITMAPFILEHEADER file_header;
    file_header.bfType = 19778;
    file_header.bfSize = 4320054;
    file_header.bfReserved1 = 0;
    file_header.bfReserved2 = 0;
    file_header.bfOffBits = 54;
    struct tagBITMAPINFOHEADER info_header;
    info_header.biSize = 40;
    info_header.biWidth = 1200;
    info_header.biHeight = 1200;
    info_header.biPlanes = 1;
    info_header.biBitCount = 24;
    info_header.biCompression = 0;
    info_header.biSizeImage = 4320000;
    info_header.biXPelsPerMeter = 3780;
    info_header.biYPelsPerMeter = 3780;
    info_header.biClrUsed = 0;
    info_header.biClrImportant = 0;
    compressedformat cf;
    fread(&cf.width, 4, 1, comp_file);
    fread(&cf.height, 4, 1, comp_file);
    fread(&cf.rowbyte_quarter, 4, 4, comp_file);
    fread(&cf.palettecolors, 4, 1, comp_file);
    cf.colors = (col*)malloc(cf.palettecolors*sizeof(col));
    chunk *color_info = (chunk*)malloc(cf.width*cf.height*sizeof(chunk));
    for (i; i < cf.palettecolors; i ++)
    {
        fread(&cf.colors[i].r, 4, 1, comp_file);
        fread(&cf.colors[i].g, 4, 1, comp_file);
        fread(&cf.colors[i].b, 4, 1, comp_file);
        num ++;
    }
    val1 = fread(&color_info[j].color_index, 1, 1, comp_file);
    while (val1 == 1)
    {
        fread(&color_info[j].count, 2, 1, comp_file);
        num ++;
        j ++;
        val1 = fread(&color_info[j].color_index, 1, 1, comp_file);

    }
    int pid = fork();
    int split = (info_header.biWidth * info_header.biHeight) / 2;

    // Bottom of image; first
    if (pid == 0){
        for (x; x < split; x ++)
        {   val2 = color_info[k].count;
            while (val2 == 0)
            {
                k ++;
                num ++;
                val2 = color_info[k].count;
            }
            color = cf.colors[color_info[k].color_index];
            pixeldata[(x * 3) + 2] = color.r;
            pixeldata[(x * 3) + 1] = color.g;
            pixeldata[(x * 3) + 0] = color.b;
            num ++;
            color_info[k].count -- ;
        }
        //printf("%d", k);
    }
    
    // Top of image; second
    else if (pid > 0){
        wait(0);
        //printf("%d", k);
        for (x; x < (info_header.biWidth * info_header.biHeight); x ++)
        {   val2 = color_info[k].count;
            while (val2 == 0)
            {
                k ++;
                num ++;
                val2 = color_info[k].count;
            }
            color = cf.colors[color_info[k].color_index];
            pixeldata[(x * 3) + 2] = color.r;
            pixeldata[(x * 3) + 1] = color.g;
            pixeldata[(x * 3) + 0] = color.b;
            num ++;
            color_info[k].count -- ;
         }
    }
    
    fclose(comp_file);
    FILE* decomp_file;
    decomp_file = fopen("decompressed.bmp", "wb+");
    fwrite(&file_header.bfType, 2, 1, decomp_file);
    fwrite(&file_header.bfSize, 4, 1, decomp_file);
    fwrite(&file_header.bfReserved1, 2, 1, decomp_file);
    fwrite(&file_header.bfReserved2, 2, 1, decomp_file);
    fwrite(&file_header.bfOffBits, 4, 1, decomp_file);
    fwrite(&info_header, sizeof(struct tagBITMAPINFOHEADER), 1, decomp_file);
    fwrite(pixeldata, info_header.biSizeImage, 1, decomp_file);
    fclose(decomp_file);
    free(cf.colors);
    free(color_info);
    munmap(&pixeldata, 4320000);
}