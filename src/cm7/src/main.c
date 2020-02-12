/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "shell.h"
#include "aauart.h"
#include "LCD.h"
#include "font.h"
#include "touch.h"
#include "crash.h"
#include "config.h"
#include "mainwnd.h"
#include "dsp.h"
#include "gen.h"
#include "screenshot.h"
#include <memory.h>

volatile uint32_t ReloadFlag = 0;
volatile uint32_t main_sleep_timer = 0;
volatile uint32_t autosleep_timer = 0xFFFFFFFFul;

FATFS MMCFatFs;  // File system object
char MMCPath[4]; // Logical drive path

void Sleep(uint32_t nms)
{
    uart_rx_proc();

    uint32_t ts = CFG_GetParam(CFG_PARAM_LOWPWR_TIME);
    if (ts != 0)
    {
        if (autosleep_timer == 0 &&
                !LCD_IsOff() )
        {
            //TODO: fix it
            //BSP_LCD_DisplayOff();
            //LCD_TurnOff();
        }
    }

    if (nms == 0)
        return;

    if (nms == 1)
    {
        HAL_Delay(1);
        return;
    }

    // Enter sleep mode. The device will wake up on interrupts, and go sleep again
    // after interrupt routine exit. The SysTick_Handler interrupt routine will
    // leave device running when the main_sleep_timer downcount reaches zero,
    // until then the device remains in Sleep state with only interrupts running.
    main_sleep_timer = nms;
    HAL_PWR_EnableSleepOnExit();
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

static void SystemClock_Config(void);
static void CPU_CACHE_Enable(void);
static void MPU_Config(void);

__attribute((unused)) static void _wait_for_cpu2_rdy(void)
{
    // Wait until CPU2 boots and enters in stop mode or timeout
    uint32_t timeout = 0xFFFF;
    while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0));
    if ( timeout < 0 )
    {
        printf("D2 not ready in time\n");
    }
}

// Mount filesystem. Creates it if necessary.
static void _openfs(void)
{
#if FS_ENABLED
    LOCK_HSEM(HSEM_ID_0);
    if (FATFS_LinkDriver(&MMC_Driver, MMCPath) == 0)
    {
        FRESULT fres = f_mount(&MMCFatFs, (TCHAR const*)MMCPath, 1);
        if (FR_NO_FILESYSTEM == fres)
        {
            uint8_t fsWorkBuffer[_MAX_SS];
            fres = f_mkfs(MMCPath, FM_ANY, 0, fsWorkBuffer, sizeof(fsWorkBuffer));
            if (fres != FR_OK)
            {
                UNLOCK_HSEM(HSEM_ID_0);
                CRASH("Creating FileSystem Failed");
            }
            fres = f_mount(&MMCFatFs, (TCHAR const*)MMCPath, 1);
            if (fres != FR_OK)
            {
                UNLOCK_HSEM(HSEM_ID_0);
                CRASHF("File System mount failure after FS creation, err %d", fres);
            }
        }
        else if (FR_OK != fres)
        {
            UNLOCK_HSEM(HSEM_ID_0);
            CRASHF("File System mount failure, err %d", fres);
        }
    }
    else
    {
        UNLOCK_HSEM(HSEM_ID_0);
        CRASH("FS driver link failed");
    }
    UNLOCK_HSEM(HSEM_ID_0);
#endif // #if FS_ENABLED
}

int main(void)
{
    MPU_Config();
    CPU_CACHE_Enable();
    _wait_for_cpu2_rdy();
    HAL_Init();
    SystemClock_Config();
    setvbuf(stdout,NULL,_IONBF,0);
    BSP_LED_Init(LED_GREEN);
    BSP_LED_Init(LED_RED);
    LCD_Init(); // initializes SDRAM
    TOUCH_Init();

    _openfs();

    CFG_Init();
    AAUART_Init();
    GEN_Init();
    DSP_Init();

    autosleep_timer = CFG_GetParam(CFG_PARAM_LOWPWR_TIME);
    if (autosleep_timer != 0 && autosleep_timer < 10000)
    {
        //Disable too low value for autosleep timer. Minimal value is 10 seconds.
        CFG_SetParam(CFG_PARAM_LOWPWR_TIME, 0);
        autosleep_timer = 0;
    }

    shell_set_protocol_group(CFG_GetParam(CFG_PARAM_SHELL_PROTOCOL));

    // Try to display logo file
    if (0 == SCREENSHOT_ShowPNG("/aa/logo.png"))
    {
        Sleep(2000);
        while (TOUCH_IsPressed())
        {
            Sleep(20);
        }
    }

    BSP_LCD_SelectLayer(0);
    LCD_FillAll(LCD_BLACK);
    LCD_ShowActiveLayerOnly();

    MainWnd();

    return 0;
}

void HAL_LTDC_ReloadEventCallback(LTDC_HandleTypeDef *hltdc)
{
    ReloadFlag = 1;
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 400000000 (CM7 CPU Clock)
  *            HCLK(Hz)                       = 200000000 (CM4 CPU, AXI and AHBs Clock)
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2 (APB3 Clock  100MHz)
  *            D2 APB1 Prescaler              = 2 (APB1 Clock  100MHz)
  *            D2 APB2 Prescaler              = 2 (APB2 Clock  100MHz)
  *            D3 APB4 Prescaler              = 2 (APB4 Clock  100MHz)
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 5
  *            PLL_N                          = 160
  *            PLL_P                          = 2
  *            PLL_Q                          = 4
  *            PLL_R                          = 2
  *            VDD(V)                         = 3.3
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;
    HAL_StatusTypeDef ret = HAL_OK;

    /*!< Supply configuration update enable */
    HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);

    /* The voltage scaling allows optimizing the power consumption when the device is
       clocked below the maximum system frequency, to update the voltage scaling value
       regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI48;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

    RCC_OscInitStruct.PLL.PLLM = 5;
    RCC_OscInitStruct.PLL.PLLN = 160;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLQ = 4;

    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
    ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
    if (ret != HAL_OK)
    {
        while (1)
        {
            ;
        }
    }

    /* HSI48 for USB Clock */
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    /* Select PLL as system clock source and configure  bus clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | \
                                   RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
    ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
    if (ret != HAL_OK)
    {
        while (1)
        {
            ;
        }
    }

    __HAL_RCC_CSI_ENABLE() ;
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    HAL_EnableCompensationCell();

    __HAL_RCC_HSEM_CLK_ENABLE();

    // Allocate SRAM memories to both CPUs:
    // keep D2 domain in DRun when CPU1 is in CRun
    __HAL_RCC_D2SRAM1_CLK_ENABLE();
    __HAL_RCC_D2SRAM2_CLK_ENABLE();
    __HAL_RCC_D2SRAM3_CLK_ENABLE();
    // keep D1 domain in DRun when CPU2 is in CRun
    __HAL_RCC_D1SRAM1_C2_ALLOCATE(); // AXI SRAM clock
}

static void CPU_CACHE_Enable(void)
{
    SCB_EnableICache();
    SCB_EnableDCache();
}

static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU attributes as WT for SRAM */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = SDRAM_DEVICE_ADDR;
  MPU_InitStruct.Size = MPU_REGION_SIZE_32MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

// Overrides default HAL_Delay() in order to be used in interrupt-driven USBD code
void HAL_Delay(__IO uint32_t Delay)
{
    while(Delay)
    {
        if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
        {
            Delay--;
        }
    }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    static uint32_t in_assert = 0;
    if (in_assert)
    {
        return;
    }
    in_assert = 1;
    CRASHF("ASSERT @ %s:%lu", file, line);
}
#endif

void CRASH(const char *text)
{
    if (LCD_IsOff())
    {
        BSP_LCD_DisplayOn();
    }
    BSP_LED_On(LED_RED);
    BSP_LCD_SelectLayer(0);
    FONT_Write(FONT_FRAN, LCD_RED, LCD_BLACK, 0, 0, text);
    FONT_Write(FONT_FRAN, LCD_RED, LCD_BLACK, 0, 14, "SYSTEM HALTED ");
    LCD_ShowActiveLayerOnly();
    printf("\nSYSTEM HALTED: %s\n", text);
    for(;;);
}

/* RESET shell command */
bool shell_reset(uint32_t argc, char* const argv[])
{
    (void)argc;
    (void)argv;
    printf("RESET\n");
    NVIC_SystemReset();
    return true;
}
SHELL_CMD_PROTO(reset, "Full device reset", shell_reset, SHELL_PROT_GROUP_ALL);

#if FS_ENABLED
// Filesystem related shell commands
static const char * const ff_error_str[] =
{
    "FR_OK",                    /* (0) Succeeded */
    "FR_DISK_ERR",              /* (1) A hard error occurred in the low level disk I/O layer */
    "FR_INT_ERR",               /* (2) Assertion failed */
    "FR_NOT_READY",             /* (3) The physical drive cannot work */
    "FR_NO_FILE",               /* (4) Could not find the file */
    "FR_NO_PATH",               /* (5) Could not find the path */
    "FR_INVALID_NAME",          /* (6) The path name format is invalid */
    "FR_DENIED",                /* (7) Access denied due to prohibited access or directory full */
    "FR_EXIST",                 /* (8) Access denied due to prohibited access */
    "FR_INVALID_OBJECT",        /* (9) The file/directory object is invalid */
    "FR_WRITE_PROTECTED",       /* (10) The physical drive is write protected */
    "FR_INVALID_DRIVE",         /* (11) The logical drive number is invalid */
    "FR_NOT_ENABLED",           /* (12) The volume has no work area */
    "FR_NO_FILESYSTEM",         /* (13) There is no valid FAT volume */
    "FR_MKFS_ABORTED",          /* (14) The f_mkfs() aborted due to any problem */
    "FR_TIMEOUT",               /* (15) Could not get a grant to access the volume within defined period */
    "FR_LOCKED",                /* (16) The operation is rejected according to the file sharing policy */
    "FR_NOT_ENOUGH_CORE",       /* (17) LFN working buffer could not be allocated */
    "FR_TOO_MANY_OPEN_FILES",   /* (18) Number of open files > _FS_LOCK */
    "FR_INVALID_PARAMETER"      /* (19) Given parameter is invalid */
};

bool _shell_cmd_cd(uint32_t argc, char* const argv[])
{
    if (argc == 1)
    {
        return false;
    }

    FRESULT fr = f_chdir(argv[1]);
    if (FR_OK == fr)
    {
        char path[_MAX_LFN + 1];
        path[0] = '\0';
        f_getcwd(path, sizeof(path));
        printf("%s\n", path);
    }
    else
    {
        printf("f_chdir error %d (%s)\n", fr, ff_error_str[fr]);
    }
    return false;
}
SHELL_CMD(cd, "Change working directory", _shell_cmd_cd);

bool _shell_cmd_pwd(uint32_t argc, char* const argv[])
{
    char path[_MAX_LFN + 1];
    path[0] = '\0';
    f_getcwd(path, sizeof(path));
    printf("Current dir: %s\n", path);
    return false;
}
SHELL_CMD(pwd, "Print working directory", _shell_cmd_pwd);

bool _shell_cmd_mkdir(uint32_t argc, char* const argv[])
{
    if (argc < 2)
    {
        printf("Create subdirectory relative to the current directory.\n    Usage: mkdir <dirname>\n");
        return false;
    }
    FRESULT fr = f_mkdir(argv[1]);
    if (FR_OK == fr)
    {
        printf("%s created\n", argv[1]);
    }
    else
    {
        printf("f_mkdir error %d (%s)\n", fr, ff_error_str[fr]);
    }
    return false;
}
SHELL_CMD(mkdir, "Make directory", _shell_cmd_mkdir);

bool _shell_cmd_rm(uint32_t argc, char* const argv[])
{
    if (argc < 2)
    {
        printf("Delete file or subdirectory in the current directory.\n    Usage: rm <path>\n");
        return false;
    }

    FRESULT fr = f_unlink(argv[1]);
    if (FR_OK == fr)
    {
        printf("%s deleted\n", argv[1]);
    }
    else
    {
        printf("f_unlink error %d (%s) deleting %s\n", fr, ff_error_str[fr], argv[1]);
    }
    return false;
}
SHELL_CMD(rm, "Delete file or directory", _shell_cmd_rm);

bool _shell_cmd_ls(uint32_t argc, char* const argv[])
{
    DIR dir = { 0 };
    FILINFO fno = { 0 };
    char fs_current_dir[_MAX_LFN+1];

    FRESULT fr = f_getcwd(fs_current_dir, sizeof(fs_current_dir));
    if (FR_OK != fr)
    {
        printf("f_getcwd error %d (%s)\n", fr, ff_error_str[fr]);
        return false;
    }

    fr = f_opendir(&dir, fs_current_dir);
    if (FR_OK != fr)
    {
        printf("f_opendir %d (%s) : %s\n", fr, ff_error_str[fr], fs_current_dir);
        return false;
    }

    printf("%s :\n", fs_current_dir);
    uint32_t files = 0;
    uint32_t filesizes = 0;
    uint32_t directories = 0;
    for(;;)
    { // Iterate through the directory
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK || !fno.fname[0])
            break; //Nothing to do
        if (fno.fattrib & AM_DIR)
        {
            printf("  <DIR>             %s\n", fno.fname);
            ++directories;
        }
        else
        {
            printf("      %12u          %s\n", (unsigned int)fno.fsize, fno.fname);
            ++files;
            filesizes += fno.fsize;
        }
    }
    f_closedir(&dir);
    printf("    %6u Files     %u bytes\n", (unsigned int)files, (unsigned int)filesizes);
    printf("    %6u Directories\n", (unsigned int)directories);
    return false;
}
SHELL_CMD(ls, "list contents of the current directory", _shell_cmd_ls);

#endif //#if FS_ENABLED
