#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include "Device/US363/Util/string_util.h"


int stringIsNumber(char *str) {
	int i;
	int len = strlen(str);
	for(i = 0; i < len; i++) {
		if(isdigit(str[i]) == 0) {
			return STRING_NOT_NUMBER;
		}
	}
	return STRING_IS_NUMBER;
}