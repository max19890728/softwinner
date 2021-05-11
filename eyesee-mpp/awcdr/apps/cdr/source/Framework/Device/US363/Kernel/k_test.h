/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/
 
#ifndef __K_TEST_H__
#define __K_TEST_H__

#ifdef __cplusplus
extern "C" {
#endif

struct US360_Stitching_Table {
  unsigned		CB_DDR_P        :24;	// unit: burst
  unsigned  	CB_Sensor_ID	:3;
  unsigned  	CB_Alpha_Flag	:1;
  unsigned  	CB_Block_ID		:3;
  unsigned  	CB_Mask			:1;
  unsigned  	CB_Posi_X0     	:16;
  unsigned     	CB_Posi_Y0      :16;
  unsigned     	CB_Posi_X1      :16;
  unsigned     	CB_Posi_Y1      :16;
  unsigned     	CB_Posi_X2      :16;
  unsigned     	CB_Posi_Y2      :16;
  unsigned     	CB_Posi_X3      :16;
  unsigned     	CB_Posi_Y3      :16;
  unsigned     	CB_Adj_Y0       :8;
  unsigned     	CB_Adj_Y1       :8;
  unsigned     	CB_Adj_Y2       :8;
  unsigned     	CB_Adj_Y3       :8;
  unsigned     	CB_Adj_U0       :8;
  unsigned     	CB_Adj_U1       :8;
  unsigned     	CB_Adj_U2       :8;
  unsigned     	CB_Adj_U3       :8;
  unsigned     	CB_Adj_V0       :8;
  unsigned     	CB_Adj_V1       :8;
  unsigned     	CB_Adj_V2       :8;
  unsigned     	CB_Adj_V3       :8;
};

struct S2_Focus_XY_Struct {
	int X;
	int Y;
};
extern struct S2_Focus_XY_Struct Foucs_XY[4][5][4];

void Set_Focus_XY(int tool_id, int s_id, int idx, int X, int Y);
void Get_Focus_XY(int tool_id, int s_id, int idx, int *X, int *Y);
void Set_Focus_XY_Offset(int s_id, int xy, int offset);
void Get_Focus_XY_Offset(int s_id, int *offsetX, int *offsetY);
void Get_Focus_Degree(int s_id, int *degree);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif  // __K_TEST_H__
