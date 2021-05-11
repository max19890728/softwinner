/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <linux/fb.h>
#include <sys/types.h>

#include <memory>
#include <string>

#define FBIO_CACHE_SYNC 0x4630

namespace Device {

// todo: 發生錯誤時應該擲出錯誤
class FrameBuffer {
 public:
  struct Device {
    int frame_buffer;
    uint memory_offset;
    void *memory;
    struct fb_fix_screeninfo fb_fix;
    struct fb_var_screeninfo fb_var;
    std::string path;
  };

  static FrameBuffer& instance() {
    static FrameBuffer instance_;
    return instance_;
  }

  void Clean();

 private:
  FrameBuffer() {}
  ~FrameBuffer() {}
  FrameBuffer(const FrameBuffer&) = delete;
  FrameBuffer& operator=(const FrameBuffer&) = delete;

  // open & init a frame buffer
  bool Open(std::unique_ptr<Device> const& device);

  // close frame buffer
  void Close(std::unique_ptr<Device> const& device);

  // get display depth
  int GetDepathIn(std::unique_ptr<Device> const& device);

  // full screen clear
  void Memset(void *address, int c, size_t length);

  void Sync(std::unique_ptr<Device> const& device);
};
}  // namespace Device
