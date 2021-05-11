#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <dirent.h>

#include "Device/US363/Data/us363_folder.h"
#include "Device/US363/Util/file_util.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::US363FileSystem"


int makeUS360Folder()
{
	return makeFolder(US360_FOLDER_PATH);
}

int makeTestFolder()
{
	return makeFolder(TEST_FOLDER_PATH);
}