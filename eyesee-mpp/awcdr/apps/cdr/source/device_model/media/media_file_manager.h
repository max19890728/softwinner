/*************************************************
Copyright (C), 2015, AllwinnerTech. Co., Ltd.
File name: media_file_manager.h
Author: yinzh@allwinnertech.com
Version: 0.1
Date: 2015-10-30
Description:
    1.通过调用数据库接口(DBCtrl)接口实现数据存储
    2.管理多媒体文件，包括，文件名的生成，文件加解锁
    3.实现拍照和录像完成的回调处理
History:
*************************************************/
#ifndef SOFTWINNER_EYESEE_MPP_AWCDR_APPS_CDR_SOURCE_DEVICE_MODEL_MEDIA_MEDIA_FILE_MANAGER_H_
#define SOFTWINNER_EYESEE_MPP_AWCDR_APPS_CDR_SOURCE_DEVICE_MODEL_MEDIA_MEDIA_FILE_MANAGER_H_

#include <map>
#include <string>
#include <vector>

#include "SQLiteCpp/SQLiteCpp.h"

#include "common/common_inc.h"
#include "common/message.h"
#include "common/observer.h"
#include "common/singleton.h"
#include "common/subject.h"

namespace SQLite {
class Database;
}

namespace EyeseeLinux {

typedef struct MixFileInfo {
    std::string file_name;
    int64_t time_value;
    std::string typeStr;
    std::string infoStr;
    int64_t sizeValue;
    int64_t LockValue;
    int64_t LockTimeValue;
    int64_t CheckStateValue;
    std::string KeyStr;
    std::string orderNumStr;
} MixFileInfo_;

class MediaFile;

class MediaFileManager : public IObserverWrap(MediaFileManager),
                         public Singleton<MediaFileManager>,
                         public ISubjectWrap(MediaFileManager) {
    friend class Singleton<MediaFileManager>;

  public:
    MediaFileManager();
    ~MediaFileManager();
    MediaFileManager(const MediaFileManager &o);
    MediaFileManager &operator=(const MediaFileManager &o);

    int8_t AddFile(const MediaFile &file, int p_CamId = 0, int64_t keyValue = 0, bool lock_flag = false);

    int8_t AddFile(const MediaFile &file, SQLite::Statement &stm, int count = -1, int p_CamId = 0, int64_t keyValue = 0,
                   std::string order_Num = "", bool lock_flag = false);
    int8_t RemoveFile(const std::string &filename);
    int8_t GetMediaFileList(std::vector<std::string> &file_list, uint32_t idx_s, uint16_t list_cnt, bool order,
                            const std::string &media_type, int p_CamId = 0);
    uint32_t GetMediaFileCnt(const std::string &media_type, int p_CamId = 0);
    int8_t DeleteFileByName(const std::string &file_name, int p_CamId = 0);
    int8_t DeleteLastFilesDatabaseIndex(const std::string &file_name, int p_CamId);
    int8_t DeleteLastFile(const std::string &media_type, int cnt, int p_CamId = 0, bool m_force = false);
    const std::string GetMediaFileType(const std::string &file_name, int p_CamId = 0);
    time_t GetFileTimestampByName(const std::string &file_name, int p_CamId = 0);
    void Update(MSG_TYPE msg, int p_CamID = 0, int p_recordId = 0);
    std::string GetThumbPicName(const std::string &file_name, int p_CamId);
    std::string GetThumbVideoName(const std::string &file_name);
    inline bool GetDataBaseStartUpInitFlag() { return db_start_up_init_; }
    inline void SetDataBaseStartUpInitFlag(bool flag) { db_start_up_init_ = flag; }
    int64_t GetLastFileFallocateSizeByType(const std::string &media_type, int p_CamId);
    uint32_t getFilefallocateSize(const std::string p_DirName);

    // 功能: 判断指定的文件在数据库中是否存在
    // 参数:
    //    - p_FileName: 需要查找的文件名称
    //    - p_CamId: 需要查找的哪个数据库
    // 返回值: true:成功, false:失败
    bool IsFileExist(const std::string &p_FileName, int p_CamId, int p_CountCapacity = 0);

    // 设置数据库中文件的锁定状态值
    // parameters:
    //    - p_FileName: 需要查找的文件名称
    //    - p_LockStatus: 锁定值
    //    - p_CamId: 需要查找的哪个数据库
    // return: 0:成功, -1:失败
    int SetFileLockStatus(const std::string &p_FileName, int p_LockStatus, int p_CamId);

    // 设置数据库中文件的锁定状态值
    // parameters:
    //    - p_FileName: 需要查找的文件名称
    //    - p_LockStatus: 锁定值
    //    - p_CamId: 需要查找的哪个数据库
    // return: 0:成功, -1:失败
    int GetFileLockStatus(const std::string &p_FileName, int *p_LockStatus, int p_CamId);

    // 获取指定时间段内的文件列表信息
    // parameters:
    //    - p_startTime: 开始时间
    //    - p_stopTime: 结束时间
    //    - p_FileNameList: 查询结果存储容器
    //    - p_CamId: 需要查找的哪个数据库
    // return: 0: 成功, -1:失败
    int GetFileList(const std::string p_startTime, const std::string p_stopTime,
                    std::vector<std::string> &p_FileNameList, int p_CamId, std::string p_OrderId);

    // 设置指定文件的锁定时间
    // parameters:
    //    - p_FileName: 文件名
    //    - p_LockTime: 锁定时间
    //    - p_CamId: 需要查找的哪个数据库
    //    - p_action: 0(设置锁定) / 1(设置解锁)
    // return: 0:成功, -1:失败
    int setFileLockTime(const std::string p_FileName, const std::string p_LockTime, int p_CamId, int p_action);

    // 获取数据库中的锁定文件信息,包括文件名与锁定时间
    // parameters:
    //    - p_Info: 文件信息
    //    - p_CamId:  摄像头ID 0:前置 1:后置
    // return: 0:成功, -1:失败
    int getFileLockInfo(std::map<std::string, std::string> &p_Info, int p_CamId);

    // 获取数据库中的锁定文件个数
    // parameters:
    //    - p_CamId: 摄像头ID 0:前置 1:后置
    // return: 文件个数
    int getLockFileCount(int p_CamId);

    // 功能: 根据 p_FileName 和 p_CamId 获取对应数据库对应的 keystring
    // parameters:
    //    - p_FileName:
    //    - keystring:
    // return: 成功：0, 失败-1
    int GetFileKey(const std::string &p_FileName, std::string &keystring);

    // 获取 p_CamId 对应数据库文件信息
    // parameters:
    //    - file_list: 文件信息
    //    - p_CamId:
    // return: 0: 成功, -1: 失败
    int GetFileList(std::map<std::string, std::string> &file_list, int p_CamId);

    // 获取 p_CamId 对应数据库文件信息
    // parameters:
    //    - file_list: 数据库文件所有信息
    //    - p_CamId:
    // return: 0: 成功, -1: 失败
    int GetAllInfoFileList(std::vector<MixFileInfo_> &file_list, int p_CamId);

    // 获取数据库是否创建和刷新完成
    // return: true:可以读取, false:未准备好
    bool DataBaseIsAlready();

    int startCreateDataBase();

    // 获取更新数据标识
    // parameters:
    //    - p_flag: true:需要更新  false: 不需要更新
    // return:
    int GetUpdateFlag(bool &p_flag);

    // 设置更新数据标识
    // parameters:
    //    - p_flag: true:需要更新  false: 不需要更新
    // return:
    int SetUpdateFlag(bool p_flag);

    int GetDeleteFileList(std::map<std::string, std::string> &file_list, int p_CamId);
    std::string getFileReallyName(std::string p_FileName);
    std::string getFilePath(std::string p_FilePath);
    int getFileType(std::string fileType);
    std::string getlockFileName();
    std::string getlocThumbPickFileName();
    int SetFileInfoByName(const std::string &fileName, const std::string &fileName_dts, const std::string typeStr,
                          int lockStatus, int p_CamId);
    int GetThumbFromMainPic(const std::string &src_pic, const std::string &dest_pic);
    void GetLastFileByType(const std::string &media_type, int p_CamId, std::string &filename);

  private:
    static void *DBInitThread(void *arg);
    static void *DoDBUpdate(void *arg);
    int8_t DBUpdate();
    int8_t DeleteFilesByType(const std::string &media_type, int p_CamId = 0);

    // 根据 p_CamId 获取对应的数据库是否创建标志位
    // parameters:
    //    - p_CamId:  摄像头ID 0:前置 1:后置
    // return: 数据库的创建标志位
    bool getNewDbCreateFlag(int P_CamID);

    // 根据 p_CamId 对应数据库设置对应的 checkstate = 0
    // parameters:
    //    - p_CamId: 摄像头ID 0:前置 1:后置
    // return: 成功：0  失败-1
    int8_t clearCheckFlag(int p_CamId);

    // 根据 p_CamId 对应数据库设置对应的 checkstate 的值
    // parameters:
    //    - p_CamId: 摄像头ID 0:前置 1:后置
    // return: 成功：0  失败-1
    int8_t setCheckFlag(int P_CamID, int checkF, const std::string &file_name);

    // 根据p_CamId 获取对应的数据库句柄
    // parameters:
    //    p_CamId: 摄像头ID 0:前置 1:后置
    // return: 数据库的句柄
    SQLite::Database *getDBHandle(int P_CamID) { return db_; }

    // 根据 p_CamId 更新对应的数据库记录内容
    //
    // parameters:
    //    - self: MediaFileManager 类指针
    //    - p_CamId: 摄像头ID 0:前置 1:后置
    void DoDBUpdateByCamId(MediaFileManager *self, int P_CamID);

    void DoDBUpdateByCamIdEx(std::string p_scanPath, MediaFileManager *self, int P_CamID, int p_IsVideo);

    // 根据 p_CamId 删除对应数据库里面 checkstatus = 0 的不存在的文件记录信息
    // parameters:
    //    - p_CamId: 摄像头ID 0:前置 1:后置
    // return: 成功：0  失败-1
    int8_t deleteFileFromDB(int p_CamId);

    // 获取数据库中的对应 Id 文件个数
    // parameters:
    //    - p_CamId: 摄像头ID 0:前置 1:后置
    // return: 檔案數量
    int getFileCount(int p_CamId);

    int8_t DBInit();
    int8_t DBReset();
    int8_t DeleteFileByOrder(const std::string &media_type, bool order, int p_CamId = 0, bool m_force = false);
    void timeString2Time_t(const std::string &timestr, time_t *time);
    int InsertLockInfoToMap(std::map<std::string, std::string> &p_Info, const std::string &file_name,
                            const std::string &LockTime);

  private:
    pthread_mutex_t m_lock, m_db_lock, m_update_lock;
    bool db_inited_;       // 记录db数据库是否创建完成
    bool db_stop_update_;  // 记录数据库文件扫描是否完成
    bool db_update_done_;
    bool db_new_create_;
    bool db_backCam_new_create_;
    bool db_start_up_init_;
    SQLite::Database *db_;
    std::string m_orderId;
    int m_OrderStatus;
    bool m_needUpdate;
    pthread_t DB_updata_thread_id, DB_init_thread_id;
    bool m_IgnoreScan;
    std::string m_lockFileName;
    std::string m_lockthumbPicFileName;
    std::vector<uint32_t> m_fallocateSize;
};
}  // namespace EyeseeLinux

#endif  // SOFTWINNER_EYESEE_MPP_AWCDR_APPS_CDR_SOURCE_DEVICE_MODEL_MEDIA_MEDIA_FILE_MANAGER_H_
