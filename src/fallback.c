#include <string.h>

#include "module.h"

static const char *fallback_text(struct media *self) {
	(void) self;
	return "(unable to open)";
}

static void fallback_set_index(struct media *self, int index_mode, int amount) {
	(void) self;
	(void) index_mode;
	(void) amount;
}

static bool fallback_open(struct media *media, const char *mediapath) {
	(void) mediapath;
	*media = (struct media){
		.text = fallback_text,
		.set_index = fallback_set_index,
		.userdata = NULL,
		.texture = {0},
	};

	return true;
}

struct module fallback_init(void) {
	return (struct module) {
		.open = fallback_open,
	};
}
