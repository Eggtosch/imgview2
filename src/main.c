#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include <raymath.h>

#include "module.h"

struct module image_init(void);
struct module pdf_init(void);
struct module video_init(void);
struct module fallback_init(void);

#define N_MODULES 4
#define MOD_INDEX_IMAGE 0
#define MOD_INDEX_PDF 1
#define MOD_INDEX_VIDEO 2
#define MOD_INDEX_FALLBACK 3

struct state {
	struct module modules[N_MODULES];
	struct media *medias;
	int n_medias;
	const char **media_paths;
	int current_media;

	Camera2D camera;
	Font font;

	bool zoom;
	bool is_video;
	bool video_running;
};

static void stop_video(struct state *state) {
	if (!state->video_running) {
		return;
	}

	state->video_running = false;
	EnableEventWaiting();
}

static void start_video(struct state *state) {
	if (state->video_running) {
		return;
	}

	state->video_running = true;
	DisableEventWaiting();
	SetTargetFPS(video_get_fps(&state->medias[state->current_media]));
}

static void load_current_media(struct state *state) {
	const char *mediapath = state->media_paths[state->current_media];
	struct media *media = &state->medias[state->current_media];
	for (int i = 0; i < N_MODULES; i++) {
		if (state->modules[i].open(media, mediapath) == true) {
			state->is_video = i == MOD_INDEX_VIDEO;
			stop_video(state);
			return;
		}
	}

	state->is_video = false;
	stop_video(state);
	*media = (struct media){0};
}

static void render_text(struct state *state) {
	if (state->zoom) {
		return;
	}

	int current = state->current_media;
	struct media *media = &state->medias[current];
	const char *path = state->media_paths[current];
	const char *text = TextFormat("%d/%d: %s %s", current + 1, state->n_medias, path, media->text(media));
	DrawRectangleV((Vector2){0, 0}, MeasureTextEx(state->font, text, 20, 0), BLACK);
	DrawTextEx(state->font, text, (Vector2){0, 0}, 20, 0, WHITE);
}

static void redraw_current_media(struct state *state) {
	struct media *media = &state->medias[state->current_media];
	Texture2D t = media->texture;

	BeginDrawing();
	ClearBackground(BLACK);

	BeginMode2D(state->camera);
	int w = GetScreenWidth();
	int h = GetScreenHeight();
	float scale = fminf(w / (float) t.width, h / (float) t.height);
	Vector2 pos = {(w - t.width * scale) / 2, (h - t.height * scale) / 2};
	DrawTextureEx(t, pos, 0, scale, WHITE);
	EndMode2D();

	render_text(state);
	EndDrawing();
}

static int get_number(int key) {
	switch (key) {
		case KEY_ZERO: return 0;
		case KEY_ONE: return 1;
		case KEY_TWO: return 2;
		case KEY_THREE: return 3;
		case KEY_FOUR: return 4;
		case KEY_FIVE: return 5;
		case KEY_SIX: return 6;
		case KEY_SEVEN: return 7;
		case KEY_EIGHT: return 8;
		case KEY_NINE: return 9;
		default: return -1;
	}
}

int main(int argc, const char **argv) {
	if (argc < 2) {
		printf("Usage as zoom: %s --zoom <image>\n", argv[0]);
		printf("Usage: %s [images/pdfs...]\n", argv[0]);
		printf("Mouse- / Keybinds:\n");
		printf("  scrolling up: zooming in\n");
		printf("  scrolling down: zooming out\n");
		printf("  click and drag: move image around\n");
		printf("  r: reset image and position to fit screen\n");
		printf("  Arrow left/right: go to previous/next image\n");
		printf("  Arrow up/down: go to previous/next pdf page/video frame\n");
		printf("  Space: start video playback\n");
		return 1;
	}

	struct state state = {0};

	SetTraceLogLevel(LOG_NONE);
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(0, 0, "imgview2");
	state.font = LoadFont("/home/oskar/.local/share/fonts/NotoSansMonoNerd.ttf");
	SetTextureFilter(state.font.texture, TEXTURE_FILTER_TRILINEAR);

	state.modules[MOD_INDEX_IMAGE] = image_init();
	state.modules[MOD_INDEX_PDF] = pdf_init();
	state.modules[MOD_INDEX_VIDEO] = video_init();
	state.modules[MOD_INDEX_FALLBACK] = fallback_init();

	state.n_medias = argc - 1;
	state.media_paths = argv + 1;

	state.zoom = false;
	if (strcmp(argv[1], "--zoom") == 0) {
		state.zoom = true;
		state.n_medias--;
		state.media_paths++;
		ToggleFullscreen();
	}

	state.medias = calloc(state.n_medias, sizeof(struct media));
	state.current_media = 0;

	state.camera = (Camera2D){0};
	state.camera.zoom = 1;
	state.is_video = false;
	stop_video(&state);

	load_current_media(&state);

	EnableEventWaiting();
	while (!WindowShouldClose()) {
		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
			Vector2 delta = GetMouseDelta();
			delta = Vector2Scale(delta, -1.0 / state.camera.zoom);
			state.camera.target = Vector2Add(state.camera.target, delta);
		}

		float wheel = GetMouseWheelMove();
		if (wheel != 0) {
			Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), state.camera);
			state.camera.offset = GetMousePosition();
			state.camera.target = mouseWorldPos;
			state.camera.zoom *= wheel > 0 ? 1.25 : 1 / 1.25;
			state.camera.zoom = Clamp(state.camera.zoom, 0.00001, 500000);
		}

		int key = GetKeyPressed();
		switch (key) {
			case KEY_Q: goto loop_exit;
			case KEY_R: {
				state.camera = (Camera2D){0};
				state.camera.zoom = 1;
				break;
			}
			case KEY_RIGHT: {
				state.camera = (Camera2D){0};
				state.camera.zoom = 1;
				state.current_media = (state.current_media + 1) % state.n_medias;
				load_current_media(&state);
				break;
			}
			case KEY_LEFT: {
				state.camera = (Camera2D){0};
				state.camera.zoom = 1;
				state.current_media--;
				if (state.current_media < 0) {
					state.current_media = state.n_medias - 1;
				}
				load_current_media(&state);
				break;
			}
			case KEY_UP:
			case KEY_DOWN: {
				int shift = IsKeyDown(KEY_LEFT_SHIFT) ? state.is_video ? 50 : 5 : 1;
				int dir = key == KEY_UP ? -1 : 1;
				struct media *media = &state.medias[state.current_media];
				media->set_index(media, INDEX_RELATIVE, dir * shift);
				break;
			}
			default: {
				int n = get_number(key);
				if (n < 0) {
					break;
				}

				struct media *media = &state.medias[state.current_media];
				media->set_index(media, INDEX_ABSOLUTE, n);
				break;
			}
		}

		if (state.is_video && IsKeyPressed(KEY_SPACE)) {
			if (state.video_running) {
				stop_video(&state);
			} else {
				start_video(&state);
			}
		}

		if (state.video_running) {
			struct media *media = &state.medias[state.current_media];
			media->set_index(media, INDEX_RELATIVE, 1);
		}

		redraw_current_media(&state);
	}

loop_exit:
	CloseWindow();
	return 0;
}
