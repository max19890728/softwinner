/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/
 
#ifndef __ALETAS3_REGISTER_MAP_H__
#define __ALETAS3_REGISTER_MAP_H__

#ifdef __cplusplus
extern "C" {
#endif

//Parameter

//Write
#define	SPDDR_ADDR	0x00004
#define	MDDR_ADDR	0x00008
#define	VST_ADDR	0x0000C
#define	VMAX_ADDR	0x00010
#define	HMAX_ADDR	0x00014
#define	SOFTNUM_ADDR	0x00018
#define	SRST_ADDR	0x0001C
#define	VLWIDTH_ADDR	0x00020
#define	HLWIDTH_ADDR	0x00024
#define	CISPSEL_ADDR	0x00028
#define	CISCMDW_ADDR	0x0002C
#define	MSTART_ADDR	0x0003F

#define BASE_CLK	100 //MHz
#define SYNC_TIME       100 //ms     
#define VST_TIME        10 //ms

#define SENSOR_ADDR     0x2000000
#define MAIN_ADDR       0x2100000
#define	SPDDR_DATA	(SENSOR_ADDR >> 5)
#define	MDDR_DATA	(MAIN_ADDR >> 5)
#define	VST_DATA	(BASE_CLK * VST_TIME * 1000 - 1)
#define	VMAX_DATA	(BASE_CLK * SYNC_TIME * 1000 - 1)
#define	HMAX_DATA	((VMAX_DATA + 1) / 3571 + 2)
#define	SOFTNUM_DATA	63
#define	SRST_DATA	1
#define	VLWIDTH_DATA	12
#define	HLWIDTH_DATA	12

#define CIS_PORT_SEL	0
#define CIS_I2C_SPI_SEL	0
#define CIS_SPI_SPEED	0    
#define CIS_I2C_SPEED	0    

#define	CISPSEL_DATA	((CIS_I2C_SPEED << 7) | (CIS_SPI_SPEED << 5) | (CIS_I2C_SPI_SEL << 4) | CIS_PORT_SEL)

#define	CMD_START_EN    1
#define	CISCMDEN	1   
#define CMD_MAX		3       

#define	MSTART_DATA	((CMD_MAX << 8) | (CISCMDEN << 1) | CMD_START_EN)


//Read Sequence
#define	I2CRD_ADDR	0x10000


#ifdef __cplusplus
}   // extern "C"
#endif

#endif    // __ALETAS3_REGISTER_MAP_H__










