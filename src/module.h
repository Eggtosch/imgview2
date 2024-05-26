#pragma once

#include <stdbool.h>
#include <raylib.h>

#define INDEX_RELATIVE 0
#define INDEX_ABSOLUTE 1

struct media {
	void (*set_index)(struct media *self, int index_mode, int amount);
	const char* (*text)(struct media *self);
	void *userdata;
	Texture2D texture;
};

struct module {
	bool (*open)(struct media *media, const char *file);
};

int video_get_fps(struct media *media);
