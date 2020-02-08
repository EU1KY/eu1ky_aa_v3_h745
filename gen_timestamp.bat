@echo off
echo #ifndef BUILD_TIMESTAMP > src/common/build_timestamp.h

git status > NUL
if ERRORLEVEL 1 goto GITERR
echo | set /p TMPHG="#define GITREV " >> src/common/build_timestamp.h
git rev-parse --short=8 HEAD >> src/common/build_timestamp.h

:MAKE_TIMESTAMP
for /F "skip=1 delims=" %%F in ('
    wmic PATH win32_utctime GET Day^,Month^,Hour^,Minute^,Year /FORMAT:TABLE
 ') do (
    for /F "tokens=1-5" %%L in ("%%F") do (
        set CurrDay=0%%L
        set CurrHour=0%%M
        set CurrMinute=0%%N
        set CurrMonth=0%%O
        set CurrYear=%%P))
set CurrDay=%CurrDay:~-2%
set CurrMonth=%CurrMonth:~-2%
set CurrHour=%CurrHour:~-2%
set CurrMinute=%CurrMinute:~-2%

echo #define BUILD_TIMESTAMP "%CurrYear%-%CurrMonth%-%CurrDay% %CurrHour%:%CurrMinute% UT">> src/common/build_timestamp.h
echo #define GITREVSTR(s) stringify_(s) >> src/common/build_timestamp.h
echo #define stringify_(s) #s >> src/common/build_timestamp.h
echo const char * get_revision(void); >> src/common/build_timestamp.h
echo const char * get_build_timestamp(void); >> src/common/build_timestamp.h
echo extern const char VERSION_STRING[]; >> src/common/build_timestamp.h
echo #endif >> src/common/build_timestamp.h
echo src/common/build_timestamp.h file created at %CurrYear%-%CurrMonth%-%CurrDay% %CurrHour%:%CurrMinute% UT
goto END

:GITERR
echo #warning GIT failed. Repository not found. Firmware revision will not be generated. >> src/common/build_timestamp.h
echo #define GITREV N/A >> src/common/build_timestamp.h
goto MAKE_TIMESTAMP

:END
