#include <raylib.h>
#include <stdlib.h>

#include "module.h"

static const char *image_text(struct media *self) {
	(void) self;
	return "";
}

static void image_set_index(struct media *self, int index_mode, int amount) {
	(void) self;
	(void) index_mode;
	(void) amount;
}

static bool image_try_open(struct media *media, const char *mediapath) {
	static struct media media_template = {
		.text = image_text,
		.set_index = image_set_index,
		.userdata = NULL,
		.texture = {0},
	};

	Image i = LoadImage(mediapath);
	if (i.data == NULL) {
		return false;
	}

	*media = media_template;
	media->texture = LoadTextureFromImage(i);
	UnloadImage(i);
	return true;
}

struct module image_init(void) {
	return (struct module) {
		.open = image_try_open,
	};
}
