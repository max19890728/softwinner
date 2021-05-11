/*
*��Ȩ����:����
*�ļ�����:aes_ctrl.h
*������:����
*��������:2018-5-30
*�ļ�����:���ļ���Ҫ������ļ����м���
*��ʷ��¼:��
*/
#ifndef __AES_CTRL_H__
#define __AES_CTRL_H__
	
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/subject.h"
#include <utils/Mutex.h>
#include <openssl/aes.h>

using namespace EyeseeLinux;

#define AES_BITS 256
#define BLOCK_SIZE 32
#define CHUNK_SIZE 32
#define IV_SIZE 16
#define FIXED_KEY_ENCODE (0)

class AesCtrl
{
	public:
		AesCtrl();
		~AesCtrl();

	public:
		static AesCtrl* GetInstance(void);
		
		/*
		*����: int aes_decrtpt(std::string in_file_path, std::string out_file_path)
		*����: ��������ļ����н��ܣ�����������ļ�
		*����:
		*	in_file_path: ��Ҫ���ܵ��ļ���(��������·��)
		*   out_file_path: ���ܺ���ļ�(��������·��)
		*����ֵ:
		*	1:�ɹ�
		*	0:ʧ��
		*�޸�: ����2018/5/30
		*/
		int aes_decrtpt(std::string in_file_path, std::string out_file_path);

		/*
		*����: int aes_encrypt(std::string in_file_path, std::string out_file_path)
		*����: ��������ļ����м��ܣ�������ܺ���ļ�
		*����:
		*	in_file_path: ��Ҫ���ܵ��ļ���(��������·��)
		*   out_file_path: ���ܺ���ļ�(��������·��)
		*����ֵ:
		*	1:�ɹ�
		*	0:ʧ��
		*�޸�: ����2018/5/30
		*/
		int aes_encrypt(std::string in_file_path, std::string out_file_path);

		/*
		*����: int setUserKey(std::string Key)
		*����: �����û�key,key����32λ�����+imei��ɡ�
		*����:
		*	Key: �û�key
		*����ֵ:
		*	0:�ɹ�
		*	1:ʧ��
		*�޸�: ����2018/5/30
		*/
		int setUserKey(std::string Key);

		/*
		*����: int getKey(std::string &Key)
		*����: ��ȡ�ļ�����key
		*����:
		*	Key: �û�key
		*����ֵ:
		*	1:�ɹ�
		*	0:ʧ��
		*�޸�: ����2018/5/30
		*/
		int getKey(std::string &Key);

	private:
		int generateAesKey();

	private:
		std::string m_user_key;
		unsigned char m_ivec[IV_SIZE*2+1];
		unsigned char m_ivec_use[IV_SIZE*2+1];
		unsigned char m_ivec_peer[IV_SIZE*2+1];
		char m_aes_key[CHUNK_SIZE+1];
		char m_aes_key_peer[CHUNK_SIZE+1];
		Mutex m_mutex;		
};

#endif
