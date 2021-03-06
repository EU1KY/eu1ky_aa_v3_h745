/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <stdint.h>
#include <stdio.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <string.h>

#include "LCD.h"
#include "touch.h"
#if FS_ENABLED
#include "ff.h"
#endif
#include "textbox.h"
#include "screenshot.h"
#include "crash.h"
#include "stm32h745i_discovery_lcd.h"
#include "keyboard.h"
#include "lodepng.h"
#include "sdram_heap.h"

extern void Sleep(uint32_t);

#if FS_ENABLED
static const TCHAR *SNDIR = "/aa/snapshot";
static uint32_t oldest = 0xFFFFFFFFul;
static uint32_t numfiles = 0;
#endif

#define SCREENSHOT_FILE_SIZE 391734
uint8_t __attribute__((section (".user_sdram"))) __attribute__((used)) bmpFileBuffer[SCREENSHOT_FILE_SIZE];

__attribute__((unused)) static const uint8_t bmp_hdr[] =
{
    0x42, 0x4D,             //"BM"
    0x36, 0xFA, 0x05, 0x00, //size in bytes
    0x00, 0x00, 0x00, 0x00, //reserved
    0x36, 0x00, 0x00, 0x00, //offset to image in bytes
    0x28, 0x00, 0x00, 0x00, //info size in bytes
    0xE0, 0x01, 0x00, 0x00, //width
    0x10, 0x01, 0x00, 0x00, //height
    0x01, 0x00,             //planes
    0x18, 0x00,             //bits per pixel
    0x00, 0x00, 0x00, 0x00, //compression
    0x00, 0xfa, 0x05, 0x00, //image size
    0x00, 0x00, 0x00, 0x00, //x resolution
    0x00, 0x00, 0x00, 0x00, //y resolution
    0x00, 0x00, 0x00, 0x00, // colours
    0x00, 0x00, 0x00, 0x00  //important colours
};


//This is the prototype of the function that draws a screenshot from a file on SD card,
//and waits for tapping the screen to exit.
//fname must be without path and extension
//For example: SCREENSHOT_Window("00000214");
//TODO: add error handling
void SCREENSHOT_Show(const char* fname)
{
#if FS_ENABLED
    char path[255];
    while(TOUCH_IsPressed());
    sprintf(path, "%s/%s.bmp", SNDIR, fname);
    FIL fo = { 0 };
    FRESULT fr = f_open(&fo, path, FA_READ);
    if (FR_OK != fr)
        return;
    uint32_t br = 0;
    fr = f_read(&fo, bmpFileBuffer, SCREENSHOT_FILE_SIZE, (UINT*)&br);
    f_close(&fo);
    if (br != SCREENSHOT_FILE_SIZE || FR_OK != fr)
        return;
    LCD_DrawBitmap(LCD_MakePoint(0,0), bmpFileBuffer, SCREENSHOT_FILE_SIZE);
    while (!TOUCH_IsPressed())
        Sleep(50);
    LCD_FillAll(LCD_BLACK);
    while (TOUCH_IsPressed())
        Sleep(50);
#endif
}

void SCREENSHOT_SelectFileName(char *fname)
{
    uint32_t dfnum = 0;
    fname[0] = '\0';
#if FS_ENABLED
    f_mkdir(SNDIR);

    //Scan dir for snapshot files
    uint32_t fmax = 0;
    uint32_t fmin = 0xFFFFFFFFul;
    DIR dir = { 0 };
    FILINFO fno = { 0 };
    FRESULT fr = f_opendir(&dir, SNDIR);
    numfiles = 0;
    oldest = 0xFFFFFFFFul;
    fname[0] = '\0';
    int i;
    if (fr == FR_OK)
    {
        for (;;)
        {
            fr = f_readdir(&dir, &fno); //Iterate through the directory
            if (fr != FR_OK || !fno.fname[0])
                break; //Nothing to do
            if (_FS_RPATH && fno.fname[0] == '.')
                continue; //bypass hidden files
            if (fno.fattrib & AM_DIR)
                continue; //bypass subdirs
            int len = strlen(fno.fname);
            if (len != 12) //Bypass filenames with unexpected name length
                continue;
            const char *pdot = strchr(fno.fname, (int)'.');
            if (0 == pdot)
                continue;
            if (0 != strcasecmp(pdot, ".bmp") && 0 != strcasecmp(pdot, ".png"))
                continue; //Bypass files that are not bmp
            for (i = 0; i < 8; i++)
                if (!isdigit((int)fno.fname[i]))
                    break;
            if (i != 8u)
                continue; //Bypass file names that are not 8-digit numbers
            numfiles++;
            //Now convert file name to number

            char* endptr;
            dfnum = strtoul(fno.fname, &endptr, 10);
            if (dfnum < fmin)
                fmin = dfnum;
            if (dfnum > fmax)
                fmax = dfnum;
        }
        f_closedir(&dir);
    }
    else
    {
        CRASHF("Failed to open directory %s", SNDIR);
    }

    oldest = fmin;
    dfnum = fmax + 1;
#endif
    sprintf(fname, "%08lu", dfnum);
    KeyboardWindow(fname, 8, "Select screenshot file name");
}

void SCREENSHOT_DeleteOldest(void)
{
#if FS_ENABLED
    char path[128];
    if (0xFFFFFFFFul != oldest && numfiles >= 100)
    {
        sprintf(path, "%s/%08lu.s1p", SNDIR, oldest);
        f_unlink(path);
        sprintf(path, "%s/%08lu.bmp", SNDIR, oldest);
        f_unlink(path);
        sprintf(path, "%s/%08lu.png", SNDIR, oldest);
        f_unlink(path);
        numfiles = 0;
        oldest = 0xFFFFFFFFul;
    }
#endif
}

void SCREENSHOT_Save(const char *fname)
{
#if FS_ENABLED
    char path[64];
    FRESULT fr = FR_OK;
    FIL fo = { 0 };

    SCB_CleanDCache_by_Addr((uint32_t*)BSP_LCD_GetActiveLayerStartAddress(), BSP_LCD_GetXSize()*BSP_LCD_GetYSize()*4); //Flush and invalidate D-Cache contents to the RAM to avoid cache coherency
    Sleep(2);

    if ((strlen(SNDIR) + strlen(fname) + 6) > _MAX_LFN)
    {
        CRASH("File name too long");
    }

    BSP_LCD_DisplayOff();
    f_mkdir(SNDIR);

    //Now write screenshot as bitmap
    sprintf(path, "%s/%s.bmp", SNDIR, fname);
    fr = f_open(&fo, path, FA_CREATE_ALWAYS | FA_WRITE);
    if (FR_OK != fr)
        CRASHF("Failed to open file %s", path);
    uint32_t bw;
    fr = f_write(&fo, bmp_hdr, sizeof(bmp_hdr), (UINT*)&bw);
    if (FR_OK != fr) goto CRASH_WR;
    int x = 0;
    int y = 0;
    for (y = 271; y >= 0; y--)
    {
        uint32_t line[480];
        BSP_LCD_ReadLine(y, line);
        for (x = 0; x < 480; x++)
        {
            fr = f_write(&fo, &line[x], 3, (UINT*)&bw);
            if (FR_OK != fr) goto CRASH_WR;
        }
    }
    f_close(&fo);
    BSP_LCD_DisplayOn();
    return;
CRASH_WR:
    BSP_LCD_DisplayOn();
    CRASHF("Failed to write to file %s", path);
#endif
}

//Custom allocators for LodePNG using SDRAM heap
void* lodepng_malloc(size_t size)
{
    return SDRH_malloc(size);
}

void* lodepng_realloc(void* ptr, size_t new_size)
{
    return SDRH_realloc(ptr, new_size);
}

void lodepng_free(void* ptr)
{
    SDRH_free(ptr);
}

//Exhange R and B colors for proper PNG encoding
__attribute__((unused)) static void _Change_B_R(uint32_t* image)
{
    uint32_t i;
    uint32_t endi = (uint32_t)LCD_GetWidth() * (uint32_t)LCD_GetHeight();
    for (i = 0; i < endi; i++)
    {
        uint32_t c = image[i];
        uint32_t r = (c >> 16) & 0xFF;
        uint32_t b = c & 0xFF;
        image[i] = (c & 0xFF00FF00) | r | (b << 16);
    }
}

void SCREENSHOT_SavePNG(const char *fname)
{
#if FS_ENABLED
    char path[64];
    FRESULT fr = FR_OK;
    FIL fo = { 0 };

    uint8_t* image = LCD_Push();
    if (0 == image)
        CRASH("LCD_Push failed");

    _Change_B_R((uint32_t*)image);

    uint8_t* png = 0;
    size_t pngsize = 0;

    LCD_TurnOff();
    BSP_LCD_DisplayOff();
    uint32_t error = lodepng_encode32(&png, &pngsize, image, LCD_GetWidth(), LCD_GetHeight());
    if (error)
        CRASHF("lodepng_encode failed: %lu ", error);

    f_mkdir(SNDIR);
    sprintf(path, "%s/%s.png", SNDIR, fname);
    fr = f_open(&fo, path, FA_CREATE_ALWAYS | FA_WRITE);
    if (FR_OK != fr)
        CRASHF("Failed to open file %s", path);
    uint32_t bw;
    fr = f_write(&fo, png, pngsize, (UINT*)&bw);
    if (FR_OK != fr)
        CRASHF("Failed to write to file %s", path);
    f_close(&fo);
    BSP_LCD_DisplayOn();
    LCD_TurnOn();

    lodepng_free(png);
    _Change_B_R((uint32_t*)image);
    LCD_Pop();
#endif
}

/**  Display PNG file on the screen
 *
 *   The file must exist and must be a valid PNG with 480x272 resolution.
 * 
 *   @param fpath : full path to the file
 * 
 *   @retval 0 if the image from file has been successfully decoded and is on the screen,
 *           1 otherwise.
 */
int SCREENSHOT_ShowPNG(const char *fpath)
{
#if FS_ENABLED
    FRESULT fres;
    FIL flogo = { 0 };
    FILINFO finfo;

    fres = f_stat(fpath, &finfo);
    if ((fres != FR_OK) || (0 == finfo.fsize) || (finfo.fsize > (SDRH_HEAPSIZE / 4)))
    {
        return 1;
    }

    unsigned char *pngbuf = SDRH_malloc(finfo.fsize);
    if (0 == pngbuf)
    {
        return 1;
    }

    fres = f_open(&flogo, fpath, FA_READ);
    if (FR_OK == fres)
    {
        UINT br = 0;
        f_read(&flogo, pngbuf, finfo.fsize, &br);
        f_close(&flogo);

        unsigned char *outaddr = 0;
        unsigned w = 0;
        unsigned h = 0;
        unsigned pngres = lodepng_decode32(&outaddr, &w, &h, pngbuf, (size_t)finfo.fsize);
        SDRH_free(pngbuf);

        if (0 == pngres && 0 != outaddr && w == LCD_GetWidth() && h == LCD_GetHeight())
        {
            _Change_B_R((uint32_t*)outaddr);
            BSP_LCD_SelectLayer(!BSP_LCD_GetActiveLayer());
            memcpy((void*)BSP_LCD_GetActiveLayerStartAddress(), outaddr, LCD_GetWidth() * LCD_GetHeight() * sizeof(uint32_t));
            SDRH_free(outaddr);
            LCD_ShowActiveLayerOnly();
            return 0;
        }
        if (0 != outaddr)
        {
            SDRH_free(outaddr);
        }
    }
#endif // #if FS_ENABLED
    return 1;
}
