/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/
 
#ifndef __ALETAS3_IMX206_H__
#define __ALETAS3_IMX206_H__

#ifdef __cplusplus
extern "C" {
#endif

//Parameter

typedef struct {
	unsigned ChipID		:8;
	unsigned Addr_H		:8;
	unsigned Addr_L		:8;
	unsigned Data		:8;
}IMX206_struct;

IMX206_struct	IMX206_FS_CMD_Table[] =
{
    {0x81,0x00,0x00,0x02},		// 1
    {0x81,0x05,0xA2,0x10},              // 2
 //Wait > 1 ms
    {0x81,0x00,0x00,0x00},              // 3
 //Wait > 16 ms
    {0x81,0x00,0x2c,0x00},              // 4
    {0x81,0x01,0x0e,0x11},              // 5
    {0x81,0x03,0x08,0x13},              // 6
    {0x81,0x03,0x09,0x10},              // 7
    {0x81,0x03,0x0a,0x11},              // 8
    {0x81,0x03,0x20,0x14},              // 9
    {0x81,0x03,0x21,0x01},              //10
    {0x81,0x03,0x22,0x18},              //11
    {0x81,0x03,0x23,0x00},              //12
    {0x81,0x03,0x38,0x00},              //13
    {0x81,0x03,0x42,0x06},              //14
    {0x81,0x03,0x43,0x00},              //15
    {0x81,0x03,0x44,0x13},              //16
    {0x81,0x03,0x45,0x01},              //17
    {0x81,0x03,0x4a,0x12},              //18
    {0x81,0x03,0x4b,0x01},              //19
    {0x81,0x03,0x4c,0x07},              //20
    {0x81,0x03,0x4d,0x00},              //21
    {0x81,0x03,0x52,0x00},              //22
    {0x81,0x03,0x53,0x00},              //23
    {0x81,0x03,0x54,0x00},              //24
    {0x81,0x03,0x59,0x11},              //25
    {0x81,0x03,0x5f,0xff},              //26
    {0x81,0x03,0x60,0x01},              //27
    {0x81,0x03,0x61,0x00},              //28
    {0x81,0x03,0x62,0x00},              //29
    {0x81,0x04,0x06,0x7e},              //30
    {0x81,0x04,0x07,0x00},              //31
    {0x81,0x04,0x8b,0x01},              //32
    {0x81,0x05,0x06,0x1b},              //33
    {0x81,0x05,0x07,0x00},              //34
    {0x81,0x05,0x0e,0xb7},              //35
    {0x81,0x05,0x0f,0x00},              //36
    {0x81,0x05,0x3e,0x0c},              //37
    {0x81,0x05,0x3f,0x08},              //38
    {0x81,0x05,0x40,0x03},              //39
    {0x81,0x05,0x42,0x01},              //40
    {0x81,0x05,0x5b,0x00},              //41
    {0x81,0x05,0x78,0xff},              //42
    {0x81,0x05,0x79,0x01},              //43
    {0x81,0x05,0x7a,0x00},              //44
    {0x81,0x05,0x7b,0x00},              //45
    {0x81,0x05,0x7d,0x08},              //46
 //Mode                                                       
    {0x81,0x00,0x7c,0xf0},              //60
    {0x81,0x00,0x7d,0x03},               //61      
    {0x81,0x00,0x02,0x01},               //61
    {0x81,0x00,0x01,0x11},               //61
    {0x81,0x00,0x03,0x33},              //47
    {0x81,0x00,0x04,0x00},              //48
    {0x81,0x00,0x05,0x03},              //49
    {0x81,0x00,0x06,0x30},              //50
    {0x81,0x00,0x07,0x00},              //51
    {0x81,0x00,0x08,0x00},              //52
    {0x81,0x00,0x0d,0x00},              //53
    {0x81,0x00,0x0e,0x00},              //54
    {0x81,0x00,0x1a,0x00},              //55
    {0x81,0x00,0x6f,0x00},              //56
    {0x81,0x00,0x70,0x00},              //57
    {0x81,0x00,0x71,0x00},              //58
    {0x81,0x00,0x72,0x00},              //57
    {0x81,0x00,0x0B,0xEF},              //58
    {0x81,0x00,0x0C,0x0D},              //57
    {0x81,0x00,0xFC,0x01},              //58
    {0x81,0x00,0xF8,0x08},              //58
    {0x81,0x00,0xF9,0x00},              //58
    {0x81,0x00,0xFA,0x38},              //58
    {0x81,0x00,0xFB,0x12}
};

#ifdef __cplusplus
}   // extern "C"
#endif

#endif    // __ALETAS3_IMX206_H__









