/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __SPI_CMD_H__
#define __SPI_CMD_H__

#ifdef __cplusplus
extern "C" {
#endif

#define FX_SPEED_DEFAULT 0xA00
#define F2_SPEED_DEFAULT 0xD00

#define HDR_IMG_SIZE_X 	576
#define HDR_IMG_SIZE_Y 	416

#define DDR_CHECKSUM_TIMEOUT	5

extern int FPGA_Sleep_En;

extern unsigned ST_Cmd_Cal_CheckSum[2];
extern unsigned ST_Sen_Cmd_Cal_CheckSum[2];
extern unsigned ST_Tran_Cmd_Cal_CheckSum[2];
extern unsigned MP_B_Table_Cal_CheckSum[2][2];
extern unsigned Sen_Lens_Adj_Cal_CheckSum[3][5];

void writeSPIUSB();
int readCmdIdx();
void RXDelaySet(int TValue, int ChSel, int PSel);
void AS2_Write_F2_to_FX_CMD(unsigned S_Addr, unsigned T_Addr, unsigned size, unsigned mode, unsigned id, unsigned en);
void readIMCPdata(int M_Mode, int IMCP_Idx);
int readSensorState();
void setFPGASpeed(int f_id, int speed);
int getFPGASpeed(int f_id);
void ReadFPGASpeed();
void SetSaturationValue(int idx, int value);
int GetSaturationParameter(int idx);
void setSensorPowerOff();
void Read_HDR_Img(int cap_cnt);
void ReadFXDDR(unsigned f_id, unsigned addr);
void SetFPGASleepEn(int en);
int GetFPGASleepEn();
void Check_ST_Cmd_DDR_CheckSum();
void Check_ST_Sensor_Cmd_DDR_CheckSum();
void Check_ST_Tran_Cmd_DDR_CheckSum();
void Check_MP_B_Table_DDR_CheckSum(int idx);
void Check_Sen_Lens_Adj_DDR_CheckSum(int s_id);
void CheckSTCmdDDRCheckSum();
int CheckFXCnt(void);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__SPI_CMD_H__