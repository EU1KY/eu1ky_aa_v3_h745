/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#include "stm32h7xx_hal.h"
#include "stm32h745i_discovery.h"
#include "stm32h745i_discovery_lcd.h"
#include "LCD.h"
#include "libnsbmp.h"
#include "crash.h"
#include <math.h>

//==============================================================================
//                  Module's static functions
//==============================================================================

static inline uint16_t _min(uint16_t a, uint16_t b)
{
    return a > b ? b : a;
}

static inline uint16_t _max(uint16_t a, uint16_t b)
{
    return a > b ? a : b;
}

static inline int _abs(int a)
{
    return (a < 0) ? -a : a;
}

uint16_t LCD_GetWidth(void)
{
    return BSP_LCD_GetXSize();
}

uint16_t LCD_GetHeight(void)
{
    return BSP_LCD_GetYSize();
}

void LCD_BacklightOn(void)
{
    BSP_LCD_SetBrightness(100);
}

void LCD_BacklightOff(void)
{
    BSP_LCD_SetBrightness(0);
}

void LCD_ShowActiveLayerOnly(void)
{
    uint32_t activeLayer = BSP_LCD_GetActiveLayer();
    BSP_LCD_SetLayerVisible_NoReload(!activeLayer, DISABLE);
    BSP_LCD_SetLayerVisible_NoReload(activeLayer, ENABLE);
    SCB_CleanInvalidateDCache();
    extern volatile uint32_t ReloadFlag;
    ReloadFlag = 0;
    BSP_LCD_Reload(LCD_RELOAD_VERTICAL_BLANKING);
    while (0 == ReloadFlag);
}

void LCD_Init(void)
{
    // Configure LCD : Only one layer is used
    BSP_LCD_Init();

    // Initialize LCD
    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
    BSP_LCD_LayerDefaultInit(1, LCD_FB_START_ADDRESS + LCD_FB_SDRAM_BANK_SIZE / 2);

    // Enable LCD
    BSP_LCD_DisplayOn();

    // Select background layer
    BSP_LCD_SelectLayer(0);

    // Clear the Background Layer
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    // Select foreground layer
    BSP_LCD_SelectLayer(1);
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    BSP_LCD_SetTransparency(0, 255);
    BSP_LCD_SetTransparency(1, 255);
    LCD_ShowActiveLayerOnly();
}

static uint32_t _lcd_is_on = 1;
void LCD_TurnOn(void)
{
    BSP_LCD_SetBrightness(100);
    _lcd_is_on = 1;
}

void LCD_TurnOff(void)
{
    BSP_LCD_SetBrightness(5);
    _lcd_is_on = 0;
}

uint32_t LCD_IsOff(void)
{
    return !_lcd_is_on;
}

void LCD_WaitForRedraw(void)
{
    if (LCD_IsOff())
        return;
    while (!(LTDC->CDSR & LTDC_CDSR_VSYNCS));
}

void LCD_FillRect(LCDPoint p1, LCDPoint p2, LCDColor color)
{
    if (p1.x >= LCD_GetWidth()) p1.x = LCD_GetWidth() - 1;
    if (p2.x >= LCD_GetWidth()) p2.x = LCD_GetWidth() - 1;
    if (p1.y >= LCD_GetHeight()) p1.y = LCD_GetHeight() - 1;
    if (p2.y >= LCD_GetHeight()) p2.y = LCD_GetHeight() - 1;

    color |= 0xFF000000;
    BSP_LCD_SetTextColor(color);
    BSP_LCD_FillRect(p1.x, p1.y, p2.x - p1.x + 1, p2.y - p1.y + 1);
}

void LCD_InvertRect(LCDPoint p1, LCDPoint p2)
{
    if (p1.x >= LCD_GetWidth()) p1.x = LCD_GetWidth() - 1;
    if (p2.x >= LCD_GetWidth()) p2.x = LCD_GetWidth() - 1;
    if (p1.y >= LCD_GetHeight()) p1.y = LCD_GetHeight() - 1;
    if (p2.y >= LCD_GetHeight()) p2.y = LCD_GetHeight() - 1;
    uint16_t x, y;
    for (x = p1.x; x <= p2.x; x++)
    {
        for (y = p1.y; y <= p2.y; y++)
        {
            BSP_LCD_DrawPixel(x, y, BSP_LCD_ReadPixel(x, y) ^ 0x00FFFFFF);
        }
    }
}

void LCD_FillAll(LCDColor c)
{
    c |= 0xFF000000;
    BSP_LCD_Clear(c);
}

LCDColor LCD_MakeRGB(uint8_t r, uint8_t g, uint8_t b)
{
    return LCD_RGB(r, g, b);
}

// Algorithm from https://stackoverflow.com/questions/141855/programmatically-lighten-a-color/141943#141943
LCDColor LCD_TintColor(LCDColor color, float k)
{
    float threshold = 255.999;

    uint8_t ri = color >> 16;
    uint8_t gi = color >> 8;
    uint8_t bi = color;

    float r = ri * k;
    float g = gi * k;
    float b = bi * k;
    float maxi = fmaxf(r, fmaxf(g, b));

    if (maxi <= threshold)
    {
        return LCD_RGB(r, g, b);
    }

    float total = r + g + b;
    if (total >= threshold * 3.0)
    {
        return LCD_RGB(threshold, threshold, threshold);
    }

    float x = (3.0 * threshold - total) / (3.0 * maxi - total);
    float grey = threshold - x * maxi;
    return LCD_RGB(grey + r * x, grey + g * x, grey + b * x);
}

LCDPoint LCD_MakePoint(int x, int y)
{
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    LCDPoint pt = {x, y};
    return pt;
}

LCDColor LCD_ReadPixel(LCDPoint p)
{

    if (p.x >= LCD_GetWidth() || p.y >= LCD_GetHeight())
        return LCD_BLACK;
    return BSP_LCD_ReadPixel(p.x, p.y);
 }

void LCD_InvertPixel(LCDPoint p)
{
    LCDColor c = LCD_ReadPixel(p);
    LCD_SetPixel(p, (c ^ 0x00FFFFFFul) | 0xFF000000ul);
}

void LCD_InvertLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    BSP_LCD_InvertLine(x1, y1, x2, y2);
}

void LCD_SetPixel(LCDPoint p, LCDColor color)
{
    if (p.x >= LCD_GetWidth() || p.y >= LCD_GetHeight())
        return;
    BSP_LCD_DrawPixel(p.x, p.y, color | 0xFF000000ul);
}

void LCD_Rectangle(LCDPoint a, LCDPoint b, LCDColor c)
{
    c |= 0xFF000000ul;
    BSP_LCD_SetTextColor(c);
    BSP_LCD_DrawRect(a.x, a.y, b.x - a.x + 1, b.y - a.y + 1);
}

void LCD_PolyLine(LCDPoint* points, uint16_t pointCount, LCDColor color)
{
    if (pointCount < 2)
        return;

    LCDPoint lcd_points[pointCount];

    for (int i = 0; i < pointCount; i++)
    {
        lcd_points[i].x = points[i].x;
        lcd_points[i].y = points[i].y;
        if (lcd_points[i].x >= LCD_GetWidth())
            lcd_points[i].x = LCD_GetWidth() - 1;
        if (lcd_points[i].y >= LCD_GetHeight())
            lcd_points[i].y = LCD_GetHeight() - 1;
    }

    color |= 0xFF000000ul;
    BSP_LCD_SetTextColor(color);

    for (int i = 1; i < pointCount; i++)
    {
        BSP_LCD_DrawLine(lcd_points[i-1].x, lcd_points[i-1].y,
                         lcd_points[i].x, lcd_points[i].y);
    }
}

void LCD_FillPolygon(LCDPoint* points, uint16_t pointCount, LCDColor color)
{
    Point lcd_points[pointCount];

    for (int i = 0; i < pointCount; i++)
    {
        lcd_points[i].X = points[i].x;
        lcd_points[i].Y = points[i].y;
        if (lcd_points[i].X >= LCD_GetWidth())
            lcd_points[i].X = LCD_GetWidth() - 1;
        if (lcd_points[i].Y >= LCD_GetHeight())
            lcd_points[i].Y = LCD_GetHeight() - 1;
    }

    color |= 0xFF000000ul;
    BSP_LCD_SetTextColor(color);
    BSP_LCD_FillPolygon(lcd_points, pointCount);
}

void LCD_Line(LCDPoint a, LCDPoint b, LCDColor color)
{
    if (a.x >= LCD_GetWidth())
        a.x = LCD_GetWidth() - 1;
    if (b.x >= LCD_GetWidth())
        b.x = LCD_GetWidth() - 1;
    if (a.y >= LCD_GetHeight())
        a.y = LCD_GetHeight() - 1;
    if (b.y >= LCD_GetHeight())
        b.y = LCD_GetHeight() - 1;
    color |= 0xFF000000ul;
    BSP_LCD_SetTextColor(color);
    BSP_LCD_DrawLine(a.x, a.y, b.x, b.y);
} //LCD_Line

void LCD_Line3(LCDPoint a, LCDPoint b, LCDColor color)
{
    if (a.x >= LCD_GetWidth())
        a.x = LCD_GetWidth() - 1;
    if (b.x >= LCD_GetWidth())
        b.x = LCD_GetWidth() - 1;
    if (a.y >= LCD_GetHeight())
        a.y = LCD_GetHeight() - 1;
    if (b.y >= LCD_GetHeight())
        b.y = LCD_GetHeight() - 1;
    color |= 0xFF000000ul;
    BSP_LCD_SetTextColor(color);
    BSP_LCD_DrawLine3(a.x, a.y, b.x, b.y);
} //LCD_Line3

void LCD_VLine(LCDPoint a, uint16_t lenght, LCDColor color)
{
    if (a.x >= LCD_GetWidth())
        a.x = LCD_GetWidth() - 1;
    if (a.y >= LCD_GetHeight())
        a.y = LCD_GetHeight() - 1;
    color |= 0xFF000000ul;
    BSP_LCD_SetTextColor(color);
    BSP_LCD_DrawVLine(a.x, a.y, lenght);
} //LCD_VLine

void LCD_HLine(LCDPoint a, uint16_t lenght, LCDColor color)
{
    if (a.x >= LCD_GetWidth())
        a.x = LCD_GetWidth() - 1;
    if (a.y >= LCD_GetHeight())
        a.y = LCD_GetHeight() - 1;
    color |= 0xFF000000ul;
    BSP_LCD_SetTextColor(color);
    BSP_LCD_DrawHLine(a.x, a.y, lenght);
} //LCD_HLine

void LCD_Circle(LCDPoint center, uint16_t r, LCDColor color)
{
    color |= 0xFF000000;
    BSP_LCD_SetTextColor(color);
    BSP_LCD_DrawCircle(center.x, center.y, r);
}

void LCD_FillCircle(LCDPoint center, uint16_t r, LCDColor color)
{
    color |= 0xFF000000;
    BSP_LCD_SetTextColor(color);
    BSP_LCD_FillCircle(center.x, center.y, r);
}

// Quadrants:
//     1 0
//     2 3
#define LCD_ArcPixel(x, y) if ((xmin <= x) && (x <= xmax)) LCD_SetPixel(LCD_MakePoint(centerx + x, centery + y), color)
static void LCD_DrawArcQuadrant(int32_t centerx, uint32_t centery, int32_t radius,
                                uint32_t quadrant, int32_t xmin, int32_t xmax, LCDColor color)
{
    int32_t x = 0;
    int32_t y = radius;
    int32_t tswitch = 3 - 2 * radius;

    while (x <= y)
    {
        if (quadrant == 0)
        {
            // arc right upper
            LCD_ArcPixel(x, -y);
            LCD_ArcPixel(y, -x);
        }
        else if (quadrant == 1)
        {
            // arc left upper
            LCD_ArcPixel(-x, -y);
            LCD_ArcPixel(-y, -x);
        }
        else if (quadrant == 2)
        {
            // arc left lower
            LCD_ArcPixel(-x, y);
            LCD_ArcPixel(-y, x);
        }
        else if (quadrant == 3)
        {
            // arc right lower
            LCD_ArcPixel(x, y);
            LCD_ArcPixel(y, x);
        }
        if (tswitch < 0)
            tswitch += (4 * x + 6);
        else
        {
            tswitch += (4 * (x - y) + 10);
            y--;
        }
        x++;
    }
}


void LCD_DrawArc(int32_t x, int32_t y, int32_t radius, float startDegrees, float endDegrees, LCDColor color)
{
    if (startDegrees < 0.f || startDegrees > endDegrees || startDegrees > 360.f || endDegrees > 360.f)
        return;

    float astart = M_PI * startDegrees / 180.f;
    float aend = M_PI * endDegrees / 180.f;

    int32_t xmax, xmin;
    if (astart <= M_PI / 2)
    {// Q0
        xmax = (int32_t)(cosf(astart) * radius);
        if (aend <= M_PI / 2)
            xmin = (int32_t)(cosf(aend) * radius);
        else
            xmin = 0;
        LCD_DrawArcQuadrant(x, y, radius, 0, xmin, xmax, color);
        if (endDegrees <= 90.f)
            return;
        astart = M_PI / 2;
    }

    if (astart <= M_PI)
    {// Q1
        xmax = (int32_t)(cosf(astart) * radius);
        if (aend <= M_PI)
            xmin = (int32_t)(cosf(aend) * radius);
        else
            xmin = -radius;
        LCD_DrawArcQuadrant(x, y, radius, 1, xmin, xmax, color);
        if (endDegrees <= 180.f)
            return;
        astart = M_PI;
    }

    if (astart <= (M_PI + M_PI / 2))
    {// Q2
        xmin = (int32_t)(cosf(astart) * radius);
        if (aend <= (M_PI + M_PI / 2))
            xmax = (int32_t)(cosf(aend) * radius);
        else
            xmax = 0;
        LCD_DrawArcQuadrant(x, y, radius, 2, xmin, xmax, color);
        if (endDegrees <= 270.f)
            return;
        astart = (M_PI + M_PI / 2);
    }

    if (astart < 2 * M_PI)
    {// Q3
        xmin = (int32_t)(cosf(astart) * radius);
        xmax = (int32_t)(cosf(aend) * radius);
        LCD_DrawArcQuadrant(x, y, radius, 3, xmin, xmax, color);
    }
}

//===================================================================
//Drawing bitmap file based on libnsbmp
//===================================================================
#define BYTES_PER_PIXEL 4
#define TRANSPARENT_COLOR 0xFFFFFFFF

static void *bitmap_create(int width, int height, unsigned int state)
{
    (void) state;  /* unused */
    return (void*)0x2003F000; // Fake: the bitmap buffer is not used
}

static unsigned char *bitmap_get_buffer(void *bitmap)
{
    return (unsigned char*)bitmap;
}

static size_t bitmap_get_bpp(void *bitmap)
{
    (void) bitmap;  /* unused */
    return BYTES_PER_PIXEL;
}

static void bitmap_destroy(void *bitmap)
{
}

static LCDPoint _bmpOrigin;

static void bitmap_putcolor(unsigned int color32, unsigned int x, unsigned int y)
{
    //DBGPRINT("LCD bmp color: 0x%08x, x: %d, y: %d\n", color32, x, y);
    uint16_t r, g, b;
    r = (uint16_t)(color32 & 0xFF);
    g = (uint16_t)((color32 & 0xFF00) >> 8);
    b = (uint16_t)((color32 & 0xFF0000) >> 16);
    LCDColor clr = LCD_RGB(r, g, b);
    LCD_SetPixel(LCD_MakePoint(_bmpOrigin.x + x, _bmpOrigin.y + y), clr);
}

static bmp_bitmap_callback_vt bitmap_callbacks =
{
    bitmap_create,
    bitmap_destroy,
    bitmap_get_buffer,
    bitmap_get_bpp,
    bitmap_putcolor
};

void LCD_DrawBitmap(LCDPoint origin, const uint8_t *bmpData, uint32_t bmpDataSize)
{
    bmp_image bmp;
    bmp_result code;
    _bmpOrigin = origin;

    bmp_create(&bmp, &bitmap_callbacks);
    code = bmp_analyse(&bmp, bmpDataSize, bmpData);
    if(code != BMP_OK)
    {
        CRASHF("bmp_analyse failed, %d\n", code);
        goto cleanup;
    }
    code = bmp_decode(&bmp);
    if(code != BMP_OK)
    {
        CRASHF("bmp_decode failed, %d\n", code);
        if(code != BMP_INSUFFICIENT_DATA)
        {
            goto cleanup;
        }
    }
cleanup:
    bmp_finalise(&bmp);
}

//========================================================================================================
// Display backup (to avoid redraws after running child window)
// to be used in temporary windows and pop-ups
// Uses a stack organized in the unused SDRAM as a temporary storage
//========================================================================================================
#define LCD_BUF_STACK_DEPTH 4
static uint8_t __attribute__((section (".user_sdram"))) scrBufferStack[LCD_BUF_STACK_DEPTH][480 * 272 * 4];
static uint32_t lcd_scr_stack_num = 0;

uint8_t* LCD_Push(void)
{
    if (lcd_scr_stack_num >= LCD_BUF_STACK_DEPTH)
        return 0;
    BSP_LCD_CopyActiveLayerTo(&scrBufferStack[lcd_scr_stack_num][0]);
    return &scrBufferStack[lcd_scr_stack_num++][0];
}

void LCD_Pop(void)
{
    if (lcd_scr_stack_num == 0)
        return;
    BSP_LCD_CopyToActiveLayer(&scrBufferStack[--lcd_scr_stack_num][0]);
}
