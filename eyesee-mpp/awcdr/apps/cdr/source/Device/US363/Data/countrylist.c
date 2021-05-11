/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Data/countrylist.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#include "Device/US363/Data/databin.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::CountryList"


const char countryListPath[64] = "/usr/share/minigui/res/data/CountryList.csv\0";

int CountryNum  = 0;
int CountryFreq = 60;		//60:60Hz 50:50Hz
int CountryLang = 3;		//0:繁中 1:日文 2:簡中 3:英文

int GetCountryListNum(void) {
	return CountryNum;
}
int GetCountryListFreq(void) {
	return CountryFreq;
}
int GetCountryListLang(void) {
	return CountryLang;
}

int CheckCountryFreq(int country) {
	FILE *fp;
	char *ptr, *tmp;
	struct stat sti;
	int num, freq, lang;
	char line_str[64];
	int count = 0, size;
	char *tokens[5];
	char *del = "\\,";

    if (-1 == stat (countryListPath, &sti)) {
        db_error("CheckCountryFreq() file not find\n");
        return -1;
    }
    else {
    	fp = fopen(countryListPath, "rt");
    	if(fp != NULL) {
    		while(!feof(fp) ) {
    			memset(&line_str[0], 0, sizeof(line_str) );
    			fgets(line_str, sizeof(line_str), fp);
    			//db_debug("GetCountryFreq() 02 count=%d line_str=%s\n", count, line_str);

    			if(count >= 0) {
    				size = split_c(tokens, line_str, del);
    				num  = atoi(tokens[0]);
    				freq = atoi(tokens[1]);
    				lang = atoi(tokens[2]);
    				if(num == country) {
    					CountryNum  = num;
    					CountryFreq = freq;
    					CountryLang = lang;
    					db_debug("CheckCountryFreq() 01 country=%d count=%d num=%d freq=%d lang=%d\n", country, count, CountryNum, CountryFreq, CountryLang);
    					break;
    				}
    			}
				count++;
    		}
    		fclose(fp);
    	}
    }

	if(CountryFreq != 60 && CountryFreq != 50)
		CountryFreq = 60;

    return CountryFreq;
}
