/******************
  File name : fbtools.c
  */

#include "fbtools.h"
#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define FBIO_CACHE_SYNC 0x4630

// a framebuffer device structure;
typedef struct fbdev {
  int fb;
  unsigned long fb_mem_offset;
  void *fb_mem;
  struct fb_fix_screeninfo fb_fix;
  struct fb_var_screeninfo fb_var;
  char dev[20];
} FBDEV, *PFBDEV;

// open & init a frame buffer
int fb_open(PFBDEV pFbdev) {
  pFbdev->fb = open(pFbdev->dev, O_RDWR);
  if (pFbdev->fb < 0) {
    printf("Error opening %s: %m. Check kernel config\n", pFbdev->dev);
    return -1;
  }

  if (-1 == ioctl(pFbdev->fb, FBIOGET_VSCREENINFO, &(pFbdev->fb_var))) {
    printf("ioctl FBIOGET_VSCREENINFO\n");
    return -1;
  }

  if (-1 == ioctl(pFbdev->fb, FBIOGET_FSCREENINFO, &(pFbdev->fb_fix))) {
    printf("ioctl FBIOGET_FSCREENINFO\n");
    return -1;
  }

  // map physics address to virtual address
  pFbdev->fb_mem_offset = (pFbdev->fb_fix.smem_start -
                           (pFbdev->fb_fix.smem_start & ~(getpagesize() - 1)));
  pFbdev->fb_mem = mmap(NULL, pFbdev->fb_fix.smem_len + pFbdev->fb_mem_offset,
                        PROT_READ | PROT_WRITE, MAP_SHARED, pFbdev->fb, 0);

  if (MAP_FAILED == pFbdev->fb_mem) {
    printf("mmap error! mem:%p offset:%ld\n", pFbdev->fb_mem,
           pFbdev->fb_mem_offset);
    return -1;
  }
  return 0;
}

// close frame buffer
int fb_close(PFBDEV pFbdev) {
  munmap(pFbdev->fb_mem, pFbdev->fb_fix.smem_len + pFbdev->fb_mem_offset);
  pFbdev->fb_mem = NULL;
  close(pFbdev->fb);
  pFbdev->fb = -1;

  return 0;
}

// get display depth
int get_display_depth(PFBDEV pFbdev) {
  if (pFbdev->fb <= 0) {
    printf("fb device not open, open it first\n");
    return -1;
  }
  return pFbdev->fb_var.bits_per_pixel;
}

// full screen clear
void fb_memset(void *addr, int c, size_t len) { memset(addr, c, len); }

void fb_sync(PFBDEV pFbdev) {
  void *mem_start = pFbdev->fb_mem + pFbdev->fb_mem_offset;
  unsigned int bytes_per_pixel = pFbdev->fb_var.bits_per_pixel >> 3;
  unsigned int pitch = bytes_per_pixel * pFbdev->fb_var.xres;
  int x = 0;
  int y = 0;
  int w = pFbdev->fb_var.xres;
  int h = pFbdev->fb_var.yres;

  void *args[2];
  void *dirty_rect_vir_addr_begin =
      (mem_start + pitch * y + bytes_per_pixel * x);
  void *dirty_rect_vir_addr_end = (mem_start + pitch * (y + h));
  args[0] = dirty_rect_vir_addr_begin;
  args[1] = dirty_rect_vir_addr_end;
  ioctl(pFbdev->fb, FBIO_CACHE_SYNC, args);

  pFbdev->fb_var.yoffset = 0;
  pFbdev->fb_var.reserved[0] = x;
  pFbdev->fb_var.reserved[1] = y;
  pFbdev->fb_var.reserved[2] = w;
  pFbdev->fb_var.reserved[3] = h;
  ioctl(pFbdev->fb, FBIOPAN_DISPLAY, &pFbdev->fb_var);
}

int fb_clean() {
  FBDEV fbdev;
  memset(&fbdev, 0, sizeof(FBDEV));
  strncpy(fbdev.dev, "/dev/fb0", sizeof(fbdev.dev));
  if (fb_open(&fbdev) == -1) {
    printf("open frame buffer error\n");
    return -1;
  }
  fb_memset(fbdev.fb_mem + fbdev.fb_mem_offset, 0, fbdev.fb_fix.smem_len);
  fb_sync(&fbdev);
  fb_close(&fbdev);

  return 0;
}
