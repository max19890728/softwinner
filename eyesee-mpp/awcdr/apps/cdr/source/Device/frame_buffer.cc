/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/frame_buffer.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "common/app_log.h"

void Device::FrameBuffer::Clean() {
  auto device = std::make_unique<Device>();
  device->path = "/dev/fb0";
  if (Open(device)) {
    db_error("Open Frame Buffer Failed");
    return;
  }
  Memset(device->memory + device->memory_offset, 0, device->fb_fix.smem_len);
  Sync(device);
  Close(device);
}

bool Device::FrameBuffer::Open(std::unique_ptr<Device> const& device) {
  device->frame_buffer = open(device->path.c_str(), O_RDWR);
  if (device->frame_buffer < 0) {
    db_error("Open (%s) Fail", device->path);
    return false;
  }
  if (ioctl(device->frame_buffer, FBIOGET_VSCREENINFO, &(device->fb_var)) < 0) {
    db_error("ioctl FBIOGET_VSCREENINFO");
    return false;
  }
  if (ioctl(device->frame_buffer, FBIOGET_FSCREENINFO, &(device->fb_fix)) < 0) {
    db_error("ioctl FBIOGET_FSCREENINFO\n");
    return false;
  }
  device->memory_offset = (device->fb_fix.smem_start -
                           (device->fb_fix.smem_start & ~(getpagesize() - 1)));
  device->memory =
      mmap(NULL, device->fb_fix.smem_len + device->memory_offset,
           PROT_READ | PROT_WRITE, MAP_SHARED, device->frame_buffer, 0);
  if (device->memory == MAP_FAILED) {
    db_error("mmap error");
    return false;
  }
  return true;
}

void Device::FrameBuffer::Close(std::unique_ptr<Device> const& device) {
  munmap(device->memory, device->fb_fix.smem_len + device->memory_offset);
  device->memory = NULL;
  close(device->frame_buffer);
  device->frame_buffer = -1;
}

int Device::FrameBuffer::GetDepathIn(std::unique_ptr<Device> const& device) {
  if (device->frame_buffer <= 0) {
    db_error("device not open");
    return -1;
  }
  return device->fb_var.bits_per_pixel;
}

void Device::FrameBuffer::Memset(void *address, int c, size_t length) {
  memset(address, c, length);
}

void Device::FrameBuffer::Sync(std::unique_ptr<Device> const& device) {
  void *start = device->memory + device->memory_offset;
  uint bytes_per_pixel = device->fb_var.bits_per_pixel >> 3;
  uint pitch = bytes_per_pixel * device->fb_var.xres;
  int x = 0;
  int y = 0;
  int width = device->fb_var.xres;
  int height = device->fb_var.yres;
  void *args[2];
  void *dirty_rect_vir_addr_begin = (start + pitch * y + bytes_per_pixel * x);
  void *dirty_rect_vir_addr_end = (start + pitch * (y + height));
  args[0] = dirty_rect_vir_addr_begin;
  args[1] = dirty_rect_vir_addr_end;
  ioctl(device->frame_buffer, FBIO_CACHE_SYNC, args);
  device->fb_var.yoffset = 0;
  device->fb_var.reserved[0] = x;
  device->fb_var.reserved[1] = y;
  device->fb_var.reserved[2] = width;
  device->fb_var.reserved[3] = height;
  ioctl(device->frame_buffer, FBIOPAN_DISPLAY, &device->fb_var);
}