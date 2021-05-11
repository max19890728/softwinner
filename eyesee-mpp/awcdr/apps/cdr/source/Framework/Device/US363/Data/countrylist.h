/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __COUNTRYLIST_H__
#define __COUNTRYLIST_H__

#ifdef __cplusplus
extern "C" {
#endif

int GetCountryListNum(void);
int GetCountryListFreq(void);
int GetCountryListLang(void);
int CheckCountryFreq(int country);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__COUNTRYLIST_H__
