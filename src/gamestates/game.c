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
#include <math.h>
#include <libsuperderpy.h>

struct GamestateResources {
		// This struct is for every resource allocated and used by your gamestate.
		// It gets created on load and then gets passed around to all other function calls.
		int counter;

		ALLEGRO_SHADER *shader;
		ALLEGRO_BITMAP *bg, *bg2, *target, *frame, *scene;

		ALLEGRO_BITMAP *clock1, *clock2, *clockball1, *clockball2, *hand1, *hand2, *ball, *trees, *scores;

		ALLEGRO_BITMAP *leaf;

		float fade_left, fade_right;
		bool forward_left, forward_right;
		bool backward_left, backward_right;
		bool lastbackward_left, lastbackward_right;

		double time_left, time_right;

		float dx, dy;

		float ballx, bally;

		float cooldown;

		bool started;

		bool scoreleft;

		int leftscore, rightscore;
};

int Gamestate_ProgressCount = 1; // number of loading steps as reported by Gamestate_Load

bool IsBetween(float val, float lim1, float lim2) {
	return ((val >= lim1) && (val <= lim2)) || ((val >= lim2) && (val <= lim1));
}

bool CheckCollision(struct Game *game, struct GamestateResources* data, int Px, int Py, int Pw, double angle, double speed) {
	if (data->cooldown) return false;

	float m = data->ballx, n = data->bally, r = 42;
//	float angle = data->time_right * 4 * ALLEGRO_PI * 24;
	float a = tan(angle);
//	int Px = 1672, Py = 879, Pw = 138;
	float b = Py - a*Px;
	float x1 = (sqrt(-a*a*m*m+a*a*r*r-2*a*b*m+2*a*m*n-b*b+2*b*n-n*n+r*r) - a*b + a*n + m) / (a*a + 1);
	float x2 = (-sqrt(-a*a*m*m+a*a*r*r-2*a*b*m+2*a*m*n-b*b+2*b*n-n*n+r*r) - a*b + a*n + m) / (a*a + 1);
	float y1 = a*x1+b;
	float y2 = a*x2+b;

	int xlim = Px + cos(angle)*Pw;

	al_draw_line(xlim, 0, xlim, 1080, al_map_rgb(0,255,255), 1);

	if ((!isnan(x1)) && (IsBetween(x1, Px, xlim))) {
		al_draw_filled_circle(x1, y1, 10, al_map_rgb(0,255,255));
		data->dx = -data->dx * (1.1 + speed / 3.0);
	//	data->ballx += data->dx * 4;

		data->dy = ((n - y1)/fabs(n - y1)) * 5.0;
		data->dy *= 0.9 + ((speed / 3.0) + (rand()/(float)RAND_MAX)/12.0)/2.0;

		//PrintConsole(game, "dx %f, dy %f", data->dx, data->dy);

		data->cooldown = 10;

		return true;
	} else if ((!isnan(x2)) && (IsBetween(x2, Px, xlim))) {
		al_draw_filled_circle(x2, y2, 10, al_map_rgb(0,255,255));
		data->dx = -data->dx * (1.1 + speed / 3.0);
//		data->ballx += data->dx * 4;

		data->dy = ((n - y2)/fabs(n - y2)) * 5.0;
		data->dy *= 0.9 + ((speed / 3.0) + (rand()/(float)RAND_MAX)/12.0)/2.0;

		//PrintConsole(game, "dx %f, dy %f", data->dx, data->dy);

		data->cooldown = 10;

		return true;
	}
	return false;
}


void Gamestate_Logic(struct Game *game, struct GamestateResources* data) {
	// Called 60 times per second. Here you should do all your game logic.
	data->counter++;

	if ((data->bally > (1080+42)) || (data->ballx < -42) || (data->ballx > (1920+42))) {
		if (data->scoreleft) {
			PrintConsole(game, "POINT FOR LEFT");
			data->leftscore ++;
		} else{
			PrintConsole(game, "POINT FOR RIGHT");
			data->rightscore ++;
		}
		data->dx = 12 * (((rand()%2) * 2) - 1);
		data->dy = 4;
		data->ballx = 1920/2;
		data->bally = 1080/2 - 100;
		data->started = true;
		data->scoreleft = (data->dx < 0);
	}

	if ((data->leftscore >= 10) || (data->rightscore >= 10)) {
		data->started = false;
	}

	if (data->time_left > 1) {
		data->time_left -= 1;
	}
	if (data->time_right > 1) {
		data->time_right -= 1;
	}
	if (data->time_left < 0) {
		data->time_left += 1;
	}
	if (data->time_right < 0) {
		data->time_right += 1;
	}

	if (data->started) {
		data->ballx += data->dx;
		data->bally += data->dy;

		data->dy += 0.1;
		data->dx += 0.005 * ((data->dx > 0) ? -1 : 1);
	}

	float x1 = data->ballx, y1 = data->bally;
	if (data->scoreleft) {
		float x2 = 223, y2 = 875;
		if ((sqrt(pow(x2-x1,2)+pow(y2-y1,2))) <= (42 + 150)) {
			PrintConsole(game, "score right");
			data->scoreleft = false;
		}
	} else {
		float x2 = 1672, y2 = 879;
		if ((sqrt(pow(x2-x1,2)+pow(y2-y1,2))) <= (42 + 150)) {
			PrintConsole(game, "score left");
			data->scoreleft = true;
		}

	}

	data->time_left += 1.0 / 24.0 / 60.0 / 60.0;
	data->time_right += 1.0 / 24.0 / 60.0 / 60.0;

	data->time_left += (1.0 / 24.0 / 60.0) * (data->lastbackward_left ? -data->fade_left : data->fade_left);
	data->time_right += (1.0 / 24.0 / 60.0) * (data->lastbackward_right ? -data->fade_right : data->fade_right);

	if (data->cooldown) {
		data->cooldown--;
	}

	CheckCollision(game, data, 1672, 879, 115, data->time_right * 4 * ALLEGRO_PI, data->fade_left);
	CheckCollision(game, data, 1672, 879, 138, data->time_right * 4 * ALLEGRO_PI * 24, data->fade_left / 10.0);

	CheckCollision(game, data, 223, 875, 115, data->time_left * 4 * ALLEGRO_PI, data->fade_right);
	CheckCollision(game, data, 223, 875, 138, data->time_left * 4 * ALLEGRO_PI * 24, data->fade_right / 10.0);


	if (data->backward_left || data->forward_left) {
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

	if (data->backward_right || data->forward_right) {
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

void DrawScene(struct Game *game, struct GamestateResources* data, double time) {
	al_set_target_bitmap(data->scene);
	al_clear_to_color(al_map_rgba(0,0,0,0));
	al_draw_bitmap(data->bg, 0, 0, 0);

	float night = 0;
	if ((time > 0.25) && (time < 0.5)) {
		night = (time - 0.25) * 4;
		if (night < 0.5) {
			night = fmin(night*5, 1.0);
		} else {
			night = 1.0-fmax((night-0.5)*2, 0.0);
		}
		//fmin(fmin(night*5, 1.0), fmax(1.0-(night*5), 0));
	}
//	PrintConsole(game, "time=%f, night=%f", time, night);
	al_draw_tinted_bitmap(data->bg2, al_map_rgba_f(night, night, night, night), 0, 0, 0);

	al_draw_bitmap(data->leaf, 500 * sin(time*200.0) + 1920/2 - 150, 260, 0);
	al_draw_bitmap(data->trees, 0, 0, 0);

	al_draw_filled_rectangle(0, 0, 1920, 1080, al_map_rgba_f(0,0,0,night*0.333));
}

void Gamestate_Draw(struct Game *game, struct GamestateResources* data) {
	// Called as soon as possible, but no sooner than next Gamestate_Logic call.
	// Draw everything to the screen here.

	DrawScene(game, data, data->time_left);

	al_set_target_bitmap(data->target);
	al_clear_to_color(al_map_rgba(0,0,0,0));
	al_use_shader(data->shader);

	float fade = data->fade_left;

	al_set_shader_bool("autoScan", true);
	al_set_shader_float("xScanline", 0.2);
	al_set_shader_float("xScanline2", 0.2);
	al_set_shader_float("yScanline", 1.0);
	al_set_shader_float("xScanlineSize", 0.5 * fade + 0.05);
	al_set_shader_float("xScanlineSize2", 0.83);
	al_set_shader_float("yScanlineAmount", -0.22 * fade);
	al_set_shader_float("grainLevel", 0.0 * fade);
	al_set_shader_float("scanFollowAmount", fade);
	al_set_shader_float("analogDistort", 6.66 * fade + 0.1);
	al_set_shader_float("bleedAmount", fade * 0.5);
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
	al_draw_scaled_bitmap(data->scene, 0, 0, 1920, 1080, 0, 0, 1920/2, 1080/2, 0);
	al_use_shader(NULL);

	al_set_target_backbuffer(game->display);

	al_draw_tinted_scaled_rotated_bitmap_region(data->target, 0, 0, 1920/4, 1080/2, al_map_rgb_f(1,1,1), 0, 0, 0, 0, 2, 2, 0, 0);

	// right

	DrawScene(game, data, data->time_right);

	al_set_target_bitmap(data->target);
	al_clear_to_color(al_map_rgba(0,0,0,0));
	al_use_shader(data->shader);

	 fade = data->fade_right;


	al_set_shader_bool("autoScan", true);
	al_set_shader_float("xScanline", 0.2);
	al_set_shader_float("xScanline2", 0.175);
	al_set_shader_float("yScanline", 1.0);
	al_set_shader_float("xScanlineSize", 0.5 * fade + 0.05);
	al_set_shader_float("xScanlineSize2", 0.83);
	al_set_shader_float("yScanlineAmount", -0.22 * fade);
	al_set_shader_float("grainLevel", 0.0 * fade);
	al_set_shader_float("scanFollowAmount", fade);
	al_set_shader_float("analogDistort", 6.66 * fade + 0.1);
	al_set_shader_float("bleedAmount", fade * 0.5);
	al_set_shader_float("bleedDistort", 0.666);
	al_set_shader_float("bleedRange", 1.0);
	al_set_shader_float("TIME", data->counter/60.0 + 100); //data->blink_counter/3600.0);
	al_set_shader_float_vector("RENDERSIZE", 2, size, 1);
	al_set_shader_float_vector("colorBleedL", 4, colorl, 1);
	al_set_shader_float_vector("colorBleedC", 4, colorc, 1);
	al_set_shader_float_vector("colorBleedR", 4, colorr, 1);
	al_draw_scaled_bitmap(data->scene, 0, 0, 1920, 1080, 0, 0, 1920/2, 1080/2, 0);
	al_use_shader(NULL);

	al_set_target_backbuffer(game->display);

	al_draw_tinted_scaled_rotated_bitmap_region(data->target, 1920/4, 0, 1920/4, 1080/2, al_map_rgb_f(1,1,1), 0, 0, 1920/2, 0, 2, 2, 0, 0);

	al_draw_bitmap(data->frame, 0, 0, 0);

	al_draw_bitmap(data->clock1, 30, 589, 0);
	al_draw_rotated_bitmap(data->hand2, 7, 15, 223, 875, data->time_left * 4 * ALLEGRO_PI, 0);
	al_draw_rotated_bitmap(data->hand1, 8, 14, 223, 875, data->time_left * 4 * ALLEGRO_PI * 24, 0);
	al_draw_bitmap(data->clockball1, 30, 589, 0);


	al_draw_bitmap(data->clock2, 1491, 552, 0);
	al_draw_rotated_bitmap(data->hand2, 7, 15, 1672, 879, data->time_right * 4 * ALLEGRO_PI, 0);
	al_draw_rotated_bitmap(data->hand1, 8, 14, 1672, 879, data->time_right * 4 * ALLEGRO_PI * 24, 0);
	al_draw_bitmap(data->clockball2, 1491, 552, 0);


	al_draw_bitmap(data->scores, 462, 960, 0);

	al_draw_textf(game->_priv.font_console, al_map_rgb(0,0,0), 512, 990, ALLEGRO_ALIGN_LEFT, "Now: %d", data->leftscore);
	al_draw_textf(game->_priv.font_console, al_map_rgb(0,0,0), 1122, 982, ALLEGRO_ALIGN_LEFT, "Then: %d", data->rightscore);

	al_draw_scaled_rotated_bitmap(data->ball, al_get_bitmap_width(data->ball)/2, al_get_bitmap_height(data->ball)/2,
																data->ballx, data->bally, 0.75, 0.75, 0, 0);


/*
	al_draw_line(223, 875, 223 + 115 * cos(data->time_left * 4 * ALLEGRO_PI), 875 + 115 * sin(data->time_left * 4 * ALLEGRO_PI), al_map_rgb(255,255,0), 5);
	al_draw_line(223, 875, 223 + 138 * cos(data->time_left * 4 * ALLEGRO_PI * 24), 875 + 138 * sin(data->time_left * 4 * ALLEGRO_PI * 24), al_map_rgb(255,255,0), 5);

	al_draw_line(1672, 879, 1672 + 115 * cos(data->time_right * 4 * ALLEGRO_PI), 879 + 115 * sin(data->time_right * 4 * ALLEGRO_PI), al_map_rgb(255,255,0), 5);
	al_draw_line(1672, 879, 1672 + 138 * cos(data->time_right * 4 * ALLEGRO_PI * 24), 879 + 138 * sin(data->time_right * 4 * ALLEGRO_PI * 24), al_map_rgb(255,255,0), 5);

	al_draw_circle(data->ballx, data->bally, 42, al_map_rgb(255,0,0), 5);
*/

//	PrintConsole(game, "%f %f", x1, x2);
}

void Gamestate_ProcessEvent(struct Game *game, struct GamestateResources* data, ALLEGRO_EVENT *ev) {
	// Called for each event in Allegro event queue.
	// Here you can handle user input, expiring timers etc.
	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		UnloadCurrentGamestate(game); // mark this gamestate to be stopped and unloaded
		// When there are no active gamestates, the engine will quit.
	}

	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_RIGHT)) {
		data->forward_right = true;
		data->lastbackward_right = false;
	}
	if ((ev->type==ALLEGRO_EVENT_KEY_UP) && (ev->keyboard.keycode == ALLEGRO_KEY_RIGHT)) {
		data->forward_right = false;
	}
	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_LEFT)) {
		data->backward_right = true;
		data->lastbackward_right = true;
	}
	if ((ev->type==ALLEGRO_EVENT_KEY_UP) && (ev->keyboard.keycode == ALLEGRO_KEY_LEFT)) {
		data->backward_right = false;
	}

	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_D)) {
		data->forward_left = true;
		data->lastbackward_left = false;
	}
	if ((ev->type==ALLEGRO_EVENT_KEY_UP) && (ev->keyboard.keycode == ALLEGRO_KEY_D)) {
		data->forward_left = false;
	}
	if ((ev->type==ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_A)) {
		data->backward_left = true;
		data->lastbackward_left = true;
	}
	if ((ev->type==ALLEGRO_EVENT_KEY_UP) && (ev->keyboard.keycode == ALLEGRO_KEY_A)) {
		data->backward_left = false;
	}
	if ((ev->type==ALLEGRO_EVENT_KEY_UP) && (ev->keyboard.keycode == ALLEGRO_KEY_SPACE)) {
		data->dx = 12 * (((rand()%2) * 2) - 1);
		data->dy = 4;
		data->ballx = 1920/2;
		data->bally = 1080/2 - 100;
		data->started = true;
		data->leftscore = 0;
		data->rightscore = 0;
		data->scoreleft = (data->dx < 0);
	}

}

void* Gamestate_Load(struct Game *game, void (*progress)(struct Game*)) {
	// Called once, when the gamestate library is being loaded.
	// Good place for allocating memory, loading bitmaps etc.
	struct GamestateResources *data = calloc(1, sizeof(struct GamestateResources));
	al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);

	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar

	data->bg = al_load_bitmap(GetDataFilePath(game, "bg.png"));
	data->bg2 = al_load_bitmap(GetDataFilePath(game, "bg2.png"));
	data->frame = al_load_bitmap(GetDataFilePath(game, "frame.png"));
	data->trees = al_load_bitmap(GetDataFilePath(game, "trees.png"));
	data->target = al_create_bitmap(1920/2, 1080/2);
	data->scene = al_create_bitmap(1920, 1080);

	data->leaf = al_load_bitmap(GetDataFilePath(game, "leaf.png"));

	data->ball = al_load_bitmap(GetDataFilePath(game, "ball.png"));

	data->clock1 = al_load_bitmap(GetDataFilePath(game, "clock1.png"));
	data->clock2 = al_load_bitmap(GetDataFilePath(game, "clock2.png"));
	data->clockball1 = al_load_bitmap(GetDataFilePath(game, "clockball1.png"));
	data->clockball2 = al_load_bitmap(GetDataFilePath(game, "clockball2.png"));
	data->hand1 = al_load_bitmap(GetDataFilePath(game, "hand1.png"));
	data->hand2 = al_load_bitmap(GetDataFilePath(game, "hand2.png"));
	data->scores = al_load_bitmap(GetDataFilePath(game, "scores.png"));

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

	al_destroy_shader(data->shader);
	al_destroy_bitmap(data->bg);
	al_destroy_bitmap(data->frame);
	al_destroy_bitmap(data->target);
	free(data);
}

void Gamestate_Start(struct Game *game, struct GamestateResources* data) {
	// Called when this gamestate gets control. Good place for initializing state,
	// playing music etc.
	data->counter = 0;
	data->cooldown = 0;
	data->fade_left = 0;
	data->fade_right = 0;
	data->time_left = 0;
	data->time_right = 0;
	data->backward_left = false;
	data->forward_right = false;
	data->backward_right = false;
	data->forward_left = false;
	data->lastbackward_left = false;
	data->lastbackward_right = false;

	data->started = false;
	data->dx = 0;
	data->dy = 0;
	data->ballx = 1920/2;
	data->bally = 1080/2 - 100;

	data->leftscore = 0;
	data->rightscore = 0;
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
