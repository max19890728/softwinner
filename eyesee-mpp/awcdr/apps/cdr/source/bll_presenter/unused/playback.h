/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file playback.h
 * @brief 回放presenter
 * @author id:826
 * @version v0.3
 * @date 2016-08-01
 */
#pragma once

#include "common/subject.h"
#include "common/observer.h"
#include "bll_presenter/presenter.h"
#include "bll_presenter/gui_presenter_base.h"
#include "window/window_manager.h"
#include "common/app_def.h"

#include <map>


class PlaylistWindow;
class PlaybackWindow;

namespace EyeseeLinux {

/**
 * @addtogroup BLLPresenter
 * @{
 */

/**
 * @addtogroup RecPlay
 * @{
 */

class VideoPlayer;
class JpegPlayer;
class Window;
class PlaybackPresenter
	: public GUIPresenterBase
	, public IPresenter
	, public ISubjectWrap(PlaybackPresenter)
	, public IObserverWrap(PlaybackPresenter)
{
	public:

		PlaybackPresenter();

		~PlaybackPresenter();

		void OnWindowLoaded();

		void OnWindowDetached();

		int HandleGUIMessage(int msg, int val, int id = 0);

		void BindGUIWindow(::Window *win);

		int DeviceModelInit();

		int DeviceModelDeInit();

		void PrepareExit() {}

		void Update(MSG_TYPE msg, int p_CamID = 0, int p_recordId = 0);

		int RemoteSwitchRecord() { return 0; };

		int RemoteTakePhoto() { return 0; };

		int GetRemotePhotoInitState(bool &init_state) { return 0; };

		int RemoteSwitchSlowRecord() { return 0; };

		void DoDeleteFile2StopPlay();

		bool IsLastOrFirstVideo(int pre);

		void DeleteErrorFile();

		void UpdateFileTime(const std::string &filename);

		void CreatJpegHandler();

	private:

		int mTimer_Start_;

		int status_;

		pthread_mutex_t model_lock_;

		WindowManager *win_mg_;

		PlaybackWindow *playback_win_;

		PlaylistWindow *playlist_win_;

		VideoPlayer *player_;

		JpegPlayer *jpeg_player_;

		std::vector<std::string> playlist_;

		std::vector<std::string> demolist_;

		unsigned int playlist_idx_;

		StringVector::iterator bkplaylist_;

		std::string result_alias;

		int SetFileAndPrepare(const std::string &file);

		/**
		 * @brief 具体的设置按钮事件处理handler
		 * @param id 按键子id
		 * @param value 按键值
		 *  - true on
		 *  - false off
		 *
		 *  当有多个设置按钮时就需要在OnButtonPushed中传入子id
		 *
		 * @see PlaybackPresenter::OnButtonPushed
		 */
		void SettingButtonHandler(uint8_t id, bool value);

		/**
		 * @brief 播放进度控制
		 * @param pos 跳播位置
		 */
		int PlaySeekHandler(int pos);

		/**
		 * @brief 播放开始和暂停控制
		 * @param value
		 *  - true 播放
		 *  - false 暂停
		 */
		int PlayPauseHandler(bool value);

		/**
		 * @brief 停止播放
		 */
		int PlayStopHandler();

		/**
		 * @brief 调整音量等级
		 * @param level 音量等级
		 *  - 0 静音
		 *  - 1 25%
		 *  - 2 50%
		 *  - 3 75%
		 *  - 4 100%
		 */
		void VoiceControlHandler(int level);

	int GetPlaylistTotal(void);

	int GetCurrentSelectIndex(void);

	void UpdateSelectIndexAndTotalFiles(bool flag = true);

	void RefreshUnmount();

	void RefreshMounted();

	void UpdateStatusBarUI();

	void DoDeleteFileHander();

	#ifdef USEICONTHUMB
	void DoDeleteSelectFileHander();
	#endif

	void UpdatePlaybackView();

	void ShowDeleteDialog(int val);
	
	void ShowVideoPlayImg();

	int prevnextphotoflag;
}; /* class PlaybackPresenter */

/**  @} */
/**  @} */
} /* EyeseeLinux */
