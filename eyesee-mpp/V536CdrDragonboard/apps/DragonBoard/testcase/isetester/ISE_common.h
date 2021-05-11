#ifndef _ISE_COMMON_H_
#define _ISE_COMMON_H_

#define MAX_SCALAR_CHNL     3        // scalar���·��

#ifndef PLANE_YUV
#define YUV420		0
#define YVU420		1
#define YUV420P		2
#define YUV422		4
#define YVU422		5
#define YUV422P		6
#endif


// ��ǰ�汾��
#define ISE_MAJOR_VER   	1         // ���汾�ţ����63
#define ISE_SUB_VER         0         // �Ӱ汾�ţ����31
#define ISE_REV_VER         0         // �����汾�ţ����31

// �汾����
#define ISE_VER_YEAR		16
#define ISE_VER_MONTH       1
#define ISE_VER_DAY			9

// mem tab
#define ISE_MTAB_NUM        1

/**�ڴ���뷽ʽ**/
typedef enum _ISE_MEM_ALIGNMENT_
{
	ISE_MEM_ALIGN_4BYTE   = 4,
	ISE_MEM_ALIGN_8BYTE   = 8,
	ISE_MEM_ALIGN_16BYTE  = 16,
	ISE_MEM_ALIGN_32BYTE  = 32,
	ISE_MEM_ALIGN_64BYTE  = 64,
	ISE_MEM_ALIGN_128BYTE = 128,
	ISE_MEM_ALIGN_256BYTE = 256
}ISE_MEM_ALIGNMENT;

/** �ڴ����ͱ�־**/
typedef enum _ISE_MEM_ATTRS_
{
	ISE_MEM_SCRATCH,                                // �ɸ���
	ISE_MEM_PERSIST                                 // ���ɸ���
} ISE_MEM_ATTRS;

typedef enum _ISE_MEM_SPACE_
{
	ISE_MEM_EXTERNAL_PROG,                          // �ⲿ����洢��
	ISE_MEM_INTERNAL_PROG,                          // �ڲ�����洢��
	ISE_MEM_EXTERNAL_TILERED_DATA,					// �ⲿTilered���ݴ洢��
	ISE_MEM_EXTERNAL_CACHED_DATA,					// �ⲿ��Cache�洢��
	ISE_MEM_EXTERNAL_UNCACHED_DATA,					// �ⲿ����Cache�洢��
	ISE_MEM_INTERNAL_DATA,                          // �ڲ��洢��
} ISE_MEM_SPACE;

// ���ò�������
typedef enum _ISE_SET_CFG_TYPE_
{
	ISE_SET_CFG_STRUCT_PARAM    =    0x0001,        // ��һ��������
} ISE_SET_CFG_TYPE;

// ��ȡ��������
typedef enum _ISE_GET_CFG_TYPE_
{
	ISE_GET_CFG_SINGLE_PARAM    =    0x0001,        // ��������
	ISE_GET_CFG_VERSION         =    0x0002         // �汾��Ϣ
} ISE_GET_CFG_TYPE;

// �������ʶ
typedef enum _ISE_PROC_TYPE_
{
	ISE_PROC_CALC_MO          =    0x0001,
	ISE_PROC_CALC_BI          =    0x0002,        // ˫Ŀ����ȫ��ͼ
	ISE_PROC_CALC_STI         =    0x0001,
} ISE_PROC_TYPE;


/*��ַ�ṹ��*/
typedef struct _myAddr_
{
	void* 	phy_Addr;
	void* 	mmu_Addr;
	void* 	ion_handle; 	// handle to the ion heap
	int 	fd; 			// file to the vsual address
	unsigned int length;
}myAddr;


/*****************************************************************************
* �㷨�ⵥһ���ò����ṹ��
* index                ��ֵ����, ��д AVM_SINGLE_PARAM_KEY ��������
* val_int              ���Ͳ��������Ҫ���õĲ����Ǹ����ͣ����������д
* val_float            �����Ͳ��������Ҫ���õĲ��������ͣ����������д
******************************************************************************/
typedef struct _ISE_SINGLE_PARAM_
{
	int                      index;
	int                      val_int;
	float                    val_float;
} ISE_SINGLE_PARAM;

typedef void* ISE_HANDLE_BI;
typedef void* ISE_HANDLE_MO;


#endif
