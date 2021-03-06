/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "LCD.h"
#include "font.h"
#include "touch.h"

#include "textbox.h"
#include "config.h"
#include "fftwnd.h"
#include "generator.h"
#include "measurement.h"
#include "oslcal.h"
#include "panvswr2.h"
#include "main.h"
#if FS_ENABLED
#include "ff_gen_drv.h"
#include "mmc_diskio.h"
#if USBD_ENABLED
#include "usbd_storage.h"
#endif
#endif
#include "crash.h"
#include "dsp.h"
#include "gen.h"
#include "aauart.h"
#include "build_timestamp.h"
#include "tdr.h"
#include "oslfile.h"

extern void Sleep(uint32_t);

static TEXTBOX_CTX_t main_ctx;
static TEXTBOX_t hbHwCal;
static TEXTBOX_t hbOslCal;
static TEXTBOX_t hbConfig;
static TEXTBOX_t hbPan;
static TEXTBOX_t hbMeas;
static TEXTBOX_t hbGen;
static TEXTBOX_t hbDsp;
static TEXTBOX_t hbClock;
static TEXTBOX_t hbUSBD;
static TEXTBOX_t hbTimestamp;
static TEXTBOX_t hbTDR;
static TEXTBOX_t hbZ0;

#define M_BGCOLOR LCD_RGB(0,0,127)    //Menu item background color
#define M_FGCOLOR LCD_RGB(255,255,255) //Menu item foreground color

#define COL1 10  //Column 1 x coordinate
#define COL2 250 //Column 2 x coordinate

#if FS_ENABLED
#if USBD_ENABLED
static USBD_HandleTypeDef USBD_Device;
extern char MMCPath[4];
extern FATFS MMCFatFs;
extern USBD_DescriptorsTypeDef MSC_Desc;
static void USBD_Proc()
{
    while(TOUCH_IsPressed());

    LCD_FillAll(LCD_BLACK);
    FONT_Write(FONT_FRANBIG, LCD_YELLOW, LCD_BLACK, 15, 0, "USB storage access via USB FS port");
    FONT_Write(FONT_FRANBIG, LCD_YELLOW, LCD_BLUE, 80, 200, " Exit (Reset device) ");

    FATFS_UnLinkDriver(MMCPath);
    BSP_MMC_DeInit();
    Sleep(10);

    USBD_Init(&USBD_Device, &MSC_Desc, 0);
    USBD_RegisterClass(&USBD_Device, USBD_MSC_CLASS);
    USBD_MSC_RegisterStorage(&USBD_Device, &USBD_DISK_fops);
    USBD_Start(&USBD_Device);
    //HAL_PWREx_EnableUSBReg();
    HAL_PWREx_EnableUSBVoltageDetector();

    //USBD works in interrupts only, no need to leave CPU running in main.
    for(;;)
    {
        Sleep(50); //To enter low power if necessary
        LCDPoint coord;
        if (TOUCH_Poll(&coord))
        {
            while(TOUCH_IsPressed());
            if (coord.x > 80 && coord.x < 320 && coord.y > 200 && coord.y < 240)
            {
                USBD_Stop(&USBD_Device);
                USBD_DeInit(&USBD_Device);
                BSP_LCD_DisplayOff();
                BSP_MMC_DeInit();
                Sleep(100);
                NVIC_SystemReset(); //Never returns
            }
        }
    }
}
#else
static void USBD_Proc(void)
{
}
#endif // USBD_ENABLED
#endif // FS_ENABLED

static void _get_rtc_time(char *rtc_txt)
{
    RTC_DateTypeDef sdatestructureget = {0};
    RTC_TimeTypeDef stimestructureget = {0};

    HAL_RTC_GetTime(&RtcHandle, &stimestructureget, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&RtcHandle, &sdatestructureget, RTC_FORMAT_BIN);

    if (sdatestructureget.Month > 12)
    {
        sdatestructureget.Month = 0;
    }
    sprintf(rtc_txt, "%s %.2d, %.2d %.2d:%.2d:%.2d",
        RTC_MonTxt[sdatestructureget.Month], sdatestructureget.Date, 2000 + sdatestructureget.Year,
        stimestructureget.Hours, stimestructureget.Minutes, stimestructureget.Seconds);
}

// ================================================================================================
// Main window procedure (never returns)
void MainWnd(void)
{
    BSP_LCD_SelectLayer(1);
    LCD_FillAll(LCD_BLACK);
    LCD_ShowActiveLayerOnly();

    while (TOUCH_IsPressed());

    //Initialize textbox context
    TEXTBOX_InitContext(&main_ctx);

    //Create menu items and append to textbox context

    //HW calibration menu
    hbHwCal = (TEXTBOX_t){.x0 = COL1, .y0 = 0, .text =    " HW Calibration  ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = OSL_CalErrCorr,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbHwCal);

    //OSL calibration menu
    hbOslCal = (TEXTBOX_t){.x0 = COL1, .y0 = 50, .text =  " OSL Calibration ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = OSL_CalWnd,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbOslCal);

    //Device configuration menu
    hbConfig = (TEXTBOX_t){.x0 = COL1, .y0 = 100, .text = " Configuration  ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = CFG_ParamWnd,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbConfig);

    //Clock setup
    hbClock = (TEXTBOX_t){.x0 = COL1, .y0 = 150, .text =   " Set clock ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = CFG_RTC_Wnd,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbClock);

    //USB access
    hbUSBD = (TEXTBOX_t){.x0 = COL1, .y0 = 200, .text =   " USB FS cardrdr ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = USBD_Proc,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbUSBD);

    //Panoramic scan window
    hbPan = (TEXTBOX_t){.x0 = COL2, .y0 =   0, .text =    " Panoramic scan ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = PANVSWR2_Proc,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbPan);

    //Measurement window
    hbMeas = (TEXTBOX_t){.x0 = COL2, .y0 =  40, .text =   " Measurement    ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = MEASUREMENT_Proc,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbMeas);

    //Generator window
    hbGen  = (TEXTBOX_t){.x0 = COL2, .y0 = 80, .text =   " Generator      ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = GENERATOR_Window_Proc,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbGen);

    //DSP window
    hbDsp  = (TEXTBOX_t){.x0 = COL2, .y0 = 120, .text =   " DSP            ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = FFTWND_Proc,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbDsp);

    //TDR window
    hbTDR = (TEXTBOX_t){.x0 = COL2, .y0 = 160, .text = " Time Domain ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = TDR_Proc,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbTDR);

    //Z0 window
    hbZ0 = (TEXTBOX_t){.x0 = COL2, .y0 = 200, .text = " Measure line Z0 ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = MEASUREMENT_Z0_Proc,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbZ0);

    hbTimestamp = (TEXTBOX_t) {.x0 = 0, .y0 = 256, .text = VERSION_STRING, .font = FONT_FRAN,
                            .fgcolor = LCD_LGRAY, .bgcolor = LCD_BLACK };
    TEXTBOX_Append(&main_ctx, &hbTimestamp);
    //Draw context
    TEXTBOX_DrawContext(&main_ctx);

    //Main loop
    uint32_t rtcctr = HAL_GetTick();
    for(;;)
    {
        Sleep(0); //for autosleep to work
        if (TEXTBOX_HitTest(&main_ctx))
        {
            Sleep(50);
            //Redraw main window
            BSP_LCD_SelectLayer(1);
            LCD_FillAll(LCD_BLACK);
            TEXTBOX_DrawContext(&main_ctx);
            LCD_ShowActiveLayerOnly();
        }
        extern void uart_rx_proc(void);
        uart_rx_proc();

        if (HAL_GetTick() - rtcctr > 100U)
        {
            char rtc_txt[64];
            _get_rtc_time(rtc_txt);
            LCD_ShowActiveLayerOnly();
            FONT_Write(FONT_FRAN, LCD_YELLOW, LCD_BLACK, 0, 240, rtc_txt);
            rtcctr = HAL_GetTick();
        }
    }
}
