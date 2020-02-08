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
static TEXTBOX_t hbTitle;
static TEXTBOX_t hbHwCal;
static TEXTBOX_t hbOslCal;
static TEXTBOX_t hbConfig;
static TEXTBOX_t hbPan;
static TEXTBOX_t hbMeas;
static TEXTBOX_t hbGen;
static TEXTBOX_t hbDsp;
static TEXTBOX_t hbUSBD;
static TEXTBOX_t hbTimestamp;
static TEXTBOX_t hbTDR;

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

    hbTitle = (TEXTBOX_t) {.x0 = COL1, .y0 = 0, .text = " Main menu ", .font = FONT_FRANBIG,
                            .fgcolor = LCD_WHITE, .bgcolor = LCD_BLACK };
    TEXTBOX_Append(&main_ctx, &hbTitle);

    //HW calibration menu
    hbHwCal = (TEXTBOX_t){.x0 = COL1, .y0 = 50, .text =    " HW Calibration  ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = OSL_CalErrCorr,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbHwCal);

    //OSL calibration menu
    hbOslCal = (TEXTBOX_t){.x0 = COL1, .y0 = 100, .text =  " OSL Calibration ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = OSL_CalWnd,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbOslCal);

    //Device configuration menu
    hbConfig = (TEXTBOX_t){.x0 = COL1, .y0 = 150, .text = " Configuration  ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = CFG_ParamWnd,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbConfig);

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
    hbMeas = (TEXTBOX_t){.x0 = COL2, .y0 =  50, .text =   " Measurement    ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = MEASUREMENT_Proc,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbMeas);

    //Generator window
    hbGen  = (TEXTBOX_t){.x0 = COL2, .y0 = 100, .text =   " Generator      ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = GENERATOR_Window_Proc,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbGen);

    //DSP window
    hbDsp  = (TEXTBOX_t){.x0 = COL2, .y0 = 150, .text =   " DSP            ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = FFTWND_Proc,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbDsp);

    //TDR window
    hbTDR = (TEXTBOX_t){.x0 = COL2, .y0 = 200, .text = " Time Domain ", .font = FONT_FRANBIG,
                            .fgcolor = M_FGCOLOR, .bgcolor = M_BGCOLOR, .cb = TDR_Proc,
                            .border = TEXTBOX_BORDER_BUTTON };
    TEXTBOX_Append(&main_ctx, &hbTDR);

    hbTimestamp = (TEXTBOX_t) {.x0 = 0, .y0 = 256, .text = VERSION_STRING, .font = FONT_FRAN,
                            .fgcolor = LCD_LGRAY, .bgcolor = LCD_BLACK };
    TEXTBOX_Append(&main_ctx, &hbTimestamp);
    //Draw context
    TEXTBOX_DrawContext(&main_ctx);

    //PROTOCOL_Reset();

    //Main loop
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
            //PROTOCOL_Reset();
        }
        extern void uart_rx_proc(void);
        uart_rx_proc();
        //PROTOCOL_Handler();
    }
}
