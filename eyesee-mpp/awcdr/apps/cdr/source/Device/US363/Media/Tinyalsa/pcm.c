
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <limits.h>

#include <linux/ioctl.h>
#define __force
#define __bitwise
#define __user
#include "Device/US363/Media/Tinyalsa/asound.h"
#include "Device/US363/Media/Tinyalsa/asoundlib.h"
//tmp #include "us360.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::Pcm"

#define PARAM_MAX SNDRV_PCM_HW_PARAM_LAST_INTERVAL
#define SNDRV_PCM_HW_PARAMS_NO_PERIOD_WAKEUP (1<<2)

static inline int param_is_mask(int p)
{
    return (p >= SNDRV_PCM_HW_PARAM_FIRST_MASK) &&
        (p <= SNDRV_PCM_HW_PARAM_LAST_MASK);
}

static inline int param_is_interval(int p)
{
    return (p >= SNDRV_PCM_HW_PARAM_FIRST_INTERVAL) &&
        (p <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL);
}

static inline struct snd_interval *param_to_interval(struct snd_pcm_hw_params *p, int n)
{
    return &(p->intervals[n - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL]);
}

static inline struct snd_mask *param_to_mask(struct snd_pcm_hw_params *p, int n)
{
    return &(p->masks[n - SNDRV_PCM_HW_PARAM_FIRST_MASK]);
}

static void param_set_mask(struct snd_pcm_hw_params *p, int n, unsigned int bit)
{
    if (bit >= SNDRV_MASK_MAX)
        return;
    if (param_is_mask(n)) {
        struct snd_mask *m = param_to_mask(p, n);
        m->bits[0] = 0;
        m->bits[1] = 0;
        m->bits[bit >> 5] |= (1 << (bit & 31));
    }
}

static void param_set_min(struct snd_pcm_hw_params *p, int n, unsigned int val)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        i->min = val;
    }
}

static void param_set_max(struct snd_pcm_hw_params *p, int n, unsigned int val)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        i->max = val;
    }
}

static void param_set_int(struct snd_pcm_hw_params *p, int n, unsigned int val)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        i->min = val;
        i->max = val;
        i->integer = 1;
    }
}

static unsigned int param_get_int(struct snd_pcm_hw_params *p, int n)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        if (i->integer)
            return i->max;
    }
    return 0;
}

#define DEBUG 0
#if DEBUG
static const char *param_name[PARAM_MAX+1] = {
    [SNDRV_PCM_HW_PARAM_ACCESS] = "access",
    [SNDRV_PCM_HW_PARAM_FORMAT] = "format",
    [SNDRV_PCM_HW_PARAM_SUBFORMAT] = "subformat",

    [SNDRV_PCM_HW_PARAM_SAMPLE_BITS] = "sample_bits",
    [SNDRV_PCM_HW_PARAM_FRAME_BITS] = "frame_bits",
    [SNDRV_PCM_HW_PARAM_CHANNELS] = "channels",
    [SNDRV_PCM_HW_PARAM_RATE] = "rate",
    [SNDRV_PCM_HW_PARAM_PERIOD_TIME] = "period_time",
    [SNDRV_PCM_HW_PARAM_PERIOD_SIZE] = "period_size",
    [SNDRV_PCM_HW_PARAM_PERIOD_BYTES] = "period_bytes",
    [SNDRV_PCM_HW_PARAM_PERIODS] = "periods",
    [SNDRV_PCM_HW_PARAM_BUFFER_TIME] = "buffer_time",
    [SNDRV_PCM_HW_PARAM_BUFFER_SIZE] = "buffer_size",
    [SNDRV_PCM_HW_PARAM_BUFFER_BYTES] = "buffer_bytes",
    [SNDRV_PCM_HW_PARAM_TICK_TIME] = "tick_time",
};

static void param_dump(struct snd_pcm_hw_params *p)
{
    int n;

    for (n = SNDRV_PCM_HW_PARAM_FIRST_MASK;
         n <= SNDRV_PCM_HW_PARAM_LAST_MASK; n++) {
            struct snd_mask *m = param_to_mask(p, n);
            db_debug("%s = %08x%08x\n", param_name[n],
                   m->bits[1], m->bits[0]);
    }
    for (n = SNDRV_PCM_HW_PARAM_FIRST_INTERVAL;
         n <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL; n++) {
            struct snd_interval *i = param_to_interval(p, n);
            db_debug("%s = (%d,%d) omin=%d omax=%d int=%d empty=%d\n",
                   param_name[n], i->min, i->max, i->openmin,
                   i->openmax, i->integer, i->empty);
    }
    db_debug("info = %08x\n", p->info);
    db_debug("msbits = %d\n", p->msbits);
    db_debug("rate = %d/%d\n", p->rate_num, p->rate_den);
    db_debug("fifo = %d\n", (int) p->fifo_size);
}

static void info_dump(struct snd_pcm_info *info)
{
	db_debug("device = %d\n", info->device);
	db_debug("subdevice = %d\n", info->subdevice);
	db_debug("stream = %d\n", info->stream);
	db_debug("card = %d\n", info->card);
	db_debug("id = '%s'\n", info->id);
	db_debug("name = '%s'\n", info->name);
	db_debug("subname = '%s'\n", info->subname);
	db_debug("dev_class = %d\n", info->dev_class);
	db_debug("dev_subclass = %d\n", info->dev_subclass);
	db_debug("subdevices_count = %d\n", info->subdevices_count);
	db_debug("subdevices_avail = %d\n", info->subdevices_avail);
}
#else
static void param_dump(struct snd_pcm_hw_params *p) {}
static void info_dump(struct snd_pcm_info *info) {}
#endif

static void param_init(struct snd_pcm_hw_params *p)
{
    int n;

    memset(p, 0, sizeof(*p));
    for (n = SNDRV_PCM_HW_PARAM_FIRST_MASK;
         n <= SNDRV_PCM_HW_PARAM_LAST_MASK; n++) {
            struct snd_mask *m = param_to_mask(p, n);
            m->bits[0] = ~0;
            m->bits[1] = ~0;
    }
    for (n = SNDRV_PCM_HW_PARAM_FIRST_INTERVAL;
         n <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL; n++) {
            struct snd_interval *i = param_to_interval(p, n);
            i->min = 0;
            i->max = ~0;
    }
}

#define PCM_ERROR_MAX 128

struct pcm {
    int fd;
    unsigned int flags;
    int running:1;
    int underruns;
    unsigned int buffer_size;
    unsigned int boundary;
    char error[PCM_ERROR_MAX];
    struct pcm_config config;
    struct snd_pcm_mmap_status *mmap_status;
    struct snd_pcm_mmap_control *mmap_control;
    struct snd_pcm_sync_ptr *sync_ptr;
    void *mmap_buffer;
    unsigned int noirq_frames_per_msec;
    int wait_for_avail_min;

	// star add
	int capture_channels;
	short * p_capture_buf;
};

int pcm_get_config(struct pcm *pcm, struct pcm_config *config)
{
	*config = pcm->config;
    return 0;
}

unsigned int pcm_get_buffer_size(struct pcm *pcm)
{
    return pcm->buffer_size;
}

const char* pcm_get_error(struct pcm *pcm)
{
    return pcm->error;
}

static int oops(struct pcm *pcm, int e, const char *fmt, ...)
{
    va_list ap;
    int sz;

    va_start(ap, fmt);
    vsnprintf(pcm->error, PCM_ERROR_MAX, fmt, ap);
    va_end(ap);
    sz = strlen(pcm->error);

    if (errno)
        snprintf(pcm->error + sz, PCM_ERROR_MAX - sz,
                 ": %s", strerror(e));
    return -1;
}

static unsigned int pcm_format_to_alsa(enum pcm_format format)
{
    switch (format) {
    case PCM_FORMAT_S32_LE:
        return SNDRV_PCM_FORMAT_S32_LE;
    default:
    case PCM_FORMAT_S16_LE:
        return SNDRV_PCM_FORMAT_S16_LE;
    };
}

static unsigned int pcm_format_to_bits(enum pcm_format format)
{
    switch (format) {
    case PCM_FORMAT_S32_LE:
        return 32;
    default:
    case PCM_FORMAT_S16_LE:
        return 16;
    };
}

unsigned int pcm_bytes_to_frames(struct pcm *pcm, unsigned int bytes)
{
    return bytes / (pcm->config.channels *
        (pcm_format_to_bits(pcm->config.format) >> 3));
}

unsigned int pcm_frames_to_bytes(struct pcm *pcm, unsigned int frames)
{
    return frames * pcm->config.channels *
        (pcm_format_to_bits(pcm->config.format) >> 3);
}

static int pcm_sync_ptr(struct pcm *pcm, int flags) {
    if (pcm->sync_ptr) {
        pcm->sync_ptr->flags = flags;
        if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_SYNC_PTR, pcm->sync_ptr) < 0)
            return -1;
    }
    return 0;
}

static int pcm_hw_mmap_status(struct pcm *pcm) {

    if (pcm->sync_ptr)
        return 0;

    int page_size = sysconf(_SC_PAGE_SIZE);
    pcm->mmap_status = mmap(NULL, page_size, PROT_READ, MAP_FILE | MAP_SHARED,
                            pcm->fd, SNDRV_PCM_MMAP_OFFSET_STATUS);
    if (pcm->mmap_status == MAP_FAILED)
        pcm->mmap_status = NULL;
    if (!pcm->mmap_status)
        goto mmap_error;

    pcm->mmap_control = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
                             MAP_FILE | MAP_SHARED, pcm->fd, SNDRV_PCM_MMAP_OFFSET_CONTROL);
    if (pcm->mmap_control == MAP_FAILED)
        pcm->mmap_control = NULL;
    if (!pcm->mmap_control) {
        munmap(pcm->mmap_status, page_size);
        pcm->mmap_status = NULL;
        goto mmap_error;
    }
    if (pcm->flags & PCM_MMAP)
        pcm->mmap_control->avail_min = pcm->config.avail_min;
    else
        pcm->mmap_control->avail_min = 1;

    return 0;

mmap_error:

    pcm->sync_ptr = calloc(1, sizeof(*pcm->sync_ptr));
    if (!pcm->sync_ptr)
        return -ENOMEM;
    pcm->mmap_status = &pcm->sync_ptr->s.status;
    pcm->mmap_control = &pcm->sync_ptr->c.control;
    if (pcm->flags & PCM_MMAP)
        pcm->mmap_control->avail_min = pcm->config.avail_min;
    else
        pcm->mmap_control->avail_min = 1;

    pcm_sync_ptr(pcm, 0);

    return 0;
}

static void pcm_hw_munmap_status(struct pcm *pcm) {
    if (pcm->sync_ptr) {
        free(pcm->sync_ptr);
        pcm->sync_ptr = NULL;
    } else {
        int page_size = sysconf(_SC_PAGE_SIZE);
        if (pcm->mmap_status)
            munmap(pcm->mmap_status, page_size);
        if (pcm->mmap_control)
            munmap(pcm->mmap_control, page_size);
    }
    pcm->mmap_status = NULL;
    pcm->mmap_control = NULL;
}

static int pcm_areas_copy(struct pcm *pcm, unsigned int pcm_offset,
                          const char *src, unsigned int src_offset,
                          unsigned int frames)
{
    int size_bytes = pcm_frames_to_bytes(pcm, frames);
    int pcm_offset_bytes = pcm_frames_to_bytes(pcm, pcm_offset);
    int src_offset_bytes = pcm_frames_to_bytes(pcm, src_offset);

    /* interleaved only atm */
    memcpy((char*)pcm->mmap_buffer + pcm_offset_bytes,
           src + src_offset_bytes, size_bytes);
    return 0;
}

static int pcm_mmap_write_areas(struct pcm *pcm, const char *src,
                                unsigned int offset, unsigned int size)
{
    void *pcm_areas;
    int commit;
    unsigned int pcm_offset, frames, count = 0;

    while (size > 0) {
        frames = size;
        pcm_mmap_begin(pcm, &pcm_areas, &pcm_offset, &frames);
        pcm_areas_copy(pcm, pcm_offset, src, offset, frames);
        commit = pcm_mmap_commit(pcm, pcm_offset, frames);
        if (commit < 0) {
            oops(pcm, commit, "failed to commit %d frames\n", frames);
            return commit;
        }

        offset += commit;
        count += commit;
        size -= commit;
    }
    return count;
}

int pcm_get_htimestamp(struct pcm *pcm, unsigned int *avail,
                       struct timespec *tstamp)
{
    int frames;
    int rc;
    snd_pcm_uframes_t hw_ptr;

    if (!pcm_is_ready(pcm))
        return -1;

    rc = pcm_sync_ptr(pcm, SNDRV_PCM_SYNC_PTR_APPL|SNDRV_PCM_SYNC_PTR_HWSYNC);
    if (rc < 0)
        return -1;

    if ((pcm->mmap_status->state != PCM_STATE_RUNNING) &&
            (pcm->mmap_status->state != PCM_STATE_DRAINING))
        return -1;

    *tstamp = pcm->mmap_status->tstamp;
    if (tstamp->tv_sec == 0 && tstamp->tv_nsec == 0)
        return -1;

    hw_ptr = pcm->mmap_status->hw_ptr;
    if (pcm->flags & PCM_IN)
        frames = hw_ptr - pcm->mmap_control->appl_ptr;
    else
        frames = hw_ptr + pcm->buffer_size - pcm->mmap_control->appl_ptr;

    if (frames < 0)
        frames += pcm->boundary;
    else if (frames >= (int)pcm->boundary)
        frames -= pcm->boundary;

    *avail = (unsigned int)frames;

    return 0;
}

int pcm_write(struct pcm *pcm, const void *data, unsigned int count)
{
    struct snd_xferi x;

    if (pcm->flags & PCM_IN)
        return -EINVAL;

    x.buf = (void*)data;
    x.frames = count / (pcm->config.channels *
                        pcm_format_to_bits(pcm->config.format) / 8);

    for (;;) {
        if (!pcm->running) {
            if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_PREPARE))
                return oops(pcm, errno, "cannot prepare channel");
            if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_WRITEI_FRAMES, &x))
                return oops(pcm, errno, "cannot write initial data");
            pcm->running = 1;
            return 0;
        }
        if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_WRITEI_FRAMES, &x)) {
            pcm->running = 0;
            if (errno == EPIPE) {
                /* we failed to make our window -- try to restart if we are
                 * allowed to do so.  Otherwise, simply allow the EPIPE error to
                 * propagate up to the app level */
                pcm->underruns++;
                if (pcm->flags & PCM_NORESTART)
                    return -EPIPE;
                continue;
            }
            return oops(pcm, errno, "cannot write stream data");
        }
        return 0;
    }
}

int pcm_read(struct pcm *pcm, void *data, unsigned int count)
{
    struct snd_xferi x;

    if (!(pcm->flags & PCM_IN))
        return -EINVAL;

    //x.buf = (void *)pcm->p_capture_buf;
    x.buf = data;
    //x.frames = count / (pcm->capture_channels *
    //                    pcm_format_to_bits(pcm->config.format) / 8);
    x.frames = count / (pcm->config.channels *
                        pcm_format_to_bits(pcm->config.format) / 8);

    for (;;) {
        if (!pcm->running) {
           if (pcm_start(pcm) < 0) {
        	    db_error("start error\n");
                return -errno;
            }
        }
        if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_READI_FRAMES, &x)) {
        	db_debug("pcm_read() SNDRV_PCM_IOCTL_READI_FRAMES err\n");
            pcm->running = 0;
            if (errno == EPIPE) {
                    /* we failed to make our window -- try to restart */
            	db_error("pcm_read() EPIPE\n");
                pcm->underruns++;
                continue;
            }
            return oops(pcm, errno, "cannot read stream data");
        }
        break;
    }
/*
	if (pcm->capture_channels == 1)
	{
		short * p_out_data = data;
		int offset = 0, cnt = 0;
		for (cnt = 0; cnt < x.frames; cnt++)
		{
			offset = cnt << 1;		// short
			*(p_out_data + cnt) =  (*(pcm->p_capture_buf + offset) >> 1) + (*(pcm->p_capture_buf + offset + 1) >> 1);
		}
	}
	else
	{
		int cnt = 0;
		short * p_out_data = data;
		for (cnt = 0; cnt < (count >> 1); cnt++)
		{
			//db_debug("pcm_read() 2-2-1\n");
			//*(p_out_data + cnt) = *(pcm->p_capture_buf + cnt);
			if(pcm == NULL) db_error("pcm err\n");
			else if(pcm->p_capture_buf == NULL) db_error("pcm->p_capture_buf err\n");
			db_debug("pcm_read() 2-2-2\n");
		}
	}
*/
	return 0;
}

int pcm_read_ex(struct pcm *pcm, void *data, unsigned int count)
{
    struct snd_xferi x;

    if (!(pcm->flags & PCM_IN))
        return -EINVAL;

    x.buf = data;
    x.frames = count / (pcm->config.channels *
                        pcm_format_to_bits(pcm->config.format) / 8);

    for (;;) {
        if (!pcm->running) {
            if (pcm_start(pcm) < 0) {
                db_error("start error\n");
                return -errno;
            }
        }
        if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_READI_FRAMES, &x)) {
            pcm->running = 0;
            if (errno == EPIPE) {
                    /* we failed to make our window -- try to restart */
                pcm->underruns++;
                continue;
            }
            return oops(pcm, errno, "cannot read stream data");
        }
        return 0;
    }
}

static struct pcm bad_pcm = {
    .fd = -1,
};

int pcm_close(struct pcm *pcm)
{
    if (pcm == &bad_pcm)
        return 0;

    pcm_hw_munmap_status(pcm);

    if (pcm->flags & PCM_MMAP) {
        pcm_stop(pcm);
        munmap(pcm->mmap_buffer, pcm_frames_to_bytes(pcm, pcm->buffer_size));
    }

    if (pcm->fd >= 0)
        close(pcm->fd);
    pcm->running = 0;
    pcm->buffer_size = 0;
    pcm->fd = -1;

	if (pcm->p_capture_buf != 0)
	{
		free(pcm->p_capture_buf);
		pcm->p_capture_buf = 0;
	}
    free(pcm);
    return 0;
}

static int pcm_rate[] = {8000,
						11025,
						12000,
						16000,
						22050,
						24000,
						32000,
						44100,
						48000};

static int size_rate = sizeof(pcm_rate) / sizeof(pcm_rate[0]);

struct pcm *pcm_open_req(unsigned int card, unsigned int device,
                     unsigned int flags, struct pcm_config *config, int requested_rate)
{
    struct pcm *pcm;
    struct snd_pcm_info info;
    struct snd_pcm_hw_params params;
    struct snd_pcm_sw_params sparams;
    char fn[256];
    int rc;
	int index = 0, cnt = 0;
	int ret = -1;

	db_debug("pcm_open_req, %s card: %d, device: %d, req_rate: %d\n",
		(flags & PCM_IN ? "capture" : "playback"), card, device, requested_rate);

    pcm = calloc(1, sizeof(struct pcm));
    if (!pcm || !config)
        return &bad_pcm; /* TODO: could support default config here */

    pcm->config = *config;

    snprintf(fn, sizeof(fn), "/dev/snd/pcmC%uD%u%c", card, device,
             flags & PCM_IN ? 'c' : 'p');

	pcm->capture_channels = config->in_init_channels ;
	if ((flags & PCM_IN) && (config->in_init_channels == 1))
	{
		config->channels = 2;		// set hw params stereo(2 channels)
		db_debug("force capture stereo audio\n");
	}

	if ((flags & PCM_IN))
	{
		pcm->p_capture_buf = (short*)calloc(1, 1024 * 8);
		if (pcm->p_capture_buf == 0)
		{
			db_error("calloc capture buffer failed\n");
			goto fail_close;
		}
	}

    pcm->flags = flags;
    pcm->fd = open(fn, O_RDWR);
    if (pcm->fd < 0) {
        oops(pcm, errno, "cannot open device '%s'", fn);
        return pcm;
    }

    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_INFO, &info)) {
        oops(pcm, errno, "cannot get info");
        goto fail_close;
    }
    info_dump(&info);

	for (index = 0; index < size_rate; index++)
	{
		if (pcm_rate[index] == requested_rate)
		{
			break;
		}
	}

	if (index == size_rate)
	{
		if (requested_rate < pcm_rate[0])
		{
			config->rate = pcm_rate[0];
		}
		else
		{
			config->rate = pcm_rate[index - 1];
		}
	}

    for (cnt = 0; cnt < size_rate; cnt++)
	{
		config->rate = pcm_rate[(index + cnt) % size_rate];
		db_debug("pcm_open_req try rate: %d\n", config->rate);

		param_init(&params);
	    param_set_mask(&params, SNDRV_PCM_HW_PARAM_FORMAT,
	                   pcm_format_to_alsa(config->format));
	    param_set_mask(&params, SNDRV_PCM_HW_PARAM_SUBFORMAT,
	                   SNDRV_PCM_SUBFORMAT_STD);
	    param_set_min(&params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, config->period_size);
	    param_set_int(&params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS,
	                  pcm_format_to_bits(config->format));
	    param_set_int(&params, SNDRV_PCM_HW_PARAM_FRAME_BITS,
	                  pcm_format_to_bits(config->format) * config->channels);
	    param_set_int(&params, SNDRV_PCM_HW_PARAM_CHANNELS,
	                  config->channels);
	    param_set_int(&params, SNDRV_PCM_HW_PARAM_PERIODS, config->period_count);
	    param_set_int(&params, SNDRV_PCM_HW_PARAM_RATE, config->rate);

		param_dump(&params);

	    if (flags & PCM_NOIRQ) {

	        if (!(flags & PCM_MMAP)) {
	            oops(pcm, -EINVAL, "noirq only currently supported with mmap().");
	            goto fail;
	        }

	        params.flags |= SNDRV_PCM_HW_PARAMS_NO_PERIOD_WAKEUP;
	        pcm->noirq_frames_per_msec = config->rate / 1000;
	    }

	    if (flags & PCM_MMAP)
	        param_set_mask(&params, SNDRV_PCM_HW_PARAM_ACCESS,
	                   SNDRV_PCM_ACCESS_MMAP_INTERLEAVED);
	    else
	        param_set_mask(&params, SNDRV_PCM_HW_PARAM_ACCESS,
	                   SNDRV_PCM_ACCESS_RW_INTERLEAVED);

	    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_HW_PARAMS, &params)) {
	        db_debug("cannot set hw params\n");
	    }
		else
		{
	    	db_debug("pcm_open_req OK config->rate: %d\n", config->rate);
			break;
		}
    }

	if (cnt == size_rate)
	{
		oops(pcm, errno, "pcm_open_req cannot set hw params");
        goto fail_close;
	}

	param_dump(&params);

    /* get our refined hw_params */
    config->period_size = param_get_int(&params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE);
    config->period_count = param_get_int(&params, SNDRV_PCM_HW_PARAM_PERIODS);
    pcm->buffer_size = config->period_count * config->period_size;

    if (flags & PCM_MMAP) {
        pcm->mmap_buffer = mmap(NULL, pcm_frames_to_bytes(pcm, pcm->buffer_size),
                                PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, pcm->fd, 0);
        if (pcm->mmap_buffer == MAP_FAILED) {
            oops(pcm, -errno, "failed to mmap buffer %d bytes\n",
                 pcm_frames_to_bytes(pcm, pcm->buffer_size));
            goto fail_close;
        }
    }

    memset(&sparams, 0, sizeof(sparams));
    sparams.tstamp_mode = SNDRV_PCM_TSTAMP_ENABLE;
    sparams.period_step = 1;

      if (!config->start_threshold) {
        if (pcm->flags & PCM_IN)
            pcm->config.start_threshold = sparams.start_threshold = 1;
        else
            pcm->config.start_threshold = sparams.start_threshold =
                config->period_count * config->period_size / 2;
    } else
        sparams.start_threshold = config->start_threshold;

    /* pick a high stop threshold - todo: does this need further tuning */
    if (!config->stop_threshold) {
        if (pcm->flags & PCM_IN)
            pcm->config.stop_threshold = sparams.stop_threshold =
                config->period_count * config->period_size * 10;
        else
            pcm->config.stop_threshold = sparams.stop_threshold =
                config->period_count * config->period_size;
    }
    else
        sparams.stop_threshold = config->stop_threshold;

    if (!pcm->config.avail_min) {
        if (pcm->flags & PCM_MMAP)
            pcm->config.avail_min = sparams.avail_min = pcm->config.period_size;
        else
            pcm->config.avail_min = sparams.avail_min = 1;
    } else
        sparams.avail_min = config->avail_min;

    sparams.xfer_align = config->period_size / 2; /* needed for old kernels */
    sparams.silence_size = 0;
    sparams.silence_threshold = config->silence_threshold;
    pcm->boundary = sparams.boundary = pcm->buffer_size;

    while (pcm->boundary * 2 <= LONG_MAX - pcm->buffer_size)
		pcm->boundary *= 2;

    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_SW_PARAMS, &sparams)) {
        oops(pcm, errno, "cannot set sw params");
        goto fail;
    }

    rc = pcm_hw_mmap_status(pcm);
    if (rc < 0) {
        oops(pcm, rc, "mmap status failed");
        goto fail;
    }

    pcm->underruns = 0;
    return pcm;

fail:
    if (flags & PCM_MMAP)
        munmap(pcm->mmap_buffer, pcm_frames_to_bytes(pcm, pcm->buffer_size));
fail_close:
    close(pcm->fd);
    pcm->fd = -1;
    return pcm;
}

struct pcm *pcm_open(unsigned int card, unsigned int device,
                     unsigned int flags, struct pcm_config *config, int fps)
{
    struct pcm *pcm;
    struct snd_pcm_info info;
    struct snd_pcm_hw_params params;
    struct snd_pcm_sw_params sparams;
    char fn[256];
    int rc;
    int cnt;

    pcm = calloc(1, sizeof(struct pcm));
    if (!pcm || !config)
        return &bad_pcm; /* TODO: could support default config here */

    pcm->config = *config;

    snprintf(fn, sizeof(fn), "/dev/snd/pcmC%uD%u%c", card, device,
             flags & PCM_IN ? 'c' : 'p');

    pcm->flags = flags;
    pcm->fd = open(fn, O_RDWR);
    if (pcm->fd < 0) {
        oops(pcm, errno, "cannot open device '%s'", fn);
        return pcm;
    }

    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_INFO, &info)) {
        oops(pcm, errno, "cannot get info");
        goto fail_close;
    }
    info_dump(&info);

    for(cnt = size_rate-1; cnt >= 0; cnt--){
    	config->rate = pcm_rate[cnt];
    	config->period_size = config->rate * config->channels * pcm_format_to_bits(config->format) / 8 / fps / config->period_count;
    	db_debug("pcm_open() try rate: %d, period_size: %d\n", config->rate, config->period_size);

		param_init(&params);
		param_set_mask(&params, SNDRV_PCM_HW_PARAM_FORMAT,
					   pcm_format_to_alsa(config->format));
		param_set_mask(&params, SNDRV_PCM_HW_PARAM_SUBFORMAT,
					   SNDRV_PCM_SUBFORMAT_STD);
		param_set_min(&params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, config->period_size);
		param_set_int(&params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS,
					  pcm_format_to_bits(config->format));
		param_set_int(&params, SNDRV_PCM_HW_PARAM_FRAME_BITS,
					  pcm_format_to_bits(config->format) * config->channels);
		param_set_int(&params, SNDRV_PCM_HW_PARAM_CHANNELS,
					  config->channels);
		param_set_int(&params, SNDRV_PCM_HW_PARAM_PERIODS, config->period_count);
		param_set_int(&params, SNDRV_PCM_HW_PARAM_RATE, config->rate);

		if (flags & PCM_NOIRQ) {

			if (!(flags & PCM_MMAP)) {
				oops(pcm, -EINVAL, "noirq only currently supported with mmap().");
				goto fail;
			}

			params.flags |= SNDRV_PCM_HW_PARAMS_NO_PERIOD_WAKEUP;
			pcm->noirq_frames_per_msec = config->rate / 1000;
		}

		if (flags & PCM_MMAP)
			param_set_mask(&params, SNDRV_PCM_HW_PARAM_ACCESS,
					   SNDRV_PCM_ACCESS_MMAP_INTERLEAVED);
		else
			param_set_mask(&params, SNDRV_PCM_HW_PARAM_ACCESS,
					   SNDRV_PCM_ACCESS_RW_INTERLEAVED);

		if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_HW_PARAMS, &params)) {
			oops(pcm, errno, "cannot set hw params");
			//goto fail_close;
		}
		else
			break;
    }

    if(cnt < 0)
    	goto fail_close;

	param_dump(&params);

    /* get our refined hw_params */
    config->period_size = param_get_int(&params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE);
    config->period_count = param_get_int(&params, SNDRV_PCM_HW_PARAM_PERIODS);
    pcm->buffer_size = config->period_count * config->period_size;

    if (flags & PCM_MMAP) {
        pcm->mmap_buffer = mmap(NULL, pcm_frames_to_bytes(pcm, pcm->buffer_size),
                                PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, pcm->fd, 0);
        if (pcm->mmap_buffer == MAP_FAILED) {
            oops(pcm, -errno, "failed to mmap buffer %d bytes\n",
                 pcm_frames_to_bytes(pcm, pcm->buffer_size));
            goto fail_close;
        }
    }


    memset(&sparams, 0, sizeof(sparams));
    sparams.tstamp_mode = SNDRV_PCM_TSTAMP_ENABLE;
    sparams.period_step = 1;

    if (!config->start_threshold) {
        if (pcm->flags & PCM_IN)
            pcm->config.start_threshold = sparams.start_threshold = 1;
        else
            pcm->config.start_threshold = sparams.start_threshold =
                config->period_count * config->period_size / 2;
    } else
        sparams.start_threshold = config->start_threshold;

    /* pick a high stop threshold - todo: does this need further tuning */
    if (!config->stop_threshold) {
        if (pcm->flags & PCM_IN)
            pcm->config.stop_threshold = sparams.stop_threshold =
                config->period_count * config->period_size * 10;
        else
            pcm->config.stop_threshold = sparams.stop_threshold =
                config->period_count * config->period_size;
    }
    else
        sparams.stop_threshold = config->stop_threshold;

    if (!pcm->config.avail_min) {
        if (pcm->flags & PCM_MMAP)
            pcm->config.avail_min = sparams.avail_min = pcm->config.period_size;
        else
            pcm->config.avail_min = sparams.avail_min = 1;
    } else
        sparams.avail_min = config->avail_min;

    sparams.xfer_align = config->period_size / 2; /* needed for old kernels */
    sparams.silence_size = 0;
    sparams.silence_threshold = config->silence_threshold;
    pcm->boundary = sparams.boundary = pcm->buffer_size;

    while (pcm->boundary * 2 <= INT_MAX - pcm->buffer_size)
		pcm->boundary *= 2;

    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_SW_PARAMS, &sparams)) {
        oops(pcm, errno, "cannot set sw params");
        goto fail;
    }

    rc = pcm_hw_mmap_status(pcm);
    if (rc < 0) {
        oops(pcm, rc, "mmap status failed");
        goto fail;
    }

    pcm->underruns = 0;
    db_debug("pcm_open() ok\n");
    return pcm;

fail:
	db_error("pcm_open() fail\n");
    if (flags & PCM_MMAP)
        munmap(pcm->mmap_buffer, pcm_frames_to_bytes(pcm, pcm->buffer_size));
fail_close:
	db_error("pcm_open() fail_close\n");
    close(pcm->fd);
    pcm->fd = -1;
    return pcm;
}


int pcm_is_ready(struct pcm *pcm)
{
    return pcm->fd >= 0;
}

int pcm_start(struct pcm *pcm)
{
    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_PREPARE) < 0)
        return oops(pcm, errno, "cannot prepare channel");

    if (pcm->flags & PCM_MMAP)
	    pcm_sync_ptr(pcm, 0);

    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_START) < 0)
        return oops(pcm, errno, "cannot start channel");

    pcm->running = 1;
    return 0;
}

int pcm_stop(struct pcm *pcm)
{
    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_DROP) < 0)
        return oops(pcm, errno, "cannot stop channel");

    pcm->running = 0;
    return 0;
}

static inline int pcm_mmap_playback_avail(struct pcm *pcm)
{
    int avail;

    avail = pcm->mmap_status->hw_ptr + pcm->buffer_size - pcm->mmap_control->appl_ptr;

    if (avail < 0)
        avail += pcm->boundary;
    else if (avail >= (int)pcm->boundary)
        avail -= pcm->boundary;

    return avail;
}

static inline int pcm_mmap_capture_avail(struct pcm *pcm)
{
    int avail = pcm->mmap_status->hw_ptr - pcm->mmap_control->appl_ptr;
    if (avail < 0)
        avail += pcm->boundary;
    return avail;
}

static inline int pcm_mmap_avail(struct pcm *pcm)
{
    pcm_sync_ptr(pcm, SNDRV_PCM_SYNC_PTR_HWSYNC);
    if (pcm->flags & PCM_IN)
        return pcm_mmap_capture_avail(pcm);
    else
        return pcm_mmap_playback_avail(pcm);
}

static void pcm_mmap_appl_forward(struct pcm *pcm, int frames)
{
    unsigned int appl_ptr = pcm->mmap_control->appl_ptr;
    appl_ptr += frames;

    /* check for boundary wrap */
    if (appl_ptr >= pcm->boundary)
         appl_ptr -= pcm->boundary;
    pcm->mmap_control->appl_ptr = appl_ptr;
}

int pcm_mmap_begin(struct pcm *pcm, void **areas, unsigned int *offset,
                   unsigned int *frames)
{
    unsigned int continuous, copy_frames, avail;

    /* return the mmap buffer */
    *areas = pcm->mmap_buffer;

    /* and the application offset in frames */
    *offset = pcm->mmap_control->appl_ptr % pcm->buffer_size;

    avail = pcm_mmap_avail(pcm);
    if (avail > pcm->buffer_size)
        avail = pcm->buffer_size;
    continuous = pcm->buffer_size - *offset;

    /* we can only copy frames if the are availabale and continuos */
    copy_frames = *frames;
    if (copy_frames > avail)
        copy_frames = avail;
    if (copy_frames > continuous)
        copy_frames = continuous;
    *frames = copy_frames;

    return 0;
}

int pcm_mmap_commit(struct pcm *pcm, unsigned int offset, unsigned int frames)
{
    /* update the application pointer in userspace and kernel */
    pcm_mmap_appl_forward(pcm, frames);
    pcm_sync_ptr(pcm, 0);

    return frames;
}

int pcm_avail_update(struct pcm *pcm)
{
    pcm_sync_ptr(pcm, 0);
    return pcm_mmap_avail(pcm);
}

int pcm_state(struct pcm *pcm)
{
    int err = pcm_sync_ptr(pcm, 0);
    if (err < 0)
        return err;

    return pcm->mmap_status->state;
}

int pcm_set_avail_min(struct pcm *pcm, int avail_min)
{
    if ((~pcm->flags) & (PCM_MMAP | PCM_NOIRQ))
        return -ENOSYS;

    pcm->config.avail_min = avail_min;
    return 0;
}

int pcm_wait(struct pcm *pcm, int timeout)
{
    struct pollfd pfd;
    unsigned short revents = 0;
    int err;

    pfd.fd = pcm->fd;
    pfd.events = POLLOUT | POLLERR | POLLNVAL;

    do {
        /* let's wait for avail or timeout */
        err = poll(&pfd, 1, timeout);
        if (err < 0)
            return -errno;

        /* timeout ? */
        if (err == 0)
            return 0;

        /* have we been interrupted ? */
        if (errno == -EINTR)
            continue;

        /* check for any errors */
        if (pfd.revents & (POLLERR | POLLNVAL)) {
            switch (pcm_state(pcm)) {
            case PCM_STATE_XRUN:
                return -EPIPE;
            case PCM_STATE_SUSPENDED:
                return -ESTRPIPE;
            case PCM_STATE_DISCONNECTED:
                return -ENODEV;
            default:
                return -EIO;
            }
        }
    /* poll again if fd not ready for IO */
    } while (!(pfd.revents & (POLLIN | POLLOUT)));

    return 1;
}

int pcm_mmap_write(struct pcm *pcm, const void *buffer, unsigned int bytes)
{
    int err = 0, frames, avail;
    unsigned int offset = 0, count;

    if (bytes == 0)
        return 0;

    count = pcm_bytes_to_frames(pcm, bytes);

    while (count > 0) {

        /* get the available space for writing new frames */
        avail = pcm_avail_update(pcm);
        if (avail < 0) {
            db_error("cannot determine available mmap frames\n");
            return err;
        }

        /* start the audio if we reach the threshold */
	    if (!pcm->running &&
            (pcm->buffer_size - avail) >= pcm->config.start_threshold) {
            if (pcm_start(pcm) < 0) {
               db_error("start error: hw 0x%x app 0x%x avail 0x%x\n",
                       (unsigned int)pcm->mmap_status->hw_ptr,
                       (unsigned int)pcm->mmap_control->appl_ptr,
                       avail);
                return -errno;
            }
            pcm->wait_for_avail_min = 0;
        }

        /* sleep until we have space to write new frames */
        if (pcm->running) {
            /* enable waiting for avail_min threshold when less frames than we have to write
             * are available. */
            if (!pcm->wait_for_avail_min && (count > (unsigned int)avail))
                pcm->wait_for_avail_min = 1;

            if (pcm->wait_for_avail_min && (avail < pcm->config.avail_min)) {
                int time = -1;

                /* disable waiting for avail_min threshold to allow small amounts of data to be
                 * written without waiting as long as there is enough room in buffer. */
                pcm->wait_for_avail_min = 0;

                if (pcm->flags & PCM_NOIRQ)
                    time = (pcm->config.avail_min - avail) / pcm->noirq_frames_per_msec;

                err = pcm_wait(pcm, time);
                if (err < 0) {
                    pcm->running = 0;
                    oops(pcm, err, "wait error: hw 0x%x app 0x%x avail 0x%x\n",
                        (unsigned int)pcm->mmap_status->hw_ptr,
                        (unsigned int)pcm->mmap_control->appl_ptr,
                        avail);
                    pcm->mmap_control->appl_ptr = 0;
                    return err;
                }
                continue;
            }
        }

        frames = count;
        if (frames > avail)
            frames = avail;

        if (!frames)
            break;

        /* copy frames from buffer */
        frames = pcm_mmap_write_areas(pcm, buffer, offset, frames);
        if (frames < 0) {
            db_error("write error: hw 0x%x app 0x%x avail 0x%x\n",
                    (unsigned int)pcm->mmap_status->hw_ptr,
                    (unsigned int)pcm->mmap_control->appl_ptr,
                    avail);
            return frames;
        }

        offset += frames;
        count -= frames;
    }

_end:
    return 0;
}

/*  */
int pcm_get_node_number(char *name)
{
    char card[32];
    char id[32];
    int i = 0;
    int fd = 0;
    int ret = 0;

    for(i = 0; i < 10; i++){
        memset(card, 0, 32);
        memset(id, 0, 32);

        /* "/sys/class/sound/cardx" */
        sprintf(card, "/sys/class/sound/card%d", i);
        ret = access(card, F_OK);
        if(ret != 0){
            continue;
        }

        /* "/sys/class/sound/cardx/id" */
        strcat(card, "/id");
        ret = access(card, F_OK);
        if(ret != 0){
            continue;
        }

        /* compare */
        fd = open(card, O_RDONLY);
        if(fd < 0){
            continue;
        }

        ret = read(fd, id, 32);
        if(ret < 0){
            continue;
        }

{
    int j = 0;

    /*  */
    for(j = 0; j < 32; j++){
        if(id[j] == 0x0a){
            id[j] = 0;
        }
    }
}

        if(!strcmp(id, name)){
            return i;
        }
    }

    return -1;
}


