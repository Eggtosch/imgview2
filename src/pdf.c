#include <string.h>
#include <stdio.h>
#include <mupdf/fitz.h>

#include "module.h"

struct pdf {
	fz_context *ctx;
	fz_document *doc;
	int npages;
	int current_page;
};

static struct pdf *pdf_new(const char *path) {
	struct pdf *pdf = malloc(sizeof(struct pdf));

	pdf->ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
	fz_register_document_handlers(pdf->ctx);

	fz_try (pdf->ctx) {
		pdf->doc = fz_open_document(pdf->ctx, path);
	} fz_catch (pdf->ctx) {
		fz_drop_context(pdf->ctx);
		return NULL;
	}

	pdf->npages = fz_count_pages(pdf->ctx, pdf->doc);
	pdf->current_page = 0;
	return pdf;
}

Texture2D pdf_draw(struct pdf *pdf) {
	static void *data = NULL;
	static uint64_t datasize = 0;

	fz_page *page = fz_load_page(pdf->ctx, pdf->doc, pdf->current_page);
	fz_rect r = fz_bound_page(pdf->ctx, page);
	float x_ref = 5000.0;
	float y_ref = x_ref * (r.y1 - r.y0) / (r.x1 - r.x0);
	fz_matrix ctm = fz_scale(x_ref / (r.x1 - r.x0), y_ref / (r.y1 - r.y0));
	fz_pixmap *pix = fz_new_pixmap_from_page(pdf->ctx, page, ctm, fz_device_rgb(pdf->ctx), 0);

	if (!pix) {
		return (Texture2D){0};
	}

	if (pix->n != 3) {
		fz_drop_pixmap(pdf->ctx, pix);
		return (Texture2D){0};
	}

	uint64_t size = (uint64_t) pix->w * (uint64_t) pix->h * (uint64_t) pix->n;
	if (size > datasize) {
		datasize = size;
		free(data);
		data = malloc(datasize);
	}

	memcpy(data, pix->samples, size);

	Image i = {
		.data = data,
		.width = pix->w,
		.height = pix->h,
		.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8,
		.mipmaps = 1,
	};

	fz_drop_pixmap(pdf->ctx, pix);
	return LoadTextureFromImage(i);
}

static const char *pdf_text(struct media *self) {
	struct pdf *pdf = self->userdata;
	return TextFormat("(%d/%d)", pdf->current_page + 1, pdf->npages);
}

static void pdf_set_index(struct media *self, int index_mode, int amount) {
	struct pdf *pdf = self->userdata;
	if (index_mode == INDEX_RELATIVE) {
		pdf->current_page += amount;
	} else {
		pdf->current_page = pdf->npages * amount / 10;
	}

	if (pdf->current_page >= pdf->npages) {
		pdf->current_page -= pdf->npages;
	} else if (pdf->current_page < 0) {
		pdf->current_page += pdf->npages;
	}

	UnloadTexture(self->texture);
	self->texture = pdf_draw(pdf);
}

static bool pdf_open(struct media *media, const char *mediapath) {
	if (!IsFileExtension(mediapath, ".pdf") && !IsFileExtension(mediapath, ".PDF")) {
		return false;
	}

	if (media->userdata) {
		return true;
	}

	*media = (struct media){
		.text = pdf_text,
		.set_index = pdf_set_index,
		.userdata = NULL,
		.texture = {0},
	};

	media->userdata = pdf_new(mediapath);
	media->texture = pdf_draw(media->userdata);
	return true;
}

struct module pdf_init(void) {
	return (struct module) {
		.open = pdf_open,
	};
}
