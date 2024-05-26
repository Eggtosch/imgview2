#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include "acinerella.h"
#include "module.h"

struct video {
	const char *path;
	int fd;

	lp_ac_instance instance;
	lp_ac_decoder decoder;

	double current_time;
	double fps;
	double duration;
};

int video_get_fps(struct media *media) {
	struct video *video = media->userdata;
	return video->fps;
}

static Texture2D video_draw_frame(struct video *video, int n) {
	lp_ac_package pckt = NULL;
	do {
		pckt = ac_read_package(video->instance);
		if (pckt == NULL) {
			return (Texture2D){0};
		} else if (pckt->stream_index != video->decoder->stream_index) {
			ac_free_package(pckt);
			continue;
		}

		if (ac_decode_package(pckt, video->decoder) == 0 || n > 1) {
			ac_free_package(pckt);
			if (video->decoder->timecode == 0 || n-- > 1) {
				continue;
			}

			return (Texture2D){0};
		}

		struct _ac_video_stream_info info = video->decoder->stream_info.additional_info.video_info;
		video->current_time = video->decoder->timecode;

		Image i = {
			.data = video->decoder->pBuffer,
			.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8,
			.width = info.frame_width,
			.height = info.frame_height,
			.mipmaps = 1,
		};
		Texture2D t = LoadTextureFromImage(i);
		ac_free_package(pckt);
		return t;
	} while (pckt != NULL);

	return (Texture2D){0};
}

// frame:   ffmpeg -i DJI_0703.MP4 -vf "select=eq(n\,34)" -vframes 1 -c:v bmp -f rawvideo - | pv > /dev/null
// fps:     ffmpeg -i DJI_0703.MP4 2>&1 | sed -n "s/.*, \(.*\) fp.*/\1/p"
// nframes: ffprobe -v error -select_streams v:0 -count_packets -show_entries stream=nb_read_packets -of csv=p=0 DJI_0703.MP4
static int video_read_cb(void *sender, uint8_t *buf, int size) {
	struct media *media = sender;
	struct video *video = media->userdata;
	return read(video->fd, buf, size);
}

static int64_t video_seek_cb(void *sender, int64_t pos, int whence) {
	struct media *media = sender;
	struct video *video = media->userdata;
	int64_t res = lseek(video->fd, pos, whence);
	return res;
}

static int video_open_cb(void *sender) {
	struct media *media = sender;
	struct video *video = media->userdata;
	video->fd = open(video->path, O_RDONLY);
	if (video->fd < 0) {
		return -1;
	}

	return 0;
}

static int video_close_cb(void *sender) {
	struct media *media = sender;
	struct video *video = media->userdata;
	return close(video->fd);
}

static const char *video_text(struct media *self) {
	struct video *video = self->userdata;
	if (self->texture.id > 0) {
		return TextFormat("frame %04d @ %g fps", (int) (video->current_time * video->fps), video->fps);
	} else {
		return "(unable to decode frame)";
	}
}

static void video_set_index(struct media *self, int inc_or_dec) {
	struct video *video = self->userdata;

	if (inc_or_dec < 0) {
		int current_time = 1000 * video->current_time;
		ac_seek(video->decoder, -1, current_time - 1000 / video->fps * abs(inc_or_dec));
	}

	UnloadTexture(self->texture);
	self->texture = video_draw_frame(video, abs(inc_or_dec));
}

static bool video_open(struct media *media, const char *mediapath) {
	struct video *video = calloc(1, sizeof(struct video));
	video->path = mediapath;

	*media = (struct media){
		.text = video_text,
		.set_index = video_set_index,
		.userdata = video,
		.texture = {0},
	};

	video->instance = ac_init();
	video->instance->output_format = AC_OUTPUT_RGB24;

	if (ac_open(video->instance, media, video_open_cb, video_read_cb, video_seek_cb, video_close_cb, NULL) < 0) {
		free(video);
		*media = (struct media){0};
		return false;
	}

	for (int i = 0; i < video->instance->stream_count; i++) {
		ac_stream_info info;
		ac_get_stream_info(video->instance, i, &info);
		if (info.stream_type != AC_STREAM_TYPE_VIDEO) {
			continue;
		}

		if (video->decoder == NULL) {
			video->decoder = ac_create_decoder(video->instance, i);
		}
	}

	video->fps = video->decoder->stream_info.additional_info.video_info.frames_per_second;
	video->duration = video->instance->info.duration / 1000.0;

	if (!video->instance->opened) {
		free(video);
		*media = (struct media){0};
		return false;
	}

	media->texture = video_draw_frame(video, 1);

	return true;
}

struct module video_init(void) {
	return (struct module) {
		.open = video_open,
	};
}
