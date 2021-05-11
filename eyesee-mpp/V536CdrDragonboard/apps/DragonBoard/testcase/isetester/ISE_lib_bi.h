
#ifndef _ISE_LIB_BI_H_
#define _ISE_LIB_BI_H_

#include "ISE_common.h"

/*****************************************************************************
* �����������ò���
* in_h                  ԭͼ�ֱ���h��4�ı���
* in_w                  ԭͼ�ֱ���w��4�ı���
* pano_h            	ȫ��ͼ�ֱ���h��4�ı�������С320�����4032��
* pano_w           	    ȫ��ͼ�ֱ���w��8�ı�������С320�����8064��
* in_luma_pitch         ԭͼ����pitch
* in_chroma_pitch       ԭͼɫ��pitch
* pano_luma_pitch       ȫ��ͼ����pitch
* pano_chroma_pitch     ȫ��ͼɫ��pitch
* reserved          	�����ֶΣ��ֽڶ���
*******************************************************************************/
typedef struct _ISE_CFG_PARA_BI_{
	int                      in_h;
	int                      in_w;
	int						 in_luma_pitch;
	int						 in_chroma_pitch;
	int                      in_yuv_type;
	int						 out_en[1+MAX_SCALAR_CHNL];
	int                      out_h[1+MAX_SCALAR_CHNL];
	int                      out_w[1+MAX_SCALAR_CHNL];
	int						 out_flip[1+MAX_SCALAR_CHNL];
	int					     out_mirror[1+MAX_SCALAR_CHNL];
	int						 out_luma_pitch[1+MAX_SCALAR_CHNL];
	int						 out_chroma_pitch[1+MAX_SCALAR_CHNL];
	int                      out_yuv_type;
	float					 p0;
	int					     cx0;
	int					     cy0;
  	float                    p1;
	int						 cx1;
	int						 cy1;
	double					 calib_matr[3][3];
	double					 calib_matr_cv[3][3];
	double					 distort[8];
	char                     reserved[32];
}ISE_CFG_PARA_BI;

/******************************************************************************
* �������������
* in_luma                     ԭʼͼ��ָ�����飬���ȷ�����nv12����nv16��ʽ
* in_chroma                   ԭʼͼ��ָ�����飬ɫ�ȷ�����nv12����nv16��ʽ
*******************************************************************************/
typedef struct _ISE_PROCIN_PARA_BI_{
	myAddr			 *in_luma[2];	   // PIM_PROC_CALC_PANO����
	myAddr			 *in_chroma[2];    // PIM_PROC_CALC_PANO����
	char			 reserved[32];
}ISE_PROCIN_PARA_BI;

/******************************************************************************
* �������������
* pano_luma            ȫ��ͼ�����ȷ�����nv12����nv16��ʽ
* pano_chroma          ȫ��ͼ��ɫ�ȷ�����nv12����nv16��ʽ
*******************************************************************************/
typedef struct _ISE_PROCOUT_PARA_BI_{
	myAddr			 *out_luma[1+MAX_SCALAR_CHNL];
	myAddr			 *out_chroma_u[1+MAX_SCALAR_CHNL];
	myAddr			 *out_chroma_v[1+MAX_SCALAR_CHNL];
	char			 reserved[32];
}ISE_PROCOUT_PARA_BI;


// ��ʱ��ʾ����ȫ��װ��������
int myMalloc(int fion, myAddr *addr, unsigned long numOfBytes );
void myMemSet(int fion, myAddr *addr, unsigned char value);
void myGetData(const char *path, myAddr *addr);
int myFree(int fion, myAddr *addr);

// �ӿں���
ISE_HANDLE_BI *ISE_Create_Bi(ISE_CFG_PARA_BI *ise_cfg);

int ISE_SetAttr_Bi(ISE_HANDLE_BI *handle);

int ISE_Proc_Bi(
	ISE_HANDLE_BI 		*handle,
	ISE_PROCIN_PARA_BI 	*ise_procin, 
	ISE_PROCOUT_PARA_BI *ise_procout);

int ISE_CheckResult_Bi(
	ISE_HANDLE_BI 		*handle, 
	ISE_PROCOUT_PARA_BI *ise_procout);

int ISE_Destroy_Bi(ISE_HANDLE_BI *handle);

#endif


