#include "build_timestamp.h"
#include "config.h"

const char * get_revision(void)
{
    static const char _gitrev[] = GITREVSTR(GITREV);
    return _gitrev;
}

const char * get_build_timestamp(void)
{
    static const char _buildts[] = BUILD_TIMESTAMP;
    return _buildts;
}

const char VERSION_STRING[] = "EU1KY AA v." AAVERSION ", rev: " GITREVSTR(GITREV) ", Build: " BUILD_TIMESTAMP;
