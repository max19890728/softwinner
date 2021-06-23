#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include "Device/US363/Util/ux360_byteutil.h"
#include "Device/US363/System/sys_time.h"


/**
  * 整數轉換為4位元位元組陣列
  * @param intValue
  * @return
  */
void int2Byte(int intValue, char *buf) {
	int i;
    for (i = 0; i < 4; i++) {
        buf[i] = (char) (intValue >> 8 * (3 - i) & 0xFF);
    }
}

/**
  * 4位元位元組陣列轉換為整數
  * @param b
  * @return
  */
int byte2Int(char *buf, int size) {
    int i, intValue = 0;
    for (i = 0; i < size; i++) {
        intValue += (buf[i] & 0xFF) << (8 * (3 - i));
    }
    return intValue;
}
    
/**
  * 8位元位元組陣列轉換為長整數
  * @param b
  * @return
  */
long byte2Long(char *buf, int size) {
	long i, intValue = 0;
    for (i = 0; i < size; i++) {
		intValue = (intValue << 8) + (buf[i] & 0xff);
    }
    return intValue;
}

/**
  * 8位元位元組陣列轉換為Double
  * @param b
  * @return
  */
double byte2Double(char *buf, int size) {
	double value = 0;
	memcpy(&value, buf, size);
    return value;
}

/**
  * 32位元交換
  * @param b
  * @return
  */
void swap32(unsigned int *val) {
	*val = ((*val>>24) | ((*val&0x00FF0000)>>8) | ((*val&0x0000FF00)<<8) | (*val<<24));
}