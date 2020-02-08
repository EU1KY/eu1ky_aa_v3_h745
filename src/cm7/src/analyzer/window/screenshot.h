/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#ifndef SCREENSHOT_H_
#define SCREENSHOT_H_

#if FS_ENABLED
#include "ff.h"
#endif

void SCREENSHOT_Show(const char* fname);
void SCREENSHOT_SelectFileName(char* fname);
void SCREENSHOT_DeleteOldest(void);
void SCREENSHOT_Save(const char *fname);
void SCREENSHOT_SavePNG(const char *fname);

#endif
