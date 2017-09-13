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
#include <math.h>

struct AnimalRes {
	ALLEGRO_BITMAP *bitmap, *bitmap_sitting;
	int benchPos;
	int zIndex;
};

enum ANIMAL_STATE {
	ANIMAL_WALKING,
	ANIMAL_BENCH_LEFT,
	ANIMAL_BENCH_RIGHT,
	ANIMAL_BENCH_CENTER
};

struct Path {
	double a, b;
	double start, stop;
	bool bench;
	int zIndex;
	unsigned int successorsLeft;
	unsigned int successorsRight;
	struct Path* successors[];
};

struct Animal {
	struct {
		int spawn, despawn;
	} time;
	enum ANIMAL_STATE state;
	bool reverse;
	struct Path* path;
	struct AnimalRes* type;
	double speed;
	int id;
};

struct DrawCommand {
	struct Animal* animal;
	ALLEGRO_BITMAP* bitmap;
	float cx, cy;
	float x, y;
	float angle;
	int flags;
};

struct GamestateResources {
	// This struct is for every resource allocated and used by your gamestate.
	// It gets created on load and then gets passed around to all other function calls.
	int counter;

	ALLEGRO_FONT *big, *small, *scorefont;
	ALLEGRO_SHADER* shader;
	ALLEGRO_BITMAP *bg, *bg2, *fg, *fg2, *target, *frame, *scene, *bee1, *bee2, *bee3;

	ALLEGRO_SAMPLE *yay1s, *yay2s, *yay3s, *balls;
	ALLEGRO_SAMPLE_INSTANCE *yay1, *yay2, *yay3, *ballsound;

	ALLEGRO_AUDIO_STREAM *day1, *day2, *night1, *night2, *rewind, *music;

	ALLEGRO_BITMAP *clock1, *clock2, *clockball1, *clockball2, *hand1, *hand2, *ball, *trees, *scores, *scorebmp;

	struct AnimalRes dzik, ostronos, owca, leaf;

	float fade_left, fade_right;
	bool forward_left, forward_right;
	bool backward_left, backward_right;
	bool lastbackward_left, lastbackward_right;

	double time_left, time_right;

	float dx, dy;

	int score;
	int delay;

	float ballx, bally, ballrot;

	int cooldown;

	bool started;

	bool scoreleft;
	bool lastleft;

	int shakeleft;
	int shakeright;

	int leftscore, rightscore;

	struct Animal* animals;
	struct DrawCommand* drawCommandBuffer;
	unsigned int animalsCount;
	unsigned int animalsAllocated;

	int curId;

	struct Path* paths[16];
};

// "tick" is the smallest unit of operation
// (one minute in-game, one second in real life)
#define TICKS_PER_DAY (24 * 60)
#define AVG_TICKS_PER_ANIMAL 16
#define AVG_BENCHING_TIME 32
#define MIN_BENCHING_TIME 5
#define AT_NIGHT_SUPPRESSION 0.5
#define SCREENSHAKE 20

int Gamestate_ProgressCount = 38; // number of loading steps as reported by Gamestate_Load

static bool IsBetween(float val, float lim1, float lim2) {
	return ((val >= lim1) && (val <= lim2)) || ((val >= lim2) && (val <= lim1));
}

static double NightValue(double time) {
	double night = 0;
	if ((time > 0.25) && (time < 0.5)) {
		night = (time - 0.25) * 4;
		if (night < 0.5) {
			night = fmin(night * 5, 1.0);
		} else {
			night = 1.0 - fmax((night - 0.5) * 2, 0.0);
		}
	}
	return night;
}

static bool CheckCollision(struct Game* game, struct GamestateResources* data, int Px, int Py, int Pw, double angle, double speed) {
	//	PrintConsole(game, "sizeof %d", sizeof(paths.successors)/sizeof(paths.successors[0]));
	if (data->cooldown) {
		return false;
	}

	float m = data->ballx, n = data->bally, r = 42;
	//	float angle = data->time_right * 4 * ALLEGRO_PI * 24;
	float a = tan(angle);
	//	int Px = 1672, Py = 879, Pw = 138;
	float b = Py - a * Px;
	float x1 = (sqrt(-a * a * m * m + a * a * r * r - 2 * a * b * m + 2 * a * m * n - b * b + 2 * b * n - n * n + r * r) - a * b + a * n + m) / (a * a + 1);
	float x2 = (-sqrt(-a * a * m * m + a * a * r * r - 2 * a * b * m + 2 * a * m * n - b * b + 2 * b * n - n * n + r * r) - a * b + a * n + m) / (a * a + 1);
	float y1 = a * x1 + b;
	float y2 = a * x2 + b;

	int xlim = Px + cos(angle) * Pw;

	//al_draw_line(xlim, 0, xlim, 1080, al_map_rgb(0,255,255), 1);

	if ((!isnan(x1)) && (IsBetween(x1, Px, xlim))) {
		//al_draw_filled_circle(x1, y1, 10, al_map_rgb(0,255,255));
		data->dx = -data->dx * (1.03 + speed / 6.0);
		//data->ballx += data->dx * 4;

		data->dy = ((n - y1) / fabs(n - y1)) * 5.0;
		data->dy *= 1.0 + ((speed / 8.0) + (rand() / (float)RAND_MAX) / 12.0) / 4.0;

		//PrintConsole(game, "dx %f, dy %f", data->dx, data->dy);

		data->cooldown = 10;
		al_set_sample_instance_speed(data->ballsound, 0.9 + (rand() / (float)RAND_MAX) * 0.2);
		al_play_sample_instance(data->ballsound);

		return true;
	}
	if ((!isnan(x2)) && (IsBetween(x2, Px, xlim))) {
		//al_draw_filled_circle(x2, y2, 10, al_map_rgb(0,255,255));
		data->dx = -data->dx * (1.03 + speed / 6.0);
		//data->ballx += data->dx * 4;

		data->dy = ((n - y2) / fabs(n - y2)) * 5.0;
		data->dy *= 1.0 + ((speed / 8.0) + (rand() / (float)RAND_MAX) / 12.0) / 4.0;

		//PrintConsole(game, "dx %f, dy %f", data->dx, data->dy);

		data->cooldown = 10;

		al_play_sample_instance(data->ballsound);
		return true;
	}
	return false;
}

void Gamestate_Logic(struct Game* game, struct GamestateResources* data) {
	// Called 60 times per second. Here you should do all your game logic.
	data->counter++;

	al_set_audio_stream_gain(data->rewind, fmax(data->fade_left, data->fade_right) * 2);
	al_set_audio_stream_speed(data->rewind, fmax(0.01, fmax(data->fade_left, data->fade_right)));

	data->ballrot += 0.02 + fabs(data->dx) * 0.0025 + fabs(data->dy) * 0.0025;

	if (data->delay >= 0) {
		data->delay--;
	}
	if (data->shakeleft > 0) {
		data->shakeleft--;
	}
	if (data->shakeright > 0) {
		data->shakeright--;
	}

	if ((data->bally > (1080 + 42)) || (data->ballx < -42) || (data->ballx > (1920 + 42))) {
		if (data->delay == -1) {
			if (data->scoreleft) {
				PrintConsole(game, "POINT FOR LEFT %d", data->score);
				data->leftscore++;
				data->shakeright = SCREENSHAKE;
			} else {
				PrintConsole(game, "POINT FOR RIGHT %d", data->score);
				data->rightscore++;
				data->shakeleft = SCREENSHAKE;
			}
			ALLEGRO_SAMPLE_INSTANCE* yays[3] = {data->yay1, data->yay2, data->yay3};
			al_play_sample_instance(yays[rand() % 3]);

			ALLEGRO_BITMAP* target = al_get_target_bitmap();
			al_set_target_bitmap(data->scorebmp);
			al_clear_to_color(al_map_rgba(0, 0, 0, 0));

			al_draw_textf(data->scorefont, al_map_rgb(0, 0, 0), 1920 / 2 - 6, 450 - 6, ALLEGRO_ALIGN_CENTER, "%d", data->score);
			al_draw_textf(data->scorefont, al_map_rgb(0, 0, 0), 1920 / 2 + 6, 450 + 6, ALLEGRO_ALIGN_CENTER, "%d", data->score);
			al_draw_textf(data->scorefont, al_map_rgb(0, 0, 0), 1920 / 2 - 6, 450 + 6, ALLEGRO_ALIGN_CENTER, "%d", data->score);
			al_draw_textf(data->scorefont, al_map_rgb(0, 0, 0), 1920 / 2 + 6, 450 - 6, ALLEGRO_ALIGN_CENTER, "%d", data->score);
			al_draw_textf(data->scorefont, al_map_rgb(0, 0, 0), 1920 / 2 + 0, 450 + 6, ALLEGRO_ALIGN_CENTER, "%d", data->score);
			al_draw_textf(data->scorefont, al_map_rgb(0, 0, 0), 1920 / 2 + 0, 450 - 6, ALLEGRO_ALIGN_CENTER, "%d", data->score);
			al_draw_textf(data->scorefont, al_map_rgb(0, 0, 0), 1920 / 2 + 6, 450 + 0, ALLEGRO_ALIGN_CENTER, "%d", data->score);
			al_draw_textf(data->scorefont, al_map_rgb(0, 0, 0), 1920 / 2 - 6, 450 + 0, ALLEGRO_ALIGN_CENTER, "%d", data->score);
			al_draw_textf(data->scorefont, al_map_rgb(255, 255, 255), 1920 / 2, 450, ALLEGRO_ALIGN_CENTER, "%d", data->score);

			al_set_target_bitmap(target);

			data->delay = 120;
		} else if (data->delay == 60) {
			data->dx = 12 * (((rand() % 2) * 2) - 1);
			data->dy = 4;
			data->ballx = 1920 / 2 - 10;
			data->bally = 1080 / 2 - 100;
			data->started = true;
			data->scoreleft = (data->dx < 0);
			data->lastleft = data->scoreleft;
			data->score = 0;
		}
	}

	if ((data->leftscore >= 10) || (data->rightscore >= 10)) {
		data->started = false;
		al_set_audio_stream_playing(data->music, false);
	}

	double night = NightValue(data->time_left);
	al_set_audio_stream_gain(data->day1, 1.0 - night);
	al_set_audio_stream_gain(data->night1, night);
	al_set_audio_stream_speed(data->day1, 1.0 + data->fade_left);
	al_set_audio_stream_speed(data->night1, 1.0 + data->fade_left);

	night = NightValue(data->time_right);
	al_set_audio_stream_gain(data->day2, 1.0 - night);
	al_set_audio_stream_gain(data->night2, night);
	al_set_audio_stream_speed(data->day2, 1.0 + data->fade_right);
	al_set_audio_stream_speed(data->night2, 1.0 + data->fade_right);

	if (data->started) {
		data->ballx += data->dx;
		data->bally += data->dy;

		data->dy += 0.1;
		//		data->dx += 0.005 * ((data->dx > 0) ? -1 : 1);
	}

	float x1 = data->ballx, y1 = data->bally;
	if (data->scoreleft) {
		float x2 = 223, y2 = 875;
		if ((sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2))) <= (42 + 150)) {
			PrintConsole(game, "score right");
			data->scoreleft = false;
		}
	} else {
		float x2 = 1672, y2 = 879;
		if ((sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2))) <= (42 + 150)) {
			PrintConsole(game, "score left");
			data->scoreleft = true;
		}
	}

	data->time_left += 1.0 / 24.0 / 60.0 / 60.0;
	data->time_right += 1.0 / 24.0 / 60.0 / 60.0;

	data->time_left += (1.0 / 24.0 / 60.0) * (data->lastbackward_left ? -data->fade_left : data->fade_left);
	data->time_right += (1.0 / 24.0 / 60.0) * (data->lastbackward_right ? -data->fade_right : data->fade_right);

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

	if (data->cooldown) {
		data->cooldown--;
	}

	bool left = false;
	left = left || CheckCollision(game, data, 1672, 879, 115, data->time_right * 4 * ALLEGRO_PI, data->fade_right);
	left = left || CheckCollision(game, data, 1672, 879, 138, data->time_right * 4 * ALLEGRO_PI * 24, data->fade_right / 10.0);

	bool right = false;
	right = right || CheckCollision(game, data, 223, 875, 115, data->time_left * 4 * ALLEGRO_PI, data->fade_left);
	right = right || CheckCollision(game, data, 223, 875, 138, data->time_left * 4 * ALLEGRO_PI * 24, data->fade_left / 10.0);

	if (right && data->lastleft) {
		data->score++;
		data->lastleft = false;
	} else if (left && !data->lastleft) {
		data->score++;
		data->lastleft = true;
	}

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

static struct Path* InitPath(int x1, int y1, int x2, int y2, int successorsLeft, int successorsRight, int zIndex) {
	struct Path* path = calloc(1, sizeof(struct Path) + sizeof(struct Path*) * (successorsLeft + successorsRight));
	path->a = (y2 - y1) / (float)(x2 - x1);
	path->b = y1 - path->a * x1;
	path->start = x1;
	path->stop = x2;
	path->bench = false;
	path->zIndex = zIndex;
	path->successorsLeft = successorsLeft;
	path->successorsRight = successorsRight;
	return path;
}

static struct Animal* SpawnAnimal(struct Game* game, struct GamestateResources* data, int tick, struct Path* path, struct AnimalRes* type, bool reverse, double speed) {
	struct Animal* animal = &data->animals[data->animalsCount]; //malloc(sizeof(struct Animal));
	animal->time.spawn = tick;
	double x1 = path->start;
	double x2 = path->stop;
	double y1 = path->a * x1 + path->b;
	double y2 = path->a * x2 + path->b;
	double length = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
	animal->speed = speed;
	animal->time.despawn = tick + (length / (38.8) * animal->speed);
	if (path->bench) {
		animal->time.despawn++;
	}
	animal->state = ANIMAL_WALKING;
	animal->type = type;
	animal->path = path;
	animal->reverse = reverse;
	animal->id = data->curId++;

	data->animalsCount++;
	if (data->animalsCount == data->animalsAllocated) {
		data->animalsAllocated *= 2;
		PrintConsole(game, "Reallocating animals table to %d", data->animalsAllocated);
		data->animals = realloc(data->animals, data->animalsAllocated * sizeof(struct Animal));
		animal = &data->animals[data->animalsCount - 1];
	}

	return animal;
}

static void CreatePaths(struct Game* game, struct GamestateResources* data) {
	data->paths[0] = InitPath(1545, 299, 1643, 392, 1, 0, -1);
	data->paths[1] = InitPath(28, 561, 686, 487, 0, 3, 1);
	data->paths[2] = InitPath(686, 487, 1194, 493, 3, 2, 1);
	data->paths[3] = InitPath(1194, 493, 1195, 493, 2, 2, 0);
	data->paths[4] = InitPath(1195, 493, 1414, 494, 2, 3, 1);
	data->paths[5] = InitPath(1414, 494, 1900, 432, 3, 0, 1);
	data->paths[6] = InitPath(393, 1027, 615, 738, 0, 1, 4);
	data->paths[7] = InitPath(615, 738, 840, 622, 1, 2, 4);
	data->paths[8] = InitPath(1185, 807, 1603, 999, 3, 0, 3);
	data->paths[9] = InitPath(840, 622, 1185, 807, 2, 3, 3);
	data->paths[10] = InitPath(686, 487, 840, 622, 3, 2, 3);
	data->paths[11] = InitPath(329, 225, 686, 487, 1, 3, 0);
	data->paths[12] = InitPath(260, 312, 329, 225, 0, 1, -1);
	data->paths[13] = InitPath(951, 1025, 1185, 807, 0, 3, 4);
	data->paths[14] = InitPath(1185, 807, 1414, 494, 3, 3, 2);
	data->paths[15] = InitPath(1414, 494, 1545, 299, 3, 1, 0);

	data->paths[3]->bench = true;

	// path 0 left
	data->paths[0]->successors[0] = data->paths[15];
	// no right on path 0

	// no left on path 1
	// path 1 right
	data->paths[1]->successors[0] = data->paths[11];
	data->paths[1]->successors[1] = data->paths[2];
	data->paths[1]->successors[2] = data->paths[10];

	// path 2 left
	data->paths[2]->successors[0] = data->paths[1];
	data->paths[2]->successors[1] = data->paths[11];
	data->paths[2]->successors[2] = data->paths[10];
	// path 2 right
	data->paths[2]->successors[3] = data->paths[3];
	data->paths[2]->successors[4] = data->paths[4];

	// path 3 left
	data->paths[3]->successors[0] = data->paths[2];
	data->paths[3]->successors[1] = data->paths[4];
	// path 3 right
	data->paths[3]->successors[2] = data->paths[2];
	data->paths[3]->successors[3] = data->paths[4];

	// path 4 left
	data->paths[4]->successors[0] = data->paths[2];
	data->paths[4]->successors[1] = data->paths[3];
	// path 4 right
	data->paths[4]->successors[2] = data->paths[14];
	data->paths[4]->successors[3] = data->paths[15];
	data->paths[4]->successors[4] = data->paths[5];

	// path 5 left
	data->paths[5]->successors[0] = data->paths[4];
	data->paths[5]->successors[1] = data->paths[14];
	data->paths[5]->successors[2] = data->paths[15];
	// no right on path 5

	// no left on path 6
	// path 6 right
	data->paths[6]->successors[0] = data->paths[7];

	// path 7 left
	data->paths[7]->successors[0] = data->paths[6];
	// path 7 right
	data->paths[7]->successors[1] = data->paths[9];
	data->paths[7]->successors[2] = data->paths[10];

	// path 8 left
	data->paths[8]->successors[0] = data->paths[9];
	data->paths[8]->successors[1] = data->paths[13];
	data->paths[8]->successors[2] = data->paths[14];
	// no right on path 8

	// path 9 left
	data->paths[9]->successors[0] = data->paths[7];
	data->paths[9]->successors[1] = data->paths[10];
	// path 9 right
	data->paths[9]->successors[2] = data->paths[8];
	data->paths[9]->successors[3] = data->paths[13];
	data->paths[9]->successors[4] = data->paths[14];

	// path 10 left
	data->paths[10]->successors[0] = data->paths[1];
	data->paths[10]->successors[1] = data->paths[11];
	data->paths[10]->successors[2] = data->paths[2];
	// path 10 right
	data->paths[10]->successors[3] = data->paths[7];
	data->paths[10]->successors[4] = data->paths[9];

	// path 11 left
	data->paths[11]->successors[0] = data->paths[12];
	// path 11 right
	data->paths[11]->successors[1] = data->paths[1];
	data->paths[11]->successors[2] = data->paths[2];
	data->paths[11]->successors[3] = data->paths[10];

	// no left on path 12
	// path 12 right
	data->paths[12]->successors[0] = data->paths[11];

	// no left on path 13
	// path 13 right
	data->paths[13]->successors[0] = data->paths[9];
	data->paths[13]->successors[1] = data->paths[14];
	data->paths[13]->successors[2] = data->paths[8];

	// path 14 left
	data->paths[14]->successors[0] = data->paths[9];
	data->paths[14]->successors[1] = data->paths[13];
	data->paths[14]->successors[2] = data->paths[8];
	// path 14 right
	data->paths[14]->successors[3] = data->paths[4];
	data->paths[14]->successors[4] = data->paths[15];
	data->paths[14]->successors[5] = data->paths[5];

	// path 15 left
	data->paths[15]->successors[0] = data->paths[4];
	data->paths[15]->successors[1] = data->paths[14];
	data->paths[15]->successors[2] = data->paths[5];
	// path 15 right
	data->paths[15]->successors[3] = data->paths[0];
}

static void DGZ(struct Game* game, struct GamestateResources* data) { // Dynamiczny Generator Zwierzątek™

	struct Path* entrances[] = {
		data->paths[1],
		data->paths[12],
		data->paths[6],
		data->paths[13],
		data->paths[8],
		data->paths[0],
		data->paths[5],
	};

	struct AnimalRes* animalTypes[] = {&data->owca, &data->ostronos, &data->dzik};

	data->animalsAllocated = 512;
	data->animalsCount = 0;
	data->curId = 0;
	data->animals = calloc(data->animalsAllocated, sizeof(struct Animal));

	int tick = 0;
	bool animalsLeft = false;
	while ((tick < TICKS_PER_DAY) || (animalsLeft)) {
		double night = NightValue(tick / (double)TICKS_PER_DAY);
		double probability = (1 / (float)AVG_TICKS_PER_ANIMAL) * pow(1 - AT_NIGHT_SUPPRESSION * night, 2);
		animalsLeft = false;

		if (((rand() / (float)RAND_MAX) <= probability) && (tick < TICKS_PER_DAY)) {
			// spawn an animal on entrance
			struct Path* path = entrances[rand() % (sizeof entrances / sizeof entrances[0])];
			SpawnAnimal(game, data, tick, path, animalTypes[rand() % (sizeof animalTypes / sizeof animalTypes[0])], path->successorsLeft ? true : false, 0.8 + (rand() / (float)RAND_MAX) * 0.4);
		}

		bool benchLeftTaken = false;
		bool benchRightTaken = false;

		for (int retick = 0; retick < 2; retick++) {
			if (retick) {
				tick += TICKS_PER_DAY; // wrapping
			}
			for (unsigned int i = 0; i < data->animalsCount; i++) {
				struct Animal* animal = &data->animals[i];
				if ((animal->time.spawn <= tick) && (animal->time.despawn >= tick)) {
					animalsLeft = true;
					switch (animal->state) {
						case ANIMAL_BENCH_CENTER:
							benchLeftTaken = true;
							benchRightTaken = true;
							break;
						case ANIMAL_BENCH_LEFT:
							benchLeftTaken = true;
							break;
						case ANIMAL_BENCH_RIGHT:
							benchRightTaken = true;
							break;
						case ANIMAL_WALKING:
							break;
					}
				}
			}
			if (retick) {
				tick -= TICKS_PER_DAY; // wrapping
			}
		}

		for (unsigned int i = 0; i < data->animalsCount; i++) {
			struct Animal* animal = &data->animals[i];
			if ((animal->time.spawn <= tick) && (animal->time.despawn >= tick)) {
				if (animal->state != ANIMAL_WALKING) {
					animal->time.despawn = tick + 1;
					if ((animal->time.spawn + MIN_BENCHING_TIME) <= tick) {
						if ((rand() / (double)RAND_MAX) <= (1 / (double)AVG_BENCHING_TIME)) {
							animal->time.despawn = tick;
						}
					}
				}
			}
			if (animal->time.despawn == tick) {
				int pathsNr = animal->reverse ? animal->path->successorsLeft : animal->path->successorsRight;
				int startNr = animal->reverse ? 0 : animal->path->successorsLeft;
				int id = animal->id;
				if (pathsNr) {
					struct Path* newpath = animal->path->successors[startNr + (rand() % pathsNr)];
					enum ANIMAL_STATE newstate = ANIMAL_WALKING;

					if (newpath->bench) {
						// it's a bench!
						if (benchLeftTaken && benchRightTaken) {
							// both sits are taken though, no luck :(
							while (newpath->bench) {
								newpath = animal->path->successors[startNr + (rand() % pathsNr)];
							}
						} else {
							// the bench has free sit!
							bool desiredLeft = rand() / (double)RAND_MAX < 0.5;
							if (desiredLeft && benchLeftTaken) {
								desiredLeft = false;
							}
							if (!desiredLeft && benchRightTaken) {
								desiredLeft = true;
							}
							newstate = desiredLeft ? ANIMAL_BENCH_LEFT : ANIMAL_BENCH_RIGHT;
							if (!benchLeftTaken && !benchRightTaken) {
								if (rand() / (double)RAND_MAX < 0.1) {
									// let's take the whole bench for myself, hah!
									newstate = ANIMAL_BENCH_CENTER;
									benchLeftTaken = true;
									benchRightTaken = true;
								}
							}
							if (desiredLeft) {
								benchLeftTaken = true;
							} else {
								benchRightTaken = true;
							}
						}
					}

					// check if we're arriving at beginning or ending of newpath
					bool left = false;
					for (unsigned int j = 0; j < newpath->successorsLeft; j++) {
						// either we find the old path as left successor to the new one (so we arrive at it's beginning)
						// or not (so we arrive at its end)
						if (newpath->successors[j] == animal->path) {
							left = true;
							break;
						}
					}
					struct Animal* newanimal = SpawnAnimal(game, data, tick, newpath, data->animals[i].type, !left, data->animals[i].speed);
					newanimal->state = newstate;
					newanimal->id = id;
				}
			}
		}
		tick++;
	}

	data->animalsAllocated = data->animalsCount;
	data->animals = realloc(data->animals, data->animalsAllocated * sizeof(struct Animal));

	PrintConsole(game, "DGZ: generated %d animals", data->animalsCount);
}

static void GuardedDraw(ALLEGRO_BITMAP* bitmap, float cx, float cy, float dx, float dy, float angle, int flags) {
	if ((dx > 1920 * -0.5) || (dx < 1920 * 1.5) || (dy > 1080 * -0.5) || (dy < 1080 * 1.5)) {
		al_draw_rotated_bitmap(bitmap, cx, cy, dx, dy, angle, flags);
	}
}

static int zIndexCmp(const void* a, const void* b) {
	int ret = ((struct DrawCommand const*)a)->animal->path->zIndex - ((struct DrawCommand const*)b)->animal->path->zIndex;
	if (ret == 0) {
		ret = ((struct DrawCommand const*)a)->animal->type->zIndex - ((struct DrawCommand const*)b)->animal->type->zIndex;
	}
	if (ret == 0) {
		ret = ((struct DrawCommand const*)a)->animal->id - ((struct DrawCommand const*)b)->animal->id;
	}
	return ret;
}

static void DrawScene(struct Game* game, struct GamestateResources* data, double time) {
	al_set_target_bitmap(data->scene);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	al_draw_bitmap(data->bg, 0, 0, 0);

	double night = NightValue(time);
	al_draw_tinted_bitmap(data->bg2, al_map_rgba_f(night, night, night, night), 0, 0, 0);

	int tick = time * TICKS_PER_DAY;

	int bufPos = 0;

	//	PrintConsole(game, "clock is ticking: %f = %d; animals %d", time, tick, data->animalsCount);
	for (unsigned int i = 0; i < data->animalsCount; i++) {
		//		PrintConsole(game, "animal %d exists from tick %d to %d", i, data->animals[i]->time.spawn, data->animals[i]->time.despawn);
		for (int retick = 0; retick < 2; retick++) {
			if (retick) {
				time += 1;
				tick += TICKS_PER_DAY; // wrapping
			}
			if ((data->animals[i].time.spawn <= tick) && (data->animals[i].time.despawn > tick)) {
				//PrintConsole(game, "got it");
				double progress = ((time * TICKS_PER_DAY) - data->animals[i].time.spawn) / (double)(data->animals[i].time.despawn - data->animals[i].time.spawn);
				struct Path* path = data->animals[i].path;
				double x;
				if (!data->animals[i].reverse) {
					x = path->start + (path->stop - path->start) * progress;
				} else {
					x = path->stop - (path->stop - path->start) * progress;
				}
				double y = path->a * x + path->b;

				if (data->animals[i].state == ANIMAL_WALKING) {
					struct DrawCommand cmd = {&data->animals[i], data->animals[i].type->bitmap,
						al_get_bitmap_width(data->animals[i].type->bitmap) / 2, al_get_bitmap_height(data->animals[i].type->bitmap) * 0.75,
						x, y, atan(path->a) + (sin(time * 6000 + i) / 5.0), data->animals[i].reverse ? ALLEGRO_FLIP_HORIZONTAL : 0};
					memcpy(&data->drawCommandBuffer[bufPos], &cmd, sizeof(cmd));
					bufPos++;
				} else {
					int offset = 85;
					if (data->animals[i].state == ANIMAL_BENCH_CENTER) {
						offset = 40;
					} else if (data->animals[i].state == ANIMAL_BENCH_RIGHT) {
						offset = -5;
					}
					struct DrawCommand cmd = {&data->animals[i], data->animals[i].type->bitmap_sitting, 0, 0,
						x - offset, data->animals[i].type->benchPos, 0, (data->animals[i].state == ANIMAL_BENCH_RIGHT) ? ALLEGRO_FLIP_HORIZONTAL : 0};
					memcpy(&data->drawCommandBuffer[bufPos], &cmd, sizeof(cmd));
					bufPos++;
				}
			}
			if (retick) {
				time -= 1;
				tick -= TICKS_PER_DAY; // wrapping
			}
		}
	}

	qsort(data->drawCommandBuffer, bufPos, sizeof(struct DrawCommand), zIndexCmp);
	bool negative = true;
	for (int i = 0; i < bufPos; i++) {
		if (data->drawCommandBuffer[i].animal->path->zIndex >= 0) {
			if (negative) {
				al_draw_bitmap(data->fg, 0, 0, 0);
				al_draw_tinted_bitmap(data->fg2, al_map_rgba_f(night, night, night, night), 0, 0, 0);
			}
			negative = false;
		}
		al_draw_rotated_bitmap(data->drawCommandBuffer[i].bitmap, data->drawCommandBuffer[i].cx, data->drawCommandBuffer[i].cy, data->drawCommandBuffer[i].x, data->drawCommandBuffer[i].y, data->drawCommandBuffer[i].angle, data->drawCommandBuffer[i].flags);
		//al_draw_textf(data->small, al_map_rgb(255, 255, 255), data->drawCommandBuffer[i].x, data->drawCommandBuffer[i].y, ALLEGRO_ALIGN_CENTER, "%d", data->drawCommandBuffer[i].animal->path->zIndex);
	}
	if (negative) {
		al_draw_bitmap(data->fg, 0, 0, 0);
		al_draw_tinted_bitmap(data->fg2, al_map_rgba_f(night, night, night, night), 0, 0, 0);
	}

	al_draw_bitmap(data->trees, 0, 0, 0);

	/*
	for (unsigned int i = 0; i < (sizeof(data->paths) / sizeof(struct Path*)); i++) {
		al_draw_line(data->paths[i]->start, data->paths[i]->a * data->paths[i]->start + data->paths[i]->b,
			data->paths[i]->stop, data->paths[i]->a * data->paths[i]->stop + data->paths[i]->b,
			al_map_rgb(i * 15, 0, 255), 10);
		al_draw_textf(data->small, al_map_rgb(255, 255, 255), (data->paths[i]->start + data->paths[i]->stop) / 2,
			data->paths[i]->a * (data->paths[i]->start + data->paths[i]->stop) / 2 + data->paths[i]->b - 20,
			ALLEGRO_ALIGN_CENTER, "%d", i);
	}
	*/

	ALLEGRO_BITMAP* beeframes[4] = {data->bee1, data->bee2, data->bee3, data->bee2};
	ALLEGRO_BITMAP* bee = beeframes[(int)(time * 20000) % 4];

	GuardedDraw(bee, al_get_bitmap_width(data->bee1) / 2, al_get_bitmap_height(data->bee1) / 2,
	  (time - 0.2) * 200 * 1920, 650, sin(time * 12000) / 6.0, 0);

	GuardedDraw(bee, al_get_bitmap_width(data->bee1) / 2, al_get_bitmap_height(data->bee1) / 2,
	  (time - 0.7) * 200 * 1920, 250, sin(time * 12000) / 6.0, 0);

	GuardedDraw(bee, al_get_bitmap_width(data->bee1) / 2, al_get_bitmap_height(data->bee1) / 2,
	  (time - 0.9) * 200 * 1920, 750, sin(time * 12000) / 6.0, 0);

	GuardedDraw(data->leaf.bitmap, al_get_bitmap_width(data->leaf.bitmap) / 2, al_get_bitmap_height(data->leaf.bitmap) / 2,
	  (0.3 - time) * 50 * 1920, (0.3 - time) * 50 * 1080, time * 800, ALLEGRO_FLIP_VERTICAL);

	GuardedDraw(data->leaf.bitmap, al_get_bitmap_width(data->leaf.bitmap) / 2, al_get_bitmap_height(data->leaf.bitmap) / 2,
	  (time - 0.8) * 120 * 1920 + 1920 / 2, (time - 0.8) * -80 * 1080 + 1080 / 2, time * 1000, ALLEGRO_FLIP_HORIZONTAL);

	GuardedDraw(data->leaf.bitmap, al_get_bitmap_width(data->leaf.bitmap) / 2, al_get_bitmap_height(data->leaf.bitmap) / 2,
	  (time - 0.4) * 70 * 1920, (time - 0.4) * 80 * 1080, time * 1200, 0);

	al_draw_textf(game->_priv.font_console, al_map_rgb(0, 0, 0), 10, 1030, ALLEGRO_ALIGN_LEFT, "%f (%d)", time, tick);
	al_draw_textf(game->_priv.font_console, al_map_rgb(0, 0, 0), 1910, 1030, ALLEGRO_ALIGN_RIGHT, "(%d) %f", tick, time);

	al_draw_filled_rectangle(0, 0, 1920, 1080, al_map_rgba_f(0, 0, 0, night * 0.333));
}

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	// Called as soon as possible, but no sooner than next Gamestate_Logic call.
	// Draw everything to the screen here.

	DrawScene(game, data, data->time_left);

	al_set_target_bitmap(data->target);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
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
	al_set_shader_float("TIME", data->counter / 60.0); //data->blink_counter/3600.0);
	float size[2] = {al_get_bitmap_width(data->target), al_get_bitmap_height(data->target)};
	al_set_shader_float_vector("RENDERSIZE", 2, size, 1);
	float colorl[4] = {0.8, 0.0, 0.4, 1.0};
	al_set_shader_float_vector("colorBleedL", 4, colorl, 1);
	float colorc[4] = {0.0, 0.5, 0.9, 1.0};
	al_set_shader_float_vector("colorBleedC", 4, colorc, 1);
	float colorr[4] = {0.8, 0.0, 0.4, 1.0};
	al_set_shader_float_vector("colorBleedR", 4, colorr, 1);
	al_draw_scaled_bitmap(data->scene, 0, 0, 1920, 1080, 0, 0, 1920 / 2, 1080 / 2, 0);
	al_use_shader(NULL);

	al_set_target_backbuffer(game->display);

	al_draw_tinted_scaled_rotated_bitmap_region(data->target, 0, 0, 1920 / 4, 1080 / 2, al_map_rgb_f(1, 1, 1), 0, 0, (data->shakeleft ? ((rand() % 20) - 10) : 0), (data->shakeleft ? ((rand() % 20) - 10) : 0), 2, 2, 0, 0);

	// right

	DrawScene(game, data, data->time_right);

	al_set_target_bitmap(data->target);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
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
	al_set_shader_float("TIME", data->counter / 60.0 + 100); //data->blink_counter/3600.0);
	al_set_shader_float_vector("RENDERSIZE", 2, size, 1);
	al_set_shader_float_vector("colorBleedL", 4, colorl, 1);
	al_set_shader_float_vector("colorBleedC", 4, colorc, 1);
	al_set_shader_float_vector("colorBleedR", 4, colorr, 1);
	al_draw_scaled_bitmap(data->scene, 0, 0, 1920, 1080, 0, 0, 1920 / 2, 1080 / 2, 0);
	al_use_shader(NULL);

	al_set_target_backbuffer(game->display);

	al_draw_tinted_scaled_rotated_bitmap_region(data->target, 1920 / 4, 0, 1920 / 4, 1080 / 2, al_map_rgb_f(1, 1, 1), 0, 0, 1920 / 2 + (data->shakeright ? ((rand() % 20) - 10) : 0), (data->shakeright ? ((rand() % 20) - 10) : 0), 2, 2, 0, 0);

	//al_draw_scaled_bitmap(data->target, 0, 0, 1920 / 2, 1080 / 2, 0, 0, 1920, 1080, 0); // debug

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

	al_draw_textf(data->small, al_map_rgb(0, 0, 0), 520, 990, ALLEGRO_ALIGN_LEFT, "Now: %d", data->leftscore);
	al_draw_textf(data->small, al_map_rgb(0, 0, 0), 1122, 987, ALLEGRO_ALIGN_LEFT, "Then: %d", data->rightscore);

	al_draw_scaled_rotated_bitmap(data->ball, al_get_bitmap_width(data->ball) / 2, al_get_bitmap_height(data->ball) / 2,
	  data->ballx, data->bally, 0.75, 0.75, data->ballrot, 0);

	/*
	al_draw_line(223, 875, 223 + 115 * cos(data->time_left * 4 * ALLEGRO_PI), 875 + 115 * sin(data->time_left * 4 * ALLEGRO_PI), al_map_rgb(255,255,0), 5);
	al_draw_line(223, 875, 223 + 138 * cos(data->time_left * 4 * ALLEGRO_PI * 24), 875 + 138 * sin(data->time_left * 4 * ALLEGRO_PI * 24), al_map_rgb(255,255,0), 5);

	al_draw_line(1672, 879, 1672 + 115 * cos(data->time_right * 4 * ALLEGRO_PI), 879 + 115 * sin(data->time_right * 4 * ALLEGRO_PI), al_map_rgb(255,255,0), 5);
	al_draw_line(1672, 879, 1672 + 138 * cos(data->time_right * 4 * ALLEGRO_PI * 24), 879 + 138 * sin(data->time_right * 4 * ALLEGRO_PI * 24), al_map_rgb(255,255,0), 5);

	al_draw_circle(data->ballx, data->bally, 42, al_map_rgb(255,0,0), 5);
*/

	//	PrintConsole(game, "%f %f", x1, x2);

	if (data->delay != -1) {
		float tint = sin(data->delay / 120.0 * ALLEGRO_PI);
		if (data->delay > 60) {
			tint = sqrt(tint);
		}
		float scale = sqrt(cos(data->delay / 120.0 * ALLEGRO_PI / 2));
		al_draw_tinted_scaled_rotated_bitmap(data->scorebmp, al_map_rgba_f(tint, tint, tint, tint), 1920 / 2, 1080 / 2, 1920 / 2, 1080 / 2 - 160 + 40 * scale, 0.75 + scale / 2.0, 0.75 + scale / 2.0, 0, 0);
	}

	if (!data->started) {
		char* text;
		if (data->leftscore == 10) {
			text = "Now wins!";
		} else if (data->rightscore == 10) {
			text = "Then wins!";
		} else {
			text = "Now and Then";
		}
		al_draw_text(data->big, al_map_rgb(0, 0, 0), 1920 / 2 - 6, 160 - 6, ALLEGRO_ALIGN_CENTER, text);
		al_draw_text(data->big, al_map_rgb(0, 0, 0), 1920 / 2 + 6, 160 + 6, ALLEGRO_ALIGN_CENTER, text);
		al_draw_text(data->big, al_map_rgb(0, 0, 0), 1920 / 2 - 6, 160 + 6, ALLEGRO_ALIGN_CENTER, text);
		al_draw_text(data->big, al_map_rgb(0, 0, 0), 1920 / 2 + 6, 160 - 6, ALLEGRO_ALIGN_CENTER, text);
		al_draw_text(data->big, al_map_rgb(0, 0, 0), 1920 / 2 + 0, 160 + 6, ALLEGRO_ALIGN_CENTER, text);
		al_draw_text(data->big, al_map_rgb(0, 0, 0), 1920 / 2 + 0, 160 - 6, ALLEGRO_ALIGN_CENTER, text);
		al_draw_text(data->big, al_map_rgb(0, 0, 0), 1920 / 2 + 6, 160 + 0, ALLEGRO_ALIGN_CENTER, text);
		al_draw_text(data->big, al_map_rgb(0, 0, 0), 1920 / 2 - 6, 160 + 0, ALLEGRO_ALIGN_CENTER, text);
		al_draw_text(data->big, al_map_rgb(255, 255, 255), 1920 / 2, 160, ALLEGRO_ALIGN_CENTER, text);

		al_draw_text(data->small, al_map_rgb(0, 0, 0), 1920 / 2 - 4, 860 - 4, ALLEGRO_ALIGN_CENTER, "Press SPACE...");
		al_draw_text(data->small, al_map_rgb(0, 0, 0), 1920 / 2 + 4, 860 + 4, ALLEGRO_ALIGN_CENTER, "Press SPACE...");
		al_draw_text(data->small, al_map_rgb(0, 0, 0), 1920 / 2 + 4, 860 - 4, ALLEGRO_ALIGN_CENTER, "Press SPACE...");
		al_draw_text(data->small, al_map_rgb(0, 0, 0), 1920 / 2 - 4, 860 + 4, ALLEGRO_ALIGN_CENTER, "Press SPACE...");
		al_draw_text(data->small, al_map_rgb(0, 0, 0), 1920 / 2 - 4, 860 - 0, ALLEGRO_ALIGN_CENTER, "Press SPACE...");
		al_draw_text(data->small, al_map_rgb(0, 0, 0), 1920 / 2 + 4, 860 - 0, ALLEGRO_ALIGN_CENTER, "Press SPACE...");
		al_draw_text(data->small, al_map_rgb(0, 0, 0), 1920 / 2 - 0, 860 - 4, ALLEGRO_ALIGN_CENTER, "Press SPACE...");
		al_draw_text(data->small, al_map_rgb(0, 0, 0), 1920 / 2 - 0, 860 + 4, ALLEGRO_ALIGN_CENTER, "Press SPACE...");
		al_draw_text(data->small, al_map_rgb(255, 255, 255), 1920 / 2, 860, ALLEGRO_ALIGN_CENTER, "Press SPACE...");
	}
}

void Gamestate_ProcessEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev) {
	// Called for each event in Allegro event queue.
	// Here you can handle user input, expiring timers etc.
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		UnloadCurrentGamestate(game); // mark this gamestate to be stopped and unloaded
		// When there are no active gamestates, the engine will quit.
	}

	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_RIGHT)) {
		data->forward_right = true;
		data->lastbackward_right = false;
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_UP) && (ev->keyboard.keycode == ALLEGRO_KEY_RIGHT)) {
		data->forward_right = false;
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_LEFT)) {
		data->backward_right = true;
		data->lastbackward_right = true;
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_UP) && (ev->keyboard.keycode == ALLEGRO_KEY_LEFT)) {
		data->backward_right = false;
	}

	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_D)) {
		data->forward_left = true;
		data->lastbackward_left = false;
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_UP) && (ev->keyboard.keycode == ALLEGRO_KEY_D)) {
		data->forward_left = false;
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_A)) {
		data->backward_left = true;
		data->lastbackward_left = true;
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_UP) && (ev->keyboard.keycode == ALLEGRO_KEY_A)) {
		data->backward_left = false;
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_UP) && (ev->keyboard.keycode == ALLEGRO_KEY_SPACE)) {
		if (data->started) {
			return;
		}
		al_rewind_audio_stream(data->music);
		al_set_audio_stream_playing(data->music, true);

		data->dx = 12 * (((rand() % 2) * 2) - 1);
		data->dy = 4;
		data->ballx = 1920 / 2 - 10;
		data->bally = 1080 / 2 - 100;
		data->started = true;
		data->leftscore = 0;
		data->rightscore = 0;
		data->scoreleft = (data->dx < 0);
		data->lastleft = data->scoreleft;
	}
}

void* Gamestate_Load(struct Game* game, void (*progress)(struct Game*)) {
	// Called once, when the gamestate library is being loaded.
	// Good place for allocating memory, loading bitmaps etc.
	struct GamestateResources* data = calloc(1, sizeof(struct GamestateResources));

	data->small = al_load_font(GetDataFilePath(game, "fonts/belligerent.ttf"), 53, 0);
	progress(game); // report that we progressed with the loading, so the engine can draw a progress bar
	data->big = al_load_font(GetDataFilePath(game, "fonts/belligerent.ttf"), 200, 0);
	progress(game);
	data->scorefont = al_load_font(GetDataFilePath(game, "fonts/belligerent.ttf"), 400, 0);
	progress(game);

	data->yay1s = al_load_sample(GetDataFilePath(game, "sounds/yay1.flac"));
	data->yay1 = al_create_sample_instance(data->yay1s);
	al_attach_sample_instance_to_mixer(data->yay1, game->audio.fx);
	al_set_sample_instance_gain(data->yay1, 2.2);
	al_set_sample_instance_playmode(data->yay1, ALLEGRO_PLAYMODE_ONCE);
	progress(game);

	data->yay2s = al_load_sample(GetDataFilePath(game, "sounds/yay2.flac"));
	data->yay2 = al_create_sample_instance(data->yay2s);
	al_attach_sample_instance_to_mixer(data->yay2, game->audio.fx);
	al_set_sample_instance_gain(data->yay2, 2.2);
	al_set_sample_instance_playmode(data->yay2, ALLEGRO_PLAYMODE_ONCE);
	progress(game);

	data->yay3s = al_load_sample(GetDataFilePath(game, "sounds/yay3.flac"));
	data->yay3 = al_create_sample_instance(data->yay3s);
	al_attach_sample_instance_to_mixer(data->yay3, game->audio.fx);
	al_set_sample_instance_gain(data->yay3, 2.2);
	al_set_sample_instance_playmode(data->yay3, ALLEGRO_PLAYMODE_ONCE);
	progress(game);

	data->balls = al_load_sample(GetDataFilePath(game, "sounds/ball.flac"));
	data->ballsound = al_create_sample_instance(data->balls);
	al_attach_sample_instance_to_mixer(data->ballsound, game->audio.fx);
	al_set_sample_instance_gain(data->ballsound, 2.2);
	al_set_sample_instance_playmode(data->ballsound, ALLEGRO_PLAYMODE_ONCE);
	progress(game);

	data->bg = al_load_bitmap(GetDataFilePath(game, "bg.png"));
	progress(game);
	data->bg2 = al_load_bitmap(GetDataFilePath(game, "bg2.png"));
	progress(game);
	data->fg = al_load_bitmap(GetDataFilePath(game, "fg.png"));
	progress(game);
	data->fg2 = al_load_bitmap(GetDataFilePath(game, "fg2.png"));
	progress(game);
	data->frame = al_load_bitmap(GetDataFilePath(game, "frame.png"));
	progress(game);
	data->trees = al_load_bitmap(GetDataFilePath(game, "trees.png"));
	progress(game);
	data->target = CreateNotPreservedBitmap(1920 / 2, 1080 / 2);
	data->scene = CreateNotPreservedBitmap(1920, 1080);
	data->scorebmp = CreateNotPreservedBitmap(1920, 1080);

	data->leaf.bitmap = al_load_bitmap(GetDataFilePath(game, "leaf.png"));
	progress(game);

	data->ostronos.bitmap = al_load_bitmap(GetDataFilePath(game, "animals/ostronos.png"));
	progress(game);
	data->owca.bitmap = al_load_bitmap(GetDataFilePath(game, "animals/owca.png"));
	progress(game);
	data->dzik.bitmap = al_load_bitmap(GetDataFilePath(game, "animals/dzik.png"));
	progress(game);
	data->ostronos.bitmap_sitting = al_load_bitmap(GetDataFilePath(game, "animals/ostronos1.png"));
	progress(game);
	data->owca.bitmap_sitting = al_load_bitmap(GetDataFilePath(game, "animals/owca1.png"));
	progress(game);
	data->dzik.bitmap_sitting = al_load_bitmap(GetDataFilePath(game, "animals/dzik1.png"));
	progress(game);
	data->dzik.benchPos = 310;
	data->dzik.zIndex = 1;
	data->owca.benchPos = 310;
	data->owca.zIndex = 0;
	data->ostronos.benchPos = 350;
	data->ostronos.zIndex = 2;

	data->bee1 = al_load_bitmap(GetDataFilePath(game, "animals/pszczolka1.png"));
	progress(game);
	data->bee2 = al_load_bitmap(GetDataFilePath(game, "animals/pszczolka2.png"));
	progress(game);
	data->bee3 = al_load_bitmap(GetDataFilePath(game, "animals/pszczolka3.png"));
	progress(game);

	data->rewind = al_load_audio_stream(GetDataFilePath(game, "sounds/rewind.ogg"), 8, 1024);
	al_set_audio_stream_gain(data->rewind, 0);
	al_set_audio_stream_playmode(data->rewind, ALLEGRO_PLAYMODE_LOOP);
	al_attach_audio_stream_to_mixer(data->rewind, game->audio.fx);
	progress(game);

	data->music = al_load_audio_stream(GetDataFilePath(game, "sounds/music.flac"), 8, 1024);
	al_set_audio_stream_playmode(data->music, ALLEGRO_PLAYMODE_LOOP);
	al_set_audio_stream_playing(data->music, false);
	al_attach_audio_stream_to_mixer(data->music, game->audio.music);
	progress(game);

	data->day1 = al_load_audio_stream(GetDataFilePath(game, "sounds/day1.ogg"), 8, 1024);
	al_set_audio_stream_gain(data->day1, 0);
	al_attach_audio_stream_to_mixer(data->day1, game->audio.fx);
	al_set_audio_stream_pan(data->day1, -0.5);
	al_set_audio_stream_playmode(data->day1, ALLEGRO_PLAYMODE_LOOP);
	progress(game);

	data->day2 = al_load_audio_stream(GetDataFilePath(game, "sounds/day2.ogg"), 8, 1024);
	al_set_audio_stream_gain(data->day2, 0);
	al_attach_audio_stream_to_mixer(data->day2, game->audio.fx);
	al_set_audio_stream_pan(data->day2, 0.5);
	al_set_audio_stream_playmode(data->day2, ALLEGRO_PLAYMODE_LOOP);
	progress(game);

	data->night1 = al_load_audio_stream(GetDataFilePath(game, "sounds/night1.ogg"), 8, 1024);
	al_set_audio_stream_gain(data->night1, 0);
	al_attach_audio_stream_to_mixer(data->night1, game->audio.fx);
	al_set_audio_stream_pan(data->night1, -0.5);
	al_set_audio_stream_playmode(data->night1, ALLEGRO_PLAYMODE_LOOP);
	progress(game);

	data->night2 = al_load_audio_stream(GetDataFilePath(game, "sounds/night2.ogg"), 8, 1024);
	al_set_audio_stream_gain(data->night2, 0);
	al_attach_audio_stream_to_mixer(data->night2, game->audio.fx);
	al_set_audio_stream_pan(data->night2, 0.5);
	al_set_audio_stream_playmode(data->night2, ALLEGRO_PLAYMODE_LOOP);
	progress(game);

	data->ball = al_load_bitmap(GetDataFilePath(game, "ball.png"));
	progress(game);
	data->clock1 = al_load_bitmap(GetDataFilePath(game, "clock1.png"));
	progress(game);
	data->clock2 = al_load_bitmap(GetDataFilePath(game, "clock2.png"));
	progress(game);
	data->clockball1 = al_load_bitmap(GetDataFilePath(game, "clockball1.png"));
	progress(game);
	data->clockball2 = al_load_bitmap(GetDataFilePath(game, "clockball2.png"));
	progress(game);
	data->hand1 = al_load_bitmap(GetDataFilePath(game, "hand1.png"));
	progress(game);
	data->hand2 = al_load_bitmap(GetDataFilePath(game, "hand2.png"));
	progress(game);
	data->scores = al_load_bitmap(GetDataFilePath(game, "scores.png"));
	progress(game);

	CreatePaths(game, data);
	DGZ(game, data);
	progress(game);

	data->drawCommandBuffer = calloc(data->animalsCount, sizeof(struct DrawCommand));

	return data;
}

void Gamestate_Unload(struct Game* game, struct GamestateResources* data) {
	// Called when the gamestate library is being unloaded.
	// Good place for freeing all allocated memory and resources.

	al_destroy_font(data->big);
	al_destroy_font(data->small);
	al_destroy_font(data->scorefont);

	al_destroy_bitmap(data->bg);
	al_destroy_bitmap(data->bg2);
	al_destroy_bitmap(data->frame);
	al_destroy_bitmap(data->target);
	al_destroy_bitmap(data->scene);
	al_destroy_bitmap(data->bee1);
	al_destroy_bitmap(data->bee2);
	al_destroy_bitmap(data->bee3);
	al_destroy_bitmap(data->clock1);
	al_destroy_bitmap(data->clock2);
	al_destroy_bitmap(data->clockball1);
	al_destroy_bitmap(data->clockball2);
	al_destroy_bitmap(data->hand1);
	al_destroy_bitmap(data->hand2);
	al_destroy_bitmap(data->ball);
	al_destroy_bitmap(data->trees);
	al_destroy_bitmap(data->scores);
	al_destroy_bitmap(data->scorebmp);
	al_destroy_bitmap(data->dzik.bitmap);
	al_destroy_bitmap(data->ostronos.bitmap);
	al_destroy_bitmap(data->owca.bitmap);
	al_destroy_bitmap(data->leaf.bitmap);
	al_destroy_bitmap(data->dzik.bitmap_sitting);
	al_destroy_bitmap(data->ostronos.bitmap_sitting);
	al_destroy_bitmap(data->owca.bitmap_sitting);

	al_destroy_audio_stream(data->day1);
	al_destroy_audio_stream(data->day2);
	al_destroy_audio_stream(data->night1);
	al_destroy_audio_stream(data->night2);
	al_destroy_audio_stream(data->rewind);
	al_destroy_audio_stream(data->music);

	al_destroy_sample_instance(data->yay1);
	al_destroy_sample_instance(data->yay2);
	al_destroy_sample_instance(data->yay3);
	al_destroy_sample_instance(data->ballsound);
	al_destroy_sample(data->yay1s);
	al_destroy_sample(data->yay2s);
	al_destroy_sample(data->yay3s);
	al_destroy_sample(data->balls);

	free(data->animals);
	free(data->drawCommandBuffer);
	for (unsigned int i = 0; i < (sizeof(data->paths) / sizeof(struct Path*)); i++) {
		free(data->paths[i]);
	}

	free(data);
}

void Gamestate_Start(struct Game* game, struct GamestateResources* data) {
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
	data->ballrot = 0;

	data->started = false;
	data->dx = 0;
	data->dy = 0;
	data->ballx = 1920 / 2 - 10;
	data->bally = 1080 / 2 - 100;

	data->leftscore = 0;
	data->rightscore = 0;

	data->shakeleft = 0;
	data->shakeright = 0;

	data->score = 0;
	data->delay = -1;

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

	al_set_target_bitmap(data->scorebmp);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	al_set_target_backbuffer(game->display);
}

void Gamestate_Stop(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets stopped. Stop timers, music etc. here.
	al_destroy_shader(data->shader);
}

void Gamestate_Pause(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets paused (so only Draw is being called, no Logic not ProcessEvent)
	// Pause your timers here.
}

void Gamestate_Resume(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets resumed. Resume your timers here.
}

void Gamestate_Reload(struct Game* game, struct GamestateResources* data) {
	data->target = CreateNotPreservedBitmap(1920 / 2, 1080 / 2);
	data->scene = CreateNotPreservedBitmap(1920, 1080);
	data->scorebmp = CreateNotPreservedBitmap(1920, 1080);
	al_build_shader(data->shader);
}
