/*! \file empty.c
 *  \brief Empty gamestate.
 */
/*
 * Copyright (c) Sebastian Krzyszkowiak <dos@dosowisko.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../common.h"
#include <libsuperderpy.h>

struct GamestateResources {
		// This struct is for every resource allocated and used by your gamestate.
		// It gets created on load and then gets passed around to all other function calls.
		ALLEGRO_FONT *font;
		int blink_counter, counter;

		ALLEGRO_SHADER *shader;
		ALLEGRO_BITMAP *bg, *target, *frame;

		float fade_left, fade_right;
		bool forward;
		bool backward;
};

int Gamestate_ProgressCount = 1; // number of loading steps as reported by Gamestate_Load

void Gamestate_Logic(struct Game *game, struct GamestateResources* data) {
	// Called 60 times per second. Here you should do all your game logic.
	data->blink_counter++;
	data->counter++;
	if (data->blink_counter >= 60) {
		data->blink_counter = 0;
	}

	if (data->backward) {
		data->fade_left += 0.025;
		if (data->fade_left > 1) {
			data->fade_left = 1;
		}
	} else {
		data->fade_left -= 0.04;
		if (data->fade_left < 0) {
			data->fade_left = 0;
		}
	}

	if (data->forward) {
		data->fade_right += 0.025;
		if (data->fade_right > 1) {
			data->fade_right = 1;
		}
	} else {
		data->fade_right -= 0.04;
		if (data->fade_right < 0) {
			data->fade_right = 0;
		}
	}
}

void Gamestate_Draw(struct Game *game, struct GamestateResources* data) {
	// Called as soon as possible, but no sooner than next Gamestate_Logic call.
	// Draw everything to the screen here.
	if (data->blink_counter < 50) {
		al_draw_text(data->font, al_map_rgb(255,255,255), game->viewport.width / 2, game->viewport.height / 2,
								 ALLEGRO_ALIGN_CENTRE, "Nothing to see here, move along!");
	}


	al_set_target_bitmap(data->target);
	al_clear_to_color(al_map_rgba(0,0,0,0));
	al_use_shader(data->shader);

	float fade = data->fade_left;

	al_set_shader_bool("autoScan", true);
	al_set_shader_float("xScanline", 0.2);
	al_set_shader_float("xScanline2", 0.2);
	al_set_shader_float("yScanline", 1.0);
	al_set_shader_float("xScanlineSize", 0.5 * data->fade_left + 0.05);
	al_set_shader_float("xScanlineSize2", 0.83);
	al_set_shader_float("yScanlineAmount", -0.22 * data->fade_left);
	al_set_shader_float("grainLevel", 0.0 * data->fade_left);
	al_set_shader_float("scanFollowAmount", data->fade_left);
	al_set_shader_float("analogDistort", 6.66 * data->fade_left + 0.1);
	al_set_shader_float("bleedAmount", data->fade_left * 0.5);
	al_set_shader_float("bleedDistort", 0.666);
	al_set_shader_float("bleedRange", 1.0);
	al_set_shader_float("TIME", data->counter/60.0); //data->blink_counter/3600.0);
	float size[2] = {al_get_bitmap_width(data->target), al_get_bitmap_height(data->target)};
	al_set_shader_float_vector("RENDERSIZE", 2, size, 1);
	float colorl[4] = {0.8, 0.0, 0.4, 1.0};
	al_set_shader_float_vector("colorBleedL", 4, colorl, 1);
	float colorc[4] = {0.0, 0.5, 0.9, 1.0};
	al_set_shader_float_vector("colorBleedC", 4, colorc, 1);
	float colorr[4] = {0.8, 0.0, 0.4, 1.0};
	al_set_shader_float_vector("colorBleedR", 4, colorr, 1);
	al_draw_bitmap(data->bg, 0, 0, 0);
	al_use_shader(NULL);

	al_set_target_backbuffer(game->display);

	al_draw_tinted_scaled_rotated_bitmap_region(data->target, 0, 0, 1920/4, 1080/2, al_map_rgb_f(1,1,1), 0, 0, 0, 0, 2, 2, 0, 0);

	// right

	al_set_target_bitmap(data->target);
	al_clear_to_color(al_map_rgba(0,0,0,0));
	al_use_shader(data->shader);

	 fade = data->fade_right;


	al_set_shader_bool("autoScan", true);
	al_set_shader_float("xScanline", 0.2);
	al_set_shader_float("xScanline2", 0.175);
	al_set_shader_float("yScanline", 1.0);
	al_set_shader_float("xScanlineSize", 0.5 * data->fade_right + 0.05);
	al_set_shader_float("xScanlineSize2", 0.83);
	al_set_shader_float("yScanlineAmount", -0.22 * data->fade_right);
	al_set_shader_float("grainLevel", 0.0 * data->fade_right);
	al_set_shader_float("scanFollowAmount", data->fade_right);
	al_set_shader_float("analogDistort", 6.66 * data->fade_right + 0.1);
	al_set_shader_float("bleedAmount", data->fade_right * 0.5);
	al_set_shader_float("bleedDistort", 0.666);
	al_set_shader_float("bleedRange", 1.0);
	al_set_shader_float("TIME", data->counter/60.0 + 100); //data->blink_counter/3600.0);
	al_set_shader_float_vector("RENDERSIZE", 2, size, 1);
	al_set_shader_float_vector("colorBleedL", 4, colorl, 1);
	al_set_shader_float_vector("colorBleedC", 4, colorc, 1);
	al_set_shader_float_vector("colorBleedR", 4, colorr, 1);
	al_draw_bitmap(data->bg, 0, 0, 0);
	al_use_shader(NULL);

	al_set_target_backbuffer(game->display);

	al_draw_tinted_scaled_rotated_bitmap_region(data->target, 1920/4, 0, 1920/4, 1080/2, al_map_rgb_f(1,1,1), 0, 0, 1920/2, 0, 2, 2, 0, 0);


	al_draw_bitmap(data->frame, 0, 0, 0);

}

void Gamestate_ProcessEvent(struct Game *game, struct GamestateResources* data, ALLEGRO_EVENT *ev) {
	// Called for each event in Allegro event queue.
	// Here you can handle user input, expiring timers etc.
	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		UnloadCurrentGamestate(game); // mark this gamestate to be stopped and unloaded
		// When there are no active gamestates, the engine will quit.
	}

	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_RIGHT)) {
		data->forward = true;
	}
	if ((ev->type==ALLEGRO_EVENT_KEY_UP) && (ev->keyboard.keycode == ALLEGRO_KEY_RIGHT)) {
		data->forward = false;
	}
	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_LEFT)) {
		data->backward = true;
	}
	if ((ev->type==ALLEGRO_EVENT_KEY_UP) && (ev->keyboard.keycode == ALLEGRO_KEY_LEFT)) {
		data->backward = false;
	}
}

void* Gamestate_Load(struct Game *game, void (*progress)(struct Game*)) {
	// Called once, when the gamestate library is being loaded.
	// Good place for allocating memory, loading bitmaps etc.
	struct GamestateResources *data = calloc(1, sizeof(struct GamestateResources));
	al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);

	data->font = al_create_builtin_font();
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar

	data->bg = al_load_bitmap(GetDataFilePath(game, "bg.png"));
	data->frame = al_load_bitmap(GetDataFilePath(game, "frame.png"));
	data->target = al_create_bitmap(1920/2, 1080/2);

	data->shader = al_create_shader(ALLEGRO_SHADER_GLSL);
	PrintConsole(game, "VERTEX: %d", al_attach_shader_source_file(data->shader, ALLEGRO_VERTEX_SHADER, GetDataFilePath(game, "shaders/vertex.glsl")));
	const char* log;
	if ((log = al_get_shader_log(data->shader)) && (log[0])) {
		PrintConsole(game, "%s", log);
	}
	PrintConsole(game, "PIXEL: %d", al_attach_shader_source_file(data->shader, ALLEGRO_PIXEL_SHADER, GetDataFilePath(game, "shaders/vhs.glsl")));
	if ((log = al_get_shader_log(data->shader)) && (log[0])) {
		PrintConsole(game, "%s", log);
	}
	al_build_shader(data->shader);
	if ((log = al_get_shader_log(data->shader)) && (log[0])) {
		PrintConsole(game, "%s", log);
	}

	return data;
}

void Gamestate_Unload(struct Game *game, struct GamestateResources* data) {
	// Called when the gamestate library is being unloaded.
	// Good place for freeing all allocated memory and resources.
	al_destroy_font(data->font);

	al_destroy_shader(data->shader);
	al_destroy_bitmap(data->bg);
	al_destroy_bitmap(data->frame);
	al_destroy_bitmap(data->target);
	free(data);
}

void Gamestate_Start(struct Game *game, struct GamestateResources* data) {
	// Called when this gamestate gets control. Good place for initializing state,
	// playing music etc.
	data->blink_counter = 0;
	data->counter = 0;
	data->fade_left = 0;
	data->fade_right = 0;
	data->backward = false;
	data->forward = false;
}

void Gamestate_Stop(struct Game *game, struct GamestateResources* data) {
	// Called when gamestate gets stopped. Stop timers, music etc. here.
}

void Gamestate_Pause(struct Game *game, struct GamestateResources* data) {
	// Called when gamestate gets paused (so only Draw is being called, no Logic not ProcessEvent)
	// Pause your timers here.
}

void Gamestate_Resume(struct Game *game, struct GamestateResources* data) {
	// Called when gamestate gets resumed. Resume your timers here.
}

// Ignore this for now.
// TODO: Check, comment, refine and/or remove:
void Gamestate_Reload(struct Game *game, struct GamestateResources* data) {}
