#ifndef _ADASAPI_H
#define _ADASAPI_H

#define MAX_CAR_NUM 10
#define MAX_PLATE_NUM 5
#define MAX_INDEX_NUM 30

#ifdef __cplusplus
extern "C"
{
#endif

//callbackin����
typedef struct _ADASInData
{
	//ͼ��֡����
	unsigned char *ptrFrameData;

	//GPS����
	unsigned char gpsSpeed;//KM/H  ǧ��ÿСʱ
	unsigned char gpsSpeedEnable;//1-gps�ٶ���Ч  0-gps�ٶ���Ч

	//Gsensor����
	unsigned char GsensorStop;//
	unsigned char GsensorStopEnable;//

	//���ж� ÿ������ռ����λ
	unsigned int sensity;//01

	//CPUCostLevel ÿ������ռ����λ
	unsigned int CPUCostLevel;

	//����ǿ��LV
	unsigned int luminanceValue;

	//����ת����ź�
	unsigned char lightSignal;//0-���ź� 1-��ת��  2-��ת��
	unsigned char lightSignalEnable;

	//�����
	//unsigned int vanishPointX;
	//unsigned int vanishPointY;
	//unsigned char vanishPointEnable;

	//����߶�
	unsigned int cameraHeight;//mm

	/*****************************************************************/

	/******************************************************************/
	//ͼ��֡����
	unsigned char *ptrFrameDataArray;//��ɫͼ������ָ��
	unsigned char dataType;//0-YUV 1-NV12 2-NV21 3-RGB...
	unsigned char cameraType;//����ͷ����(0-ǰ������ͷ 1-��������ͷ 2-�������ͷ 3-�Ҳ�����ͷ...)
	unsigned int imageWidth;
	unsigned int imageHeight;
	unsigned int cameraNum;
	
	//GPS��γ����Ϣ
	//GPS����
	unsigned char Lng;//E W
	float LngValue;
	//GPSά��
	unsigned char Lat;//N S
	float LatValue;

	//GPS����
	float altitude;

	//�豸����(0-�г���¼�� 1-���Ӿ�...)
	unsigned char deviceType;

	//sensor����
	char sensorID[255];

	unsigned char networkStatus;//0-�˿� 1-����
	/******************************************************************/

}ADASInData;


//callbackout����
typedef struct _ADASOutData
{
	//==================================������
	//�汾��Ϣ
	unsigned char *ptrVersion;

	//�󳵵�������
	unsigned int leftLaneLineX0;
	unsigned int leftLaneLineY0;

	unsigned int leftLaneLineX1;
	unsigned int leftLaneLineY1;

	//�ҳ���������
	unsigned int rightLaneLineX0;
	unsigned int rightLaneLineY0;

	unsigned int rightLaneLineX1;
	unsigned int rightLaneLineY1;
	
	//�����߱�����Ϣ
	unsigned char leftLaneLineWarn;//0-��ɫ��δ��⵽�� 1-��ɫ����⵽�� 2-��ɫ��ѹ�߱����� 3-�����±궨����ͷ
	unsigned char rightLaneLineWarn;//0-��ɫ��δ��⵽�� 1-��ɫ����⵽�� 2-��ɫ��ѹ�߱�����3-�����±궨����ͷ

	//�����߻�ͼ��Ϣ===
	int    colorPointsNum;    //��������״��ָ��ĸ���
	unsigned char dnColor;    //�·�һ�����ɫ�� 1-����2-��
	unsigned short rowIndex[MAX_INDEX_NUM];     //��������״��ָ���������
	unsigned short ltColIndex[MAX_INDEX_NUM];   //��������״����߷ָ���������
	unsigned short mdColIndex[MAX_INDEX_NUM];   //��������״���м�ָ���������
	unsigned short rtColIndex[MAX_INDEX_NUM];   //��������״���ұ߷ָ���������
	//�����߻�ͼ��Ϣ==END

	//=================================������Ϣ
	//����λ����Ϣ
	unsigned int carX[MAX_CAR_NUM];
	unsigned int carY[MAX_CAR_NUM];
	unsigned int carW[MAX_CAR_NUM];
	unsigned int carH[MAX_CAR_NUM];

	float carTTC[MAX_CAR_NUM];
	float carDist[MAX_CAR_NUM];

	//����������Ϣ
	unsigned char carWarn[MAX_CAR_NUM];
	int carNum;

	//=================================������Ϣ
	unsigned int plateX[MAX_PLATE_NUM];
	unsigned int plateY[MAX_PLATE_NUM];
	unsigned int plateW[MAX_PLATE_NUM];
	unsigned int plateH[MAX_PLATE_NUM];

	unsigned int plateNum;

	//=================================��ͼλ��
	//int cropEnable;
	//int roiX;
	//int roiY;
	//int roiH;
	//int roiW;
	//===========================================

}ADASOutData;


typedef void (*AdasIn)(ADASInData *ptrADASInData,void *dv);
typedef void (*AdasOut)(ADASOutData *ptrADASOutData,void *dv);

//��ʼ������
typedef struct ADASPara 
{
	//ͼ����,�߶�
	unsigned int frameWidth;
	unsigned int frameHeight;

	//=================================��ͼλ��
	//int cropEnable;
	int roiX;
	int roiY;
	int roiH;
	int roiW;

	//video֡��
	unsigned int fps;

	//camera parameter�������
	unsigned int focalLength;//um
	unsigned int pixelSize;//um
	unsigned int horizonViewAngle;
	unsigned int verticalViewAngle;

	/***ADD***/
	unsigned int vanishX;//�����ʧ������
	unsigned int vanishY;

	unsigned int hoodLineDist;//��ͷ���� cm
	unsigned int vehicleWidth;//������� cm
	unsigned int wheelDist;//���־���  cm
	unsigned int cameraToCenterDist;//����ͷ���� cm

	unsigned int cameraHeight;//����߶� cm
	unsigned int leftSensity;//0-1-2-3
	unsigned int rightSensity;//0-1-2-3
	unsigned int fcwSensity;//0-1-2-3
	
	//��ʼ���ص�����
	AdasIn awAdasIn;
	AdasOut awAdasOut;

}ADASPara;

void AW_AI_ADAS_Init(ADASPara *ptrADASPara,void *dev);

void AW_AI_ADAS_UnInit();

#ifdef __cplusplus
}
#endif


#endif


