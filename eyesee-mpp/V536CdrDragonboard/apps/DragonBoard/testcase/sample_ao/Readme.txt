sample_ao�������̣�
	�������ò�����ȡpcm���ݣ�Ȼ�󲥷��������Ӷ��������������

��ȡ���Բ��������̣�
	sample�ṩ��sample_ao.conf�����Բ���������pcm��Ƶ�ļ�·��(dst_file)��������(pcm_sample_rate)��
	ͨ����Ŀ(pcm_channel_cnt)������λ��(pcm_bit_width)��ÿ��ȡpcm������Ŀ(pcm_frame_size)��
	����sample_aoʱ���������в����и���sample_ao.conf�ľ���·����sample_ao���ȡ���ļ�����ɲ���������
	Ȼ���ղ������в��ԡ�

������������sample_ai��ָ�
	./sample_ao -path /mnt/extsd/sample_ao/sample_ao.conf
	"-path /mnt/extsd/sample_ao/sample_ao.conf"ָ���˲��Բ��������ļ���·����

���Բ�����˵����
(1)pcm_file_path��ָ����Ƶpcm�ļ���·�������ļ��ǰ���waveͷ(��СΪ44Bytes)��wav��ʽ�ļ�������Ҳ������ָ�ʽ�ļ���������sample_ai����һ����
(2)pcm_sample_rate��ָ�������ʣ�����Ϊ�ļ��еĲ����ʵ�ֵ��
(3)pcm_channel_cnt��ָ��ͨ����Ŀ������Ϊ�ļ��е�ͨ������
(4)pcm_bit_width��ָ��λ������Ϊ�ļ��е�λ��
(5)pcm_frame_size���̶�ָ��Ϊ1024��