/*
*��Ȩ����:����
*�ļ�����:fileLockManager.h
*������:����
*��������:2018-5-23
*�ļ�����:���ļ���Ҫ�����ļ������������������߼�ʵ��
*��ʷ��¼:��
*/

#ifndef __FILE_LOCK_MANAGER_H__
#define __FILE_LOCK_MANAGER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <utils/Mutex.h>
#include "AdapterLayer.h"
#include "device_model/media/media_file_manager.h"
#include <pthread.h>

using namespace EyeseeLinux;

class FileLockManager
{
	public:
		FileLockManager();
		~FileLockManager();
	public:
		static FileLockManager* GetInstance(void);

		/*
		*����: int setLockFileByTime(const string p_StartTime, const string p_StopTime, const string p_LockTime, int p_CamId)
		*����: ����ָ��ʱ���������ļ�������������������ʱ��
		*����: 
		*	p_StartTime: ��ʼʱ��
		*   p_StopTime: ����ʱ��
		*   p_LockTime: ����ʱ��
		*   p_CamId:ָ��ǰ·���Ǻ�·����ͷ
		*   p_OrderId: ������
		*����ֵ:
		*	0:�ɹ�
		*	1:ʧ��
		*�޸�: ����2018/5/17
		*/
		int setLockFileByTime(const std::string p_StartTime, const std::string p_StopTime, const std::string p_LockTime, int p_CamId, std::string p_OrderId);

		/*
		*����: int getLockFileByTimeResult(std::vector<LockFileInfo> &p_fileLockInfo)
		*����: ��ȡ�����ļ����
		*����:
		*	p_fileLockInfo ���
		*����ֵ:
		*	0:�ɹ�
		*	1:ʧ��
		*�޸�: ����2018/5/17
		*/
		int getLockFileByTimeResult(std::vector<LockFileInfo> &p_fileLockInfo);

		/*
		*����: int setUnLockFileByTime(const string p_StartTime, const string p_StopTime, int p_CamId)
		*����: ����ָ��ʱ���������ļ�
		*����: 
		*	p_StartTime: ��ʼʱ��
		*   p_StopTime: ����ʱ��
		*   p_CamId: ����ͷID 0:ǰ�� 1:����
		*   p_OrderId: ������
		*����ֵ:
		*	0:�ɹ�
		*	1:ʧ��
		*�޸�: ����2018/5/17
		*/
		int setUnLockFileByTime(const std::string p_StartTime, const std::string p_StopTime, int p_CamId, std::string p_OrderId);

		/*
		*����: int getUnLockFileByTimeResult(std::vector<LockFileInfo> &p_fileUnLockInfo)
		*����: ��ȡ�����ļ����
		*����:
		*	p_fileUnLockInfo  ���
		*����ֵ:
		*	0:�ɹ�
		*	1:ʧ��
		*�޸�: ����2018/5/17
		*/
		int getUnLockFileByTimeResult(std::vector<LockFileInfo> &p_fileUnLockInfo);


		/*
		*����: int setLockFileByTime(const string p_FileName, const string p_LockTime, int p_CamId)
		*����: ����ָ���ļ�������������������ʱ��
		*����: 
		*	p_FileName: �����ļ����ļ���
		*   p_LockTime: ����ʱ��
		*   p_CamId:  ����ͷID 0:ǰ�� 1:����
		*����ֵ:
		*	0:�ɹ�
		*	1:ʧ��
		*�޸�: ����2018/5/17
		*/
		int setLockFileByName(const std::string p_FileName, const std::string p_LockTime, int p_CamId);

		/*
		*����: int  setUnLockFileByName(const string p_FileName, int p_CamId)
		*����: ָ���ļ�����
		*����: 
		*	p_FileName: �����ļ����ļ���
		*   p_CamId:  ����ͷID 0:ǰ�� 1:����
		*����ֵ:
		*	0:�ɹ�
		*	1:ʧ��
		*�޸�: ����2018/5/17
		*/
		int setUnLockFileByName(const std::string p_FileName, int p_CamId);

		/*
		*����: int getSlientPic(FilePushInfo &p_Pic)
		*����: ��ȡ��Ĭ���յĽ��
		*����: 
		*	p_Pic: �����Ϣ
		*����ֵ:
		*	0:�ɹ�
		*	1:ʧ��
		*�޸�: ����2018/5/17
		*/
		int getSlientPic(FilePushInfo &p_Pic);

		/*
		*����: int getRecordFile(FilePushInfo &p_fileInfo)
		*����: ��ȡ�ļ���Ϣ
		*����: 
		*	p_fileInfo: �ļ���Ϣ�ṹ��
		*����ֵ:
		*	0:�ɹ�
		*	1:ʧ��
		*�޸�: ����2018/5/17
		*/
		int getRecordFile(FilePushInfo &p_fileInfo);

		/*
		*����: int getFileKey(const string p_FileName, string &p_key)
		*����: ��ȡָ���ļ��ļ�����Կ
		*����:
		*	p_FileName: �ļ��� ����
		*	p_key:��Կ ���
		*����ֵ:
		*	0:�ɹ�
		*	1:ʧ��
		*�޸�: ����2018/5/17
		*/
		int getFileKey(const std::string p_FileName, std::string &p_key);

		/*
		*����: int setSlientPicName(const string p_picName)
		*����: ���þ�Ĭ�����ļ����ļ���
		*����: 
		*	p_picName: �ļ���
		*����ֵ:
		*	0:�ɹ�
		*	1:ʧ��
		*�޸�: ����2018/5/24
		*/
		int setSlientPicName(const std::string p_picName);

		/*
		*����: int setRecordFileName(const string p_recordName)
		*����: ����¼���ļ����ļ���
		*����: 
		*	p_recordName: �ļ���
		*����ֵ:
		*	0:�ɹ�
		*	1:ʧ��
		*�޸�: ����2018/5/24
		*/
		int setRecordFileName(const std::string p_recordName);

		/*
		*����: int removeSlientPic(int p_CamId)
		*����: ɾ����Ĭ�����ͼƬ
		*����: 
		*   p_CamId:  ����ͷID 0:ǰ�� 1:����
		*����ֵ:
		*	0:�ɹ�
		*	1:ʧ��
		*�޸�: ����2018/5/25
		*/
		int removeSlientPic(int p_CamId);

		/*
		*����: int removeSlientRecordFile(int p_CamId)
		*����: ɾ����Ĭ�������Ƶ
		*����: 
		*   p_CamId:  ����ͷID 0:ǰ�� 1:����
		*����ֵ:
		*	0:�ɹ�
		*	1:ʧ��
		*�޸�: ����2018/5/25
		*/
		int removeSlientRecordFile(int p_CamId);
		/*
		*����: int setFileList(std::string p_startTime, std::string p_stopTime)
		*����: ��������ļ��б��������ɷ��ʹ�����֪ͨ��Ȼ���ٵ���getFileList��ȡ������ļ���Ϣ
		*����: 
		*	p_startTime:��ʼʱ��
		*   p_stopTime:����ʱ��
		*����ֵ:
		*	0:�ɹ�
		*	-1:ʧ��
		*�޸�: ����2018/6/7
		*/
		int setFileList(std::string p_startTime, std::string p_stopTime);


		/*
		*����: int getFileList(FilePushInfo &p_fileInfoVec)
		*����: ��ȡ�ļ��б���Ϣ
		*����: 
		*   p_fileInfo:  �ļ��б���Ϣ
		*����ֵ:
		*	0:�ɹ�
		*	-1:ʧ��
		*�޸�: ����2018/6/11
		*/
		int getFileList(FilePushInfo &p_fileInfo);

		/*
		*����: int getLockFileBynameResult(LockFileInfo &p_fileLockInfo)
		*����: ��ȡ�����ļ������Ϣ
		*����: 
		*	p_fileLockInfo: �����ļ���Ϣ
		*����ֵ:
		*	0:�ɹ�
		*	-1:ʧ��
		*�޸�: ����2018/6/11
		*/
		int getLockFileBynameResult(LockFileInfo &p_fileLockInfo);


		/*
		*����: int getunLockFileBynameResult(LockFileInfo &p_fileLockInfo)
		*����: ��ȡ�������ļ������Ϣ
		*����: 
		*	p_fileLockInfo: �����ļ���Ϣ
		*����ֵ:
		*	0:�ɹ�
		*	-1:ʧ��
		*�޸�: ����2018/6/11
		*/
		int getunLockFileBynameResult(LockFileInfo &p_fileLockInfo);

		/*
		*����: int removeFile(const std::string p_FileName)
		*����: �����ݿ���ɾ��ָ���ļ�
		*����: 
		*   p_FileName:  ָ���ļ���
		*����ֵ:
		*	0:�ɹ�
		*	-1:ʧ��
		*�޸�: ����2018/6/19
		*/
		int removeFile(const std::string p_FileName);

		/*
		*����: int setLogList()
		*����: �����г����ݴ��
		*����: 
		*����ֵ:
		*	0:�ɹ�
		*	-1:ʧ��
		*�޸�: ����2018/6/27
		*/
		int setLogList(const TrafficDataMsg *log);

		/*
		*����: int getLogList(FilePushInfo &p_fileInfo)
		*����: ���log��־
		*����: 
		*����ֵ:
		*	0:�ɹ�
		*	-1:ʧ��
		*�޸�: ����2018/6/26
		*/
		int getLogList(FilePushInfo &p_fileInfo);

		/*
		*����: int getLockFileList(std::vector<LockFileInfo> &p_lockfileList, int p_CamId)
		*����: ��ȡ�����ļ��б�
		*����: 
		*	p_lockfileList:�����ļ��б�
		*	p_CamId: ��Ҫ����������ͷID�� 0:ǰ�� 1:����
		*����ֵ:
		*	0:�ɹ�
		*	-1:ʧ��
		*�޸�: ����2018/6/27
		*/
		int getLockFileList(std::vector<LockFileInfo> &p_lockfileList, int p_CamId);

		/*
		*����: int setRollingOrder(const string p_OrderId, int p_Status)
		*����: ����˳�糵ָ��
		*����:  
		*	p_OrderId: ˳�糵������ ����
		*   p_Status: ��������(0)/������ʼ(1)
		*����ֵ:
		*   0:�ɹ�
		*   1:ʧ��
		*�޸�: ����2018/6/30
		*/
		int setOrderId(const std::string p_OrderId,int p_Status);

		/*
		*����: int getRollingOrderId(string &p_OrderId)
		*����: ��ȡ˳�糵������
		*����:  
		*	p_OrderId: ˳�糵������ ���
		*����ֵ:
		*   0:�ɹ�
		*   1:ʧ��
		*�޸�: ����2018/6/27
		*/
		int getOrderId(std::string &p_OrderId);

		/*
		*����: int setQueryRollOrderId(std::string p_OrderId)
		*����: ������Ҫ��ѯ�Ķ����ţ���ѯ���ͨ���첽����
		*����: 
		*	p_OrderId:	������
		*����ֵ:
		*	0:�ɹ�
		*	-1:ʧ��
		*�޸�: ����2018/7/13
		*/
		int setQueryRollOrderId(std::string p_OrderId);
		int SetCheckThreadStatus(bool p_start);
		bool WaitCheckThreadStop();

		/*
		*����: int getDeleteFileList(std::vector<FilePushInfo> &p_fileInfoVec, int p_CamId)
		*����: ��ȡ����ɾ�����ļ��б�
		*����: 
		*	p_fileInfoVec:	ɾ�����ļ��б�
		*   p_CamId: ��Ҫ����������ͷID�� 0:ǰ�� 1:����
		*����ֵ:
		*	0:�ɹ�
		*	-1:ʧ��
		*�޸�: ����2018/8/14
		*/
		int getDeleteFileList(std::vector<LockFileInfo> &p_fileInfoVec, int p_CamId);

	private:
		/*
		*����: int getFileMd5(const string p_FileName, string &p_Md5)
		*����: ��ȡ�ļ�md5
		*����: 
		*	p_FileName: �ļ���
		*   p_Md5: �ļ�md5
		*����ֵ:
		*	0:�ɹ�
		*	1:ʧ��
		*�޸�: ����2018/5/24
		*/
		int getFileMd5(const std::string p_FileName, std::string &p_Md5);

		/*
		*����: static void *CheckThread(void *context)
		*����: ����ʱ�����߳�
		*����: 
		*����ֵ:
		*�޸�: ����2018/5/24
		*/
		static void *CheckThread(void *context);

	private:
		Mutex m_mutex;
		MediaFileManager *mMediaFileManager;
		std::string m_picName, m_recordName, m_unLockTime;
		std::vector<std::string> m_FileNameList;
		std::vector<LockFileInfo> m_LockFileVector,m_unLockFileVector;
		pthread_t m_threadId;
		bool m_ThreadExit;
		bool m_OrderIdThread_Exit;
		bool m_ThreadPause;
		bool m_ThreadPauseFinish;
		int m_FileCount;
		std::string m_LockFileName;
		std::string m_UnLockFileName;
		std::map<std::string, std::string> m_LockFileInfo;
		std::map<std::string, std::string> m_LockFileList;
		std::map<std::string, std::string> m_DeleteFileList;
};

#endif
