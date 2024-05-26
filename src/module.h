#pragma once

#include <stdbool.h>
#include <raylib.h>

struct media {
	void (*set_index)(struct media *self, int inc_or_dec);
	const char* (*text)(struct media *self);
	void *userdata;
	Texture2D texture;
};

struct module {
	bool (*open)(struct media *media, const char *file);
};

int video_get_fps(struct media *media);
