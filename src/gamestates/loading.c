/*! \file loading.c
 *  \brief Loading screen.
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

/*! \brief Resources used by Loading state. */
struct GamestateResources {
	ALLEGRO_BITMAP *clock, *hand1, *hand2, *clockball, *bmp;
};

int Gamestate_ProgressCount = -1;

void Gamestate_ProcessEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev){};

void Gamestate_Logic(struct Game* game, struct GamestateResources* data){};

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	al_set_target_bitmap(data->bmp);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	float pos = al_get_time() / 60.0;
	al_draw_bitmap(data->clock, 0, 0, 0);
	al_draw_rotated_bitmap(data->hand2, 7, 15, 223 - 30, 875 - 589, pos * 4 * ALLEGRO_PI - ALLEGRO_PI * 0.5, 0);
	al_draw_rotated_bitmap(data->hand1, 8, 14, 223 - 30, 875 - 589, pos * 4 * ALLEGRO_PI * 24 - ALLEGRO_PI * 0.5, 0);
	al_draw_bitmap(data->clockball, 0, 0, 0);

	al_set_target_backbuffer(game->display);
	al_clear_to_color(al_map_rgb(24, 24, 24));
	al_draw_scaled_bitmap(data->bmp, 0, 0, al_get_bitmap_width(data->clock), al_get_bitmap_height(data->clock),
	  game->viewport.width / 2 - al_get_bitmap_width(data->clock) / 4, game->viewport.height / 2 - al_get_bitmap_height(data->clock) / 4,
	  al_get_bitmap_width(data->clock) / 2, al_get_bitmap_height(data->clock) / 2, 0);

	al_draw_filled_rectangle(0, game->viewport.height * 0.98, game->viewport.width, game->viewport.height, al_map_rgba(32, 32, 32, 32));
	al_draw_filled_rectangle(0, game->viewport.height * 0.98, game->loading_progress * game->viewport.width, game->viewport.height, al_map_rgba(128, 128, 128, 128));
};

void* Gamestate_Load(struct Game* game, void (*progress)(struct Game*)) {
	struct GamestateResources* data = malloc(sizeof(struct GamestateResources));
	data->clock = al_load_bitmap(GetDataFilePath(game, "clock1.png"));
	data->clockball = al_load_bitmap(GetDataFilePath(game, "clockball1.png"));
	data->hand1 = al_load_bitmap(GetDataFilePath(game, "hand1.png"));
	data->hand2 = al_load_bitmap(GetDataFilePath(game, "hand2.png"));
	data->bmp = CreateNotPreservedBitmap(al_get_bitmap_width(data->clock), al_get_bitmap_height(data->clock));
	return data;
}

void Gamestate_Unload(struct Game* game, struct GamestateResources* data) {
	free(data);
}

void Gamestate_Start(struct Game* game, struct GamestateResources* data) {}
void Gamestate_Stop(struct Game* game, struct GamestateResources* data) {}

void Gamestate_Reload(struct Game* game, struct GamestateResources* data) {
	data->bmp = CreateNotPreservedBitmap(al_get_bitmap_width(data->clock), al_get_bitmap_height(data->clock));
}

void Gamestate_Pause(struct Game* game, struct GamestateResources* data) {}
void Gamestate_Resume(struct Game* game, struct GamestateResources* data) {}
