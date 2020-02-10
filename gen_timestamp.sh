#!/bin/bash

set -e

function write_timestamp {
    CurrYear=`date -u +%Y`
    CurrMonth=`date -u +%m`
    CurrDay=`date -u +%d`
    CurrHour=`date -u +%H`
    CurrMinute=`date -u +%M`

    echo \#define BUILD_TIMESTAMP \"${CurrYear}-${CurrMonth}-${CurrDay} ${CurrHour}:${CurrMinute} UT\" >> src/common/build_timestamp.h
    echo \#define GITREVSTR\(s\) stringify_\(s\) >> src/common/build_timestamp.h
    echo \#define stringify_\(s\) \#s >> src/common/build_timestamp.h
    echo const char \* get_revision\(void\)\; >> src/common/build_timestamp.h
    echo const char \* get_build_timestamp\(void\)\; >> src/common/build_timestamp.h
    echo extern const char VERSION_STRING\[\]\; >> src/common/build_timestamp.h
    echo \#endif >> src/common/build_timestamp.h
    echo src/common/build_timestamp.h file created at ${CurrYear}-${CurrMonth}-${CurrDay} ${CurrHour}:${CurrMinute} UT
}

function write_git_error {
    echo #warning GIT failed. Repository not found. Firmware revision will not be generated. >> src/common/build_timestamp.h
    echo #define GITREV N/A >> src/common/build_timestamp.h
}

function write_header {
    echo \#ifndef BUILD_TIMESTAMP > src/common/build_timestamp.h
    git status &> /dev/null
    if [ $? -ne 0 ]; then
	echo Failed to execute git status
	return 1
    fi

    GITREV="$(git rev-parse --short=8 HEAD)"
    echo "#define GITREV \"${GITREV}\"" >> src/common/build_timestamp.h
    return 0
}

write_header
if [ $? -eq 0 ]; then
    write_timestamp
else
    echo Failed to execute git status
    write_git_error
    write_timestamp
fi
