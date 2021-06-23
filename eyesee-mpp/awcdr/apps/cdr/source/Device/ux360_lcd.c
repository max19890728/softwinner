#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <linux/fcntl.h>
#include <linux/fb.h>
#include <linux/mman.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "Device/ux360_lcd.h"



int lcd_state = 0;	// 0:close 1:open 2:standby


int lcd_init(void) {
	//
}

int lcd_test(int idx, int r, int g, int b, int info) {
	int fd = 0;
	static struct fb_var_screeninfo vi;
    static struct fb_fix_screeninfo fi;
	char *bits = NULL;
	int x = 0, y = 0;
	unsigned long screensize=0;
	unsigned long location = 0;
	char path[128];
	//char buf[614400];
	int ret;
	
	printf("max+ lcd_test: 00 idx=%d r=%d g=%d b=%d\n\0", idx, r, g, b);
	
	// 開啟設備並讀取資訊
	sprintf(path, "/dev/fb%d\0", idx);
    fd = open(path, O_RDWR);
    if(fd < 0) {
		printf("\t lcd_test: cannot open fb0 (path=%s)\n\0", path);
        return -1;
    }
	
    if(ioctl(fd, FBIOGET_VSCREENINFO, &vi) < 0) {
		printf("\t lcd_test: failed to get fb0 info\n\0");
        close(fd);
        return -1;
    }
 
    /*if (ioctl(fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
		printf("\t lcd_test: failed to put fb0 info\n\0");
        close(fd);
        return -1;
    }*/
 
    if(ioctl(fd, FBIOGET_FSCREENINFO, &fi) < 0) {
		printf("\t lcd_test: failed to get fb0 info\n\0");
        close(fd);
        return -1;
    }
	
	screensize = vi.xres * vi.yres * vi.bits_per_pixel / 8;
	printf("\t lcd_test: x=%d y=%d bit=%d smem_len=%d size=%ld err=%d\n\0",
		vi.xres, vi.yres, vi.bits_per_pixel, fi.smem_len, screensize, errno);
	
if(info == 0) {	
    // 映射記憶體空間
	//int mapped_offset, mapped_memlen;
	//mapped_offset = (((long)fi.smem_start) -
    //                (((long)fi.smem_start)&~(getpagesize() - 1)));
    //mapped_memlen = ((fi.smem_len + mapped_offset)&~(getpagesize() - 1));
	//mapped_memlen = fi.smem_len;
	bits = mmap(NULL, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(bits == (void *)-1) {
		printf("\t lcd_test: failed to mmap framebuffer (err=%s<%d>)\n\0", strerror(errno), errno);
        close(fd);
        return -1;
    }
}	
	printf("\t lcd_test: line_length=%d\n\0", fi.line_length);
	
	// 寫入畫面
	/*for(y = 0; y < vi.yres; y++) {
		for(x = 0; x < vi.xres; x++) {
			location = x * (vi.bits_per_pixel / 8) + y * fi.line_length;
			*(bits + location)     = b; 		//B
			*(bits + location + 1) = g;			//G
			*(bits + location + 2) = r; 		//R
			*(bits + location + 3) = 0; 		//A
		}
	}*/
if(info == 0) {
	if(bits != NULL) {
		memset(bits, 0, fi.smem_len);
		usleep(1000000);
	}
	else {
		printf("\t lcd_test: bits == NULL\n\0");
	}
	
	/*memset(&buf[0], 0, sizeof(buf));
	ret = write(fd, &buf[0], fi.smem_len);
	printf("\t lcd_test: write ret=%d\n\0", ret);*/
}
	
	// 解除映射
if(info == 0) {	
	munmap(bits, fi.smem_len);
}
	close(fd);
	return 0;
}

int disp_ioctl(int rw, int value) {
	int fd = 0;
	char path[128];
	unsigned long args[4]={0}; 
	int ret;
	
	printf("\t disp_ioctl: 00 rw=%d value=%d\n\0", rw, value);
	
	sprintf(path, "/dev/disp\0");
	fd = open(path, O_RDWR);
    if(fd < 0) {
		printf("\t disp_ioctl: cannot open disp (path=%s)\n\0", path);
        return -1;
    }
	
	switch(rw) {
	case 1:		//get
		args[0] = 0; 	//lcd0
		args[1] = 0;	//BRIGHTNESS
		args[2] = 0;
		args[3] = 0;
		ret = ioctl(fd, DISP_LCD_GET_BRIGHTNESS, args);
		printf("\t disp_ioctl: get brightness=%ld\n\0", ret);
		break;
	case 2:		//set
		args[0] = 0; 
		args[1] = value;
		args[2] = 0;
		args[3] = 0;
		ret = ioctl(fd, DISP_LCD_SET_BRIGHTNESS, args);
		
		ret = ioctl(fd, DISP_LCD_GET_BRIGHTNESS, args);
		printf("\t disp_ioctl: set brightness=%ld\n\0", ret);
		break;
	case 3:		//backlight
		args[0] = 0; 
		args[1] = 0;
		args[2] = 0;
		args[3] = 0;
		if(value == 0)
			ret = ioctl(fd, DISP_LCD_BACKLIGHT_DISABLE, args);
		else
			ret = ioctl(fd, DISP_LCD_BACKLIGHT_ENABLE, args);
		
		printf("\t disp_ioctl: backlight en=%d ret=%d\n\0", value, ret);
		break;
	}
	
	close(fd);
	return 0;
}

int disp_colorbar(int value) {
	char path[128], sys_cmd[128];
	
	printf("\t disp_colorbar: 00 value=%d\n\0", value);
	
	if(value < 0 || value > 10) return -1;
	
	sprintf(path, "/sys/class/disp/disp/attr/colorbar\0");
	sprintf(sys_cmd, "echo %d > %s", value, path);
	system(sys_cmd);
	
	printf("\t disp_colorbar: 01 sys_cmd=%s\n\0", sys_cmd);
	return 0;
}
