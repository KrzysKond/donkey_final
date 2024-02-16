#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include"class.h"
extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH	1200
#define SCREEN_HEIGHT	900
#define FPS 300


void startNewGame(Player* player, Barrel** barrels, int barrelCount, bool nPressed, int* barrelController) {
	*barrelController = 0;
	Player newPlayer;
	newPlayer.lives = player->lives;
	newPlayer.points = player->points;
	if (nPressed == true) {
		newPlayer.lives = 3;
		newPlayer.points = 0;
	}
	Barrel* newBarrels = new Barrel[barrelCount];
	*player = newPlayer;
	*barrels = newBarrels;
}


void saveScore(int points, char* nickname) {
	FILE* file = fopen("highscores.txt", "a");
	fprintf(file, "%s %d\n", nickname, points);
	fclose(file);
}


int touchingBeamIndex(SDL_Surface* character, SDL_Surface** beam, int beamCount) {
	for (int i = 0; i < beamCount; i++) {
		if (SDL_HasIntersection(&character->clip_rect, &beam[i]->clip_rect)) {
			return i;
		}
	}
	return -1;
}

bool hasFinishedLevel(Player* player, SDL_Surface* character, SDL_Surface* portal) {
	if (SDL_HasIntersection(&character->clip_rect, &portal->clip_rect)) {
		return true;
	}
}

bool onScreen(int x, int y) {
	if (x > 0 && x < SCREEN_WIDTH - 40 && y>0 && y < SCREEN_HEIGHT) {
		return true;
	}
	return false;
}

bool barrelOfScreen(Barrel* barrels, int barrelCount, SDL_Surface** barrelSurface) {
	Barrel barrel;
	SDL_Rect barrelRect = barrelSurface[0]->clip_rect;
	for (int i = 0; i < barrelCount; i++) {
		if (barrels[i].y > SCREEN_HEIGHT) {
			*(&barrels[i]) = barrel;
			*(&barrelSurface[i]->clip_rect) = { (int)barrel.x,(int)barrel.y, barrelRect.w, barrelRect.h - 5 };
			return true;
		}
	}
	return false;
}

bool barrelColision(SDL_Surface* character, SDL_Surface** barrels, int beamCount) {
	for (int i = 0; i < beamCount; i++) {
		if (SDL_HasIntersection(&character->clip_rect, &barrels[i]->clip_rect)) {
			return true;
		}
	}
	return false;
}

bool jumpedOverBarrel(SDL_Surface* character, SDL_Surface** barrels, int beamCount) {
	SDL_Rect charRect = character->clip_rect;
	charRect.y += 80;

	for (int i = 0; i < beamCount; i++) {
		if (SDL_HasIntersection(&charRect, &barrels[i]->clip_rect)) {
			return true;
		}
	}
	return false;
}


bool isLadderBelow(SDL_Surface* character, SDL_Surface** ladder, int ladderCount) {
	SDL_Rect charRect = character->clip_rect;
	charRect.y += 100;
	for (int i = 0; i < ladderCount; i++) {
		if (SDL_HasIntersection(&charRect, &ladder[i]->clip_rect)) {
			return true;
		}
	}
	return false;
}

SDL_Surface* surfaceFromTexture(SDL_Texture* texture, SDL_Renderer* renderer) {
	int width, height;
	SDL_QueryTexture(texture, NULL, NULL, &width, &height);

	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
	if (!surface) {
		// Handle surface creation error
		printf("Unable to create surface! SDL Error: %s\n", SDL_GetError());
		return NULL;
	}

	SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);

	SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGBA32, surface->pixels, surface->pitch);

	return surface;
}

bool checkCollisionFromLeft(SDL_Surface* character, SDL_Surface** beam, int beamCount, Player* player) {
	SDL_Rect charRect = character->clip_rect;

	for (int i = 0; i < beamCount; i++) {
		SDL_Rect beamRect = beam[i]->clip_rect;

		// Check for collision from the left between character and beam[i]
		if ((charRect.x + charRect.w == beamRect.x) &&
			(charRect.y + charRect.h > beamRect.y) &&
			(charRect.y < beamRect.y + beamRect.h)) {
			if (charRect.y - beamRect.y + beamRect.h > 3) {
				player->velocityX = -5;
				player->x -=5;
				return false;
			}
			return true;
		}
	}

	// No collision from the left detected
	return false;
}

bool checkCollisionFromRight(SDL_Surface* character, SDL_Surface** beam, int beamCount, Player* player) {
	SDL_Rect charRect = character->clip_rect;

	for (int i = 0; i < beamCount; i++) {
		SDL_Rect beamRect = beam[i]->clip_rect;

		// Check for collision from the right between character and beam[i]
		if ((charRect.x == beamRect.x) &&
			(charRect.y + charRect.h > beamRect.y) &&
			(charRect.y < beamRect.y + beamRect.h)) {
			// Collision from the right detected
			if (charRect.y - beamRect.y + beamRect.h > 3) {
				player->velocityX = 0;
				player->x -= 5;
				return false;
			}

			return true;
		}
	}

	// No collision from the right detected
	return false;
}

void checkColissions(SDL_Surface* character, SDL_Surface** beam, int beamCount, Player* player) {
	if (checkCollisionFromLeft(character, beam, beamCount, player) && player->velocityX != 0) {
		int beamIndex = touchingBeamIndex(character, beam, beamCount);
		if (beamIndex != -1) {
			int newY = beam[beamIndex]->clip_rect.y - character->clip_rect.h;
			player->y = newY;
		}
	}

	if (checkCollisionFromRight(character, beam, beamCount, player) == true && player->velocityX != 0) {
		int beamIndex = touchingBeamIndex(character, beam, beamCount);
		if (beamIndex != -1) {
			int newY = beam[beamIndex]->clip_rect.y - character->clip_rect.h;
			player->y = newY;
		}

	}
}

void whenMoving(double delta, Player* player, bool touchingBeam) {
	if (onScreen(player->x, player->y)) {
		if (player->velocityX > 0) {
			player->velocityX -= 2 * player->accelerationY * delta;
			if (player->velocityX < 0) player->velocityX = 0;
			player->x += player->velocityX * delta;
		}
		else if (player->velocityX < 0) {
			player->velocityX += 2 * player->accelerationY * delta;
			if (player->velocityX > 0) player->velocityX = 0;
			player->x += player->velocityX * delta;
		}
		if (player->velocityY < 0) {
			player->velocityY += 2 * player->accelerationY * delta;
			if (player->velocityY > 0) player->velocityY = 0;
			player->y += 2 * player->velocityY * delta;
		}
		else if (player->velocityY > 0) {
			if (touchingBeam == true) {
				player->isOnLadder = false;
				player->velocityY = 0;
			}

			player->velocityY -= player->accelerationY * delta;
			if (player->velocityY < 0) player->velocityY = 0;
			player->y += player->velocityY * delta;

		}
	}
	else {
		player->velocityX = 0;
		player->velocityY = 0;
	}
}

void DrawString(SDL_Surface* screen, int x, int y, const char* text,
	SDL_Surface* charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};



void DrawSpacedString(SDL_Surface* screen, int x, int y, const char* text,
	SDL_Surface* charset) {
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 32;
		text++;
	};
}

void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};

void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32*)p = color;
};


void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};


void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k,
	Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};


void lostLife(Player* player, SDL_Surface* screen, SDL_Surface* charset) {
	char text[128];
	sprintf(text, "Your score: %d", player->points);
	DrawString(screen, SCREEN_WIDTH / 2 - 8 * 4, SCREEN_HEIGHT / 2 - 60, text, charset);
	DrawString(screen, SCREEN_WIDTH / 2 - 8 * 4, SCREEN_HEIGHT / 2 - 8, "YOU LOST A LIFE,  DO YOU WANT TO CONTINUE?", charset);
	DrawString(screen, SCREEN_WIDTH / 2 - 8 * 4, SCREEN_HEIGHT / 2 + 8, "PRESS Y TO CONTINUE OR M TO GO TO THE MENU", charset);
}

bool isTouchingBeam(SDL_Surface* character, SDL_Surface** beam, int beamCount) {
	for (int i = 0; i < beamCount; i++) {
		if (SDL_HasIntersection(&character->clip_rect, &beam[i]->clip_rect)) {
			return true;
		}
	}
	return false;
}

bool isTouchingLadder(SDL_Surface* character, SDL_Surface** ladder, int ladderCount) {
	for (int i = 0; i < ladderCount; i++) {
		if (SDL_HasIntersection(&character->clip_rect, &ladder[i]->clip_rect)) {
			return true;
		}
	}
	return false;
}

bool isNowJumping(Player player) {
	if (player.velocityY < 0) {
		return true;
	}
	return false;
}

void canCollectTrophy(SDL_Surface* character, SDL_Rect banana, Player* player, int* earnedPoints, bool* isTrophyCollected) {
	if (SDL_HasIntersection(&character->clip_rect, &banana)) {
		player->points += 500;
		*earnedPoints = 500;
		*isTrophyCollected = true;
	}
}

void barrelMechanics(int beamCount, int barrelController, float delta, SDL_Surface** beam, Barrel* barrels, SDL_Surface* barrelSurface) {
	bool beamBarrelCollision;
	beamBarrelCollision = isTouchingBeam(barrelSurface, beam, beamCount);
	if (beamBarrelCollision == true) {
		barrels->currentBeam = touchingBeamIndex(barrelSurface, beam, beamCount);

		if (barrels->lastBeam != -1) {
			if (barrels->isFalling == true && (beam[barrels->currentBeam]->clip_rect.y - beam[barrels->lastBeam]->clip_rect.y > 100)) {
				barrels->changeDirection();
			}
		}
		barrels->x += barrels->velocityX * delta;
		barrels->isFalling = false;

	}
	else {

		barrels->y += barrels->gravity * delta;
		barrels->isFalling = true;
		barrels->lastBeam = barrels->currentBeam;
	}
	if (barrels->velocityX > 0) {
		barrels->rotationAngle += 0.5;
	}
	else {
		barrels->rotationAngle -= 0.5;
	}

}


bool setOnGround(SDL_Surface* character, SDL_Surface** beam, int beamCount, Player* player) {
	SDL_Rect charRect = character->clip_rect;
	charRect.y -= 60;
	for (int i = 0; i < beamCount; i++) {
		if (SDL_HasIntersection(&charRect, &beam[i]->clip_rect)) {
			player->y = beam[i]->clip_rect.y;
			player->y -= character->clip_rect.h;
			player->isOnLadder = false;
			return true;
		}
	}
	return false;
}

Scores* sortScores(Scores* scores, int length) {
	Scores temp;
	for (int i = 0; i < length; i++) {
		for (int j = 0; j < length - 1; j++) {
			if (scores[j].points < scores[j + 1].points) {
				temp = scores[j];
				scores[j] = scores[j + 1];
				scores[j + 1] = temp;
			}
		}
	}
	return scores;
}

void highScoreView(SDL_Surface* screen, SDL_Surface* charset,int* step) {

	FILE* file = fopen("highscores.txt", "r");
	char text[128];
	int points = 0;
	int length = 0;
	int k;
	DrawString(screen, SCREEN_WIDTH / 2 - 100, 20, "PRESS M TO GO TO THE MENU", charset);
	DrawString(screen, SCREEN_WIDTH / 2 - 100, 50, "USE ARROWS TO NAVIGATE", charset);
	
	while (fscanf(file, "%s %d", text, &points) != EOF) {
			length++;
	}
	fclose(file);
	Scores * scores = new Scores[length];
	file = fopen("highscores.txt", "r");
	for (int i = 0; i < length; i++) {
		fscanf(file, "%s %d", scores[i].nickName, &scores[i].points);
		sprintf(text, "%s %d", scores[i].nickName, scores[i].points);
		
	}
	scores=sortScores(scores, length);
	if (*step > length) {
		*step -= 5;
	}
	for (int i = 0+*step; i < length; i++) {
		k = 100 + (i-*step )* 30;
		sprintf(text, "%d. %s %d", i+1,scores[i].nickName, scores[i].points);
		if(k<SCREEN_HEIGHT-100 && i<5+*step)
		DrawString(screen, SCREEN_WIDTH / 2 - 100, k, text, charset);
	}

	fclose(file);

}


void menuLevels(SDL_Surface** screen, SDL_Surface* charset, SDL_Renderer** renderer,SDL_Event* event, SDL_Texture** scrtex) {
	SDL_RenderClear(*renderer);
	DrawString(*screen, 500, 300, "1 - Level 1", charset);
	DrawString(*screen, 500, 400, "2 - Level 2", charset);
	DrawString(*screen, 500, 500, "3 - Level 3", charset);
	DrawString(*screen, 500, 600, "M- Menu", charset);
	SDL_UpdateTexture(*scrtex, NULL, screen[0]->pixels, screen[0]->pitch);
	SDL_RenderCopy(*renderer, *scrtex, NULL, NULL);
	SDL_RenderPresent(*renderer);
}

void highScoreMenu(SDL_Surface** screen, SDL_Surface* charset, SDL_Renderer** renderer, SDL_Event* event, SDL_Texture** scrtex, int points) {
	char pointsText[128]="";
	sprintf(pointsText, "Your score: %d", points);
	DrawString(*screen, 500, 260, pointsText, charset);
	DrawString(*screen, 500, 300, "SAVE THE RESULT", charset);
	DrawString(*screen, 480, 340, "TYPE IN YOUR NICKNAME", charset);
	DrawString(*screen, 480, 380, "PRESS ENTER TO CONTINUE", charset);
}

void LoadLevel(SDL_Renderer* renderer, SDL_Surface*** beam, SDL_Texture*** beamTexture, SDL_Surface*** ladder, SDL_Texture*** ladderTexture,
	Cords** beamCoords, int* beamCount, Cords** ladderCoords, int* ladderCount, SDL_Rect* beamRect, SDL_Rect* ladderRect, SDL_Surface** portal,
	SDL_Rect* monkeyRect, SDL_Rect* bananaRect, SDL_Rect* portalRect, bool* levelLoaded, char* path, Player* player) {

	FILE* file = fopen(path, "r");
	int x, y;
	if (file == nullptr) {
		return;
	}

	fscanf(file, "BEAMS %d", beamCount);
	*beamCoords = new Cords[*beamCount];
	for (int i = 0; i < *beamCount; ++i) {
		fscanf(file, "%d %d", &(*beamCoords)[i].x, &(*beamCoords)[i].y);
	}

	fscanf(file, " ");
	fscanf(file, "LADDERS %d", ladderCount);
	*ladderCoords = new Cords[*ladderCount];
	for (int i = 0; i < *ladderCount; ++i) {
		fscanf(file, "%d %d", &(*ladderCoords)[i].x, &(*ladderCoords)[i].y);
	}

	fscanf(file, " ");
	fscanf(file, "MONKEY %d %d", &monkeyRect->x, &monkeyRect->y);
	fscanf(file, " ");
	fscanf(file, "TROPHY %d %d", &bananaRect->x, &bananaRect->y);
	fscanf(file, " ");
	fscanf(file, "PORTAL %d %d", &portalRect->x, &portalRect->y);
	fscanf(file, " ");
	fscanf(file, "PLAYER %d %d", &x, &y);
	player->x = x;
	player->y = y;

	*beam = (SDL_Surface**)malloc(*beamCount * sizeof(SDL_Surface*));
	*ladder = (SDL_Surface**)malloc(*ladderCount * sizeof(SDL_Surface*));
	*beamTexture = (SDL_Texture**)malloc(*beamCount * sizeof(SDL_Texture*));
	*ladderTexture = (SDL_Texture**)malloc(*ladderCount * sizeof(SDL_Texture*));

	(*beam)[0] = SDL_LoadBMP("./beam.bmp");
	(*beamTexture)[0] = SDL_CreateTextureFromSurface(renderer, (*beam)[0]);
	SDL_SetTextureBlendMode((*beamTexture)[0], SDL_BLENDMODE_BLEND);
	SDL_QueryTexture((*beamTexture)[0], NULL, NULL, &beamRect->w, &beamRect->h);

	(*ladder)[0] = SDL_LoadBMP("./ladder.bmp");
	(*ladderTexture)[0] = SDL_CreateTextureFromSurface(renderer, (*ladder)[0]);
	SDL_QueryTexture((*ladderTexture)[0], NULL, NULL, &ladderRect->w, &ladderRect->h);

	for (int i = 0; i < *beamCount; i++) {
		(*beam)[i] = SDL_LoadBMP("./beam.bmp");
		(*beam)[i]->clip_rect = { (*beamCoords)[i].x,(*beamCoords)[i].y, beamRect->w, beamRect->h };
	}

	for (int i = 0; i < *ladderCount; i++) {
		(*ladder)[i] = SDL_LoadBMP("./ladder.bmp");
		(*ladder)[i]->clip_rect = { (*ladderCoords)[i].x + ladderRect->w / 4 ,(*ladderCoords)[i].y + 10, ladderRect->w / 2, ladderRect->h };
	}

	*levelLoaded = true;
	portal[0]->clip_rect = {portalRect[0].x,portalRect[0].y, portalRect[0].w, portalRect[0].h};

	fclose(file);
}



void loadTexture(SDL_Renderer* renderer, SDL_Surface** surface, SDL_Texture** texture,char* path, SDL_Rect* rect) {
	int w, h;
	*surface = SDL_LoadBMP(path);
	*texture = SDL_CreateTextureFromSurface(renderer, *surface);
	SDL_SetTextureBlendMode(*texture, SDL_BLENDMODE_BLEND);
	SDL_QueryTexture(*texture, NULL, NULL, &w, &h);
	rect->w = w;
	rect->h = h;
}


void monkeyAnimation(float delta, int* earnedPoints, double* showPointsTime, double* lastSpawn,
	int* barrelController, int barrelCount, SDL_Texture** monkeyTexture,
	SDL_Texture** monkey_rightTexture, SDL_Texture** monkeyBufor, SDL_Texture** monkey_leftTexture) {
	if (*earnedPoints != 0) {
		*showPointsTime += delta;
	}

	if (*showPointsTime > 1.5) {
		*showPointsTime = 0;
		*earnedPoints = 0;
	}

	if (*lastSpawn > 0.2 && *barrelController < barrelCount - 1) {
		*monkeyTexture = *monkey_rightTexture;
		(*barrelController)++;
		*lastSpawn = -5;
	}

	if (*lastSpawn > -4.5) {
		*monkeyTexture = *monkeyBufor;
	}

	if (*lastSpawn > -1) {
		*monkeyTexture = *monkey_leftTexture;
	}
}

void playerAnimation(Player* player, SDL_Texture** characterTexture, SDL_Texture* bufor,
	SDL_Texture* jumpTexture, SDL_Texture* character_rightTexture1,
	SDL_Texture* character_rightTexture2, SDL_Texture* character_leftTexture1,
	SDL_Texture* character_leftTexture2, SDL_Texture* character_climbTexture,
	SDL_Texture* character_climbTexture2, double* animationController) {

	if (player->velocityY >= 0 && player->velocityX == 0 && player->isOnLadder==false) {
		*characterTexture = bufor;
	}
	else if (player->velocityY < 0 && player->isOnLadder == false) {
		*characterTexture = jumpTexture;
	}


	if (player->velocityX > 0 && player->isOnLadder == false && player->velocityY == 0) {
		if (*animationController < 0.3) {
			*characterTexture = character_rightTexture1;

		}
		else if (*animationController > 0.3 && *animationController < 0.6) {
			*characterTexture = character_rightTexture2;
		}
		else {
			*animationController = 0;
		}
	}
	else if (player->velocityX < 0 && player->isOnLadder == false && player->velocityY == 0) {
		if (*animationController < 0.3) {
			*characterTexture = character_leftTexture1;

		}
		else if (*animationController > 0.3 && *animationController < 0.6) {
			*characterTexture = character_leftTexture2;
		}
		else {
			*animationController = 0;
		}
	}

	if (player->isOnLadder == true) {
		if (player->velocityY != 0) {
			if (*animationController < 0.1) {
				*characterTexture = character_climbTexture2;

			}
			else if (*animationController > 0.1 && *animationController < 0.2) {
				*characterTexture = character_climbTexture;
			}
			else {
				*animationController = 0;
			}
		}
	}

}

void gravity(bool touchingBeam, Player* player, double delta) {
	if (touchingBeam == false && player->isOnLadder == false) {
		player->y += 3 * player->accelerationY * delta;
	}
}




#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv) {
	int t1, t2, quit, frames, rc;
	double delta, worldTime, fpsTimer, fps, lastSpawn, animationController, showPointsTime;
	SDL_Event event;
	SDL_Surface* screen, * charset;
	SDL_Surface* character, * character_right, * character_left, * character_climb;
	SDL_Texture* scrtex;
	SDL_Window* window;
	SDL_Renderer* renderer;
	Player player;
	int beamCount;
	int ladderCount;
	int barrelCount = 15;
	int barrelController = 0;
	bool trophieCollected = false;
	bool isGameStopped = true;
	bool isMenuOn = true;
	int level = 1;
	int menuChoice = 0;
	int earnedPoints = 0;
	bool levelLoaded = false;
	char nickname[128] = "";

	SDL_Surface** barrelSurface = (SDL_Surface**)malloc(barrelCount * sizeof(SDL_Surface*));
	SDL_Surface* monkey, * monkey_left, * monkey_right;
	SDL_Surface* portal;
	SDL_Surface* characterJump;
	SDL_Surface* character_right1;
	SDL_Surface* character_left1;
	SDL_Surface* character_climb2;
	SDL_Surface* character_right2;
	SDL_Surface* character_left2;
	SDL_Surface* heart;
	SDL_Surface* banana;

	SDL_Rect beamRect;
	SDL_Rect ladderRect;
	SDL_Rect characterRect;
	SDL_Rect barrelrect;
	SDL_Rect monkeyRect;
	SDL_Rect portalRect;
	SDL_Rect bananaRect;
	Barrel* barrels = new Barrel[barrelCount];

	SDL_Texture** barrelTexture = (SDL_Texture**)malloc(barrelCount * sizeof(SDL_Texture*));
	SDL_Texture* characterTexture, * character_rightTexture1, * character_leftTexture1, * character_climbTexture2;
	SDL_Texture* character_rightTexture2, * character_leftTexture2;
	SDL_Texture* monkeyTexture;
	SDL_Texture* character_rightTexture;
	SDL_Texture* character_leftTexture;
	SDL_Texture* bufor, * monkeyBufor;
	SDL_Texture* character_climbTexture;
	SDL_Texture* monkey_leftTexture;
	SDL_Texture* monkey_rightTexture;
	SDL_Texture* portalTexture;
	SDL_Texture* jumpTexture;
	SDL_Texture* bananaTexture;

	SDL_Surface** beam;
	SDL_Surface** ladder;
	SDL_Texture** beamTexture;
	SDL_Texture** ladderTexture;


	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}
	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
		&window, &renderer);
	if (rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	};
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_SetWindowTitle(window, "Krzysztof Kondracki 197810 Donkey Kong");

	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);

	SDL_ShowCursor(SDL_DISABLE);



	charset = SDL_LoadBMP("./cs8x8.bmp");

	SDL_SetColorKey(charset, true, 0x000000);

	//load all textures
	loadTexture(renderer, &character, &characterTexture, "./character.bmp", &characterRect);
	loadTexture(renderer, &character_right, &character_rightTexture, "./character-right.bmp", &characterRect);
	loadTexture(renderer, &character_left, &character_leftTexture, "./character-left.bmp", &characterRect);
	loadTexture(renderer, &character_climb, &character_climbTexture, "./climb.bmp", &characterRect);
	loadTexture(renderer, &characterJump, &jumpTexture, "./jump.bmp", &characterRect);
	loadTexture(renderer, &character_right1, &character_rightTexture1, "./character-right1.bmp", &characterRect);
	loadTexture(renderer, &character_left1, &character_leftTexture1, "./character-left1.bmp", &characterRect);
	loadTexture(renderer, &character_climb2, &character_climbTexture2, "./climb2.bmp", &characterRect);
	loadTexture(renderer, &character_right2, &character_rightTexture2, "./character-right2.bmp", &characterRect);
	loadTexture(renderer, &character_left2, &character_leftTexture2, "./character-left2.bmp", &characterRect);
	loadTexture(renderer, &portal, &portalTexture, "./portal.bmp", &portalRect);
	loadTexture(renderer, &monkey, &monkeyTexture, "./monkey.bmp", &monkeyRect);
	loadTexture(renderer, &monkey_left, &monkey_leftTexture, "./monkey-left.bmp", &monkeyRect);
	loadTexture(renderer, &monkey_right, &monkey_rightTexture, "./monkey-right.bmp", &monkeyRect);
	loadTexture(renderer, &banana, &bananaTexture, "./banana.bmp", &bananaRect);
	
	heart = SDL_LoadBMP("./heart.bmp");

	for (int i = 0; i < barrelCount; i++) {
		barrelSurface[i] = SDL_LoadBMP("./barrel.bmp");
		barrelTexture[i] = SDL_CreateTextureFromSurface(renderer, barrelSurface[0]);
	}
	SDL_QueryTexture(barrelTexture[0], NULL, NULL, &barrelrect.w, &barrelrect.h);

	bufor = characterTexture;
	monkeyBufor = monkeyTexture;

		screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	char text[128];
	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
	Uint32 fullyTransparentBlue = SDL_MapRGBA(screen->format, 0x11, 0x11, 0xCC, 0x00); // 0x00 for fully transparent
	t1 = SDL_GetTicks();

	frames = 0;
	fpsTimer = 0;
	fps = 0;
	quit = 0;
	worldTime = 0;
	showPointsTime = 0;
	lastSpawn = 0;
	animationController = 0;
	earnedPoints = 0;
	int step = 0;
	bool dPressed = false;
	bool aPressed = false;

	Cords* beamCoords;
	Cords* ladderCoords;

	while (!quit) {
		if (isGameStopped == false) {
			if (levelLoaded==false) {		
				if (level == 1) {
					LoadLevel(renderer, &beam, &beamTexture, &ladder, &ladderTexture,
						&beamCoords, &beamCount, &ladderCoords, &ladderCount, &beamRect, &ladderRect, &portal,
						&monkeyRect, &bananaRect, &portalRect, &levelLoaded, "levels/level1.txt", &player);
					for (int i = 0; i < barrelCount; i++) {
						barrels[i].x = monkeyRect.x+60;
						barrels[i].y = monkeyRect.y+20;
					}
					trophieCollected = false;
				}
				else if (level == 2) {
					LoadLevel(renderer, &beam, &beamTexture, &ladder, &ladderTexture,
						&beamCoords, &beamCount, &ladderCoords, &ladderCount, &beamRect, &ladderRect, &portal,
						&monkeyRect, &bananaRect, &portalRect, &levelLoaded, "levels/level2.txt",&player);
					for (int i = 0; i < barrelCount; i++) {
						barrels[i].x = monkeyRect.x + 60;
						barrels[i].y = monkeyRect.y+20;
					}
					trophieCollected = false;
				}
				else if (level == 3) {
					LoadLevel(renderer, &beam, &beamTexture, &ladder, &ladderTexture,
						&beamCoords, &beamCount, &ladderCoords, &ladderCount, &beamRect, &ladderRect, &portal,
						&monkeyRect, &bananaRect, &portalRect, &levelLoaded, "levels/level3.txt",&player);
					for (int i = 0; i < barrelCount; i++) {
						barrels[i].x = monkeyRect.x + 60;
						barrels[i].y = monkeyRect.y+20;
					}
					trophieCollected = false;
				}

			}
			else {
				character->clip_rect = { (int)player.x,(int)player.y, characterRect.w, characterRect.h };
				for (int i = 0; i < barrelController; i++) {
					barrelSurface[i]->clip_rect = { (int)barrels[i].x,(int)barrels[i].y, barrelrect.w, barrelrect.h - 5 };
				}
				t2 = SDL_GetTicks();
				delta = (t2 - t1) * 0.001;
				t1 = t2;
				worldTime += delta;
				lastSpawn += delta;
				animationController += delta;

				if (delta < 1000 / FPS) {
					SDL_Delay((1000 / FPS) - delta);
				}

				monkeyAnimation(delta, &earnedPoints, &showPointsTime,
					&lastSpawn, &barrelController, barrelCount, &monkeyTexture,
					&monkey_rightTexture, &monkeyBufor, &monkey_leftTexture);


				bool touchingBeam = isTouchingBeam(character, beam, beamCount);
				bool touchingLadder = isTouchingLadder(character, ladder, ladderCount);
				bool isJumping = isNowJumping(player);

				if (player.isOnLadder == true && touchingLadder == false) {
					touchingBeam = false;
				}

				SDL_FillRect(screen, NULL, czarny);

				for (int i = 0; i < barrelController; i++)
					barrelMechanics(beamCount, barrelController, delta, beam, &barrels[i], barrelSurface[i]);

				for (int i = 0; i < beamCount; i++) {
					DrawSurface(screen, beam[i], beamCoords[i].x + 50, beamCoords[i].y + 6);
				}

				playerAnimation(&player, &characterTexture, bufor,
										jumpTexture, character_rightTexture1,
										character_rightTexture2, character_leftTexture1,
										character_leftTexture2, character_climbTexture,
										character_climbTexture2, &animationController);

				fpsTimer += delta;
				if (fpsTimer > 0.5) {
					fps = frames * 2;
					frames = 0;
					fpsTimer -= 0.5;
				};


				gravity(touchingBeam, &player, delta);//gravity

				sprintf(text, "Points %d          time = %.1lf s", player.points, worldTime);
				DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);
				for (int i = 0; i < ladderCount; i++) {
					DrawSurface(screen, ladder[i], ladderCoords[i].x + 32, ladderCoords[i].y + 85);
				}
				if (barrelController != barrelCount - 1)
					DrawSurface(screen, barrelSurface[barrelCount - 1], monkeyRect.x - 10, monkeyRect.y + 60);
				for (int i = 0; i < player.lives; i++)
					DrawSurface(screen, heart, 50 + i * 50, 60);
				if (earnedPoints != 0) {
					char points[6];
					sprintf(points, "%d", earnedPoints);
					DrawString(screen, character->clip_rect.x + 10, character->clip_rect.y - 20, points, charset);
				}
				SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
				SDL_RenderCopy(renderer, scrtex, NULL, NULL);
				SDL_RenderCopy(renderer, portalTexture, NULL, &portalRect);
				

				for (int i = 0; i < barrelController; i++) {
					if (barrelSurface[i]->clip_rect.y > 20) {
						if (lastSpawn == -5) {
							if(i!=barrelController-1)
							SDL_RenderCopyEx(renderer, barrelTexture[0], NULL, &barrelSurface[i]->clip_rect, barrels[i].rotationAngle, NULL, SDL_FLIP_HORIZONTAL);
						}
						else {
							SDL_RenderCopyEx(renderer, barrelTexture[0], NULL, &barrelSurface[i]->clip_rect, barrels[i].rotationAngle, NULL, SDL_FLIP_HORIZONTAL);
						}
					}
				}//barrels rendering

				if (trophieCollected == false) {
					SDL_RenderCopy(renderer, bananaTexture, NULL, &bananaRect);
				}
				SDL_RenderCopy(renderer, monkeyTexture, NULL, &monkeyRect);
				SDL_RenderCopy(renderer, characterTexture, NULL, &character->clip_rect);
				SDL_RenderPresent(renderer);


				if (barrelOfScreen(barrels, barrelCount, barrelSurface) == true) {
					barrelController--;
					SDL_Surface** bufor = barrelSurface;
					SDL_Texture** buforTexture = barrelTexture;
					Barrel* buforBarells = barrels;
					barrels = new Barrel[barrelCount + 1];
					barrelTexture = new SDL_Texture * [barrelCount + 1];
					barrelSurface = new SDL_Surface * [barrelCount + 1];
					for (int i = 0; i < barrelCount - 1; i++) {
						barrels[i] = buforBarells[i + 1];
						barrelSurface[i] = bufor[i + 1];
						barrelTexture[i] = buforTexture[i + 1];
					}

					barrelSurface[barrelCount - 1] = barrelSurface[0];
					barrelTexture[barrelCount - 1] = SDL_CreateTextureFromSurface(renderer, barrelSurface[0]);

					free(bufor);
					free(buforTexture);
					free(buforBarells);
				} //barrel of screen, adding new barrel to the monkey's stack

				checkColissions(character,beam,beamCount,&player);//checking colissions with the beams
				whenMoving(delta, &player, touchingBeam); //noving mechanics

				if (trophieCollected == false) {
					canCollectTrophy(character, bananaRect, &player, &earnedPoints, &trophieCollected);
				} 

				if (hasFinishedLevel(&player, character, portal) == true) {
					earnedPoints = 800;
					player.points += 800;
					if(level<3){
						level++;
						startNewGame(&player, &barrels, barrelCount, false, &barrelController);
						lastSpawn = 0;
						//go to next level
					}
					else {
						worldTime= 0;
						isGameStopped = true;
						isMenuOn = true;
						menuChoice = 3;
						//game won
					}
						
					levelLoaded = false;
					
				}

				if (barrelColision(character, barrelSurface, barrelController) == true || player.y > SCREEN_HEIGHT) {
					if (player.lives > 1) {
						player.lives--;
						isGameStopped = true;
						isMenuOn = false;
						levelLoaded = false;
						lastSpawn = 0;
						//lost life
					}
					else {
						isGameStopped = true;
						isMenuOn = true;
						menuChoice = 3;
						level = 1;
						//game over
					}

				}
				else if (jumpedOverBarrel(character, barrelSurface, barrelCount) == true && earnedPoints == 0 && isJumping == true && player.isOnLadder == false) {
					player.points += 100;
					earnedPoints = 100;
					//jumped over a barrel, adding points
				}

				while (SDL_PollEvent(&event)) {
					switch (event.type) {
					case SDL_KEYDOWN:
						if (event.key.keysym.sym == SDLK_ESCAPE) { //quit
							quit = 1;
						}
						else if (event.key.keysym.sym == SDLK_a) { //left
							aPressed = true;
							if (touchingBeam == true && player.isOnLadder == false) {
								player.left();
							}
						}
						else if (event.key.keysym.sym == SDLK_SPACE) { //jump
							if (touchingBeam == true && player.isOnLadder == false) {
								if (dPressed == true)
									player.jumpRight();
								else if (aPressed == true)
									player.jumpLeft();
								else
									player.jump();
							};
						}
						else if (event.key.keysym.sym == SDLK_d) { //right
							dPressed = true;
							if (touchingBeam == true && player.isOnLadder == false) {
								player.right();
							}
						}
						else if (event.key.keysym.sym == SDLK_w) { //up on ladder
							if (touchingLadder == true && ((player.isOnLadder == false && touchingBeam == true) || (player.isOnLadder == true && touchingBeam == false) || isJumping == true)) {
								if (setOnGround(character, beam, beamCount, &player) == false) {
									player.climb();
								}
								else {
									setOnGround(character, beam, beamCount, &player);
								}
							}
							else {
								if (player.isOnLadder == true) {
									setOnGround(character, beam, beamCount, &player);
								}
								player.isOnLadder = false;
							}
						}
						else if (event.key.keysym.sym == SDLK_s) { //down on ladder
							if (touchingLadder == true && touchingBeam == false) {
								player.getLower();
							}
							else {
								if (player.isOnLadder == true) {
									player.y += 65;
								}
								if (player.isOnLadder == false && isLadderBelow(character, ladder, ladderCount) == true) {
									player.isOnLadder = true;
									player.y += 65;
								}
							}
						}
						else if (event.key.keysym.sym == SDLK_n) {//new game
							SDL_RenderClear(renderer);
							startNewGame(&player, &barrels, barrelCount, true, &barrelController);
							worldTime = 0;
							lastSpawn = 0;
							showPointsTime = 0;
							levelLoaded = false;
							t1 = SDL_GetTicks();

						}
						else if (event.key.keysym.sym == SDLK_m) { //go to menu
							SDL_RenderClear(renderer);
							startNewGame(&player, &barrels, barrelCount, false, &barrelController);
							isGameStopped = true;
							isMenuOn = true;
							worldTime = 0;
							lastSpawn = 0;
							showPointsTime = 0;
							menuChoice = 0;
							levelLoaded = false;

						}
						break;

					case SDL_KEYUP:
						if (event.key.keysym.sym == SDLK_d) {
							if (touchingBeam == true && player.isOnLadder == false) {
								player.velocityX = 0;
							}
							dPressed = false;
						}
						else if (event.key.keysym.sym == SDLK_a) {
							if (touchingBeam == true && player.isOnLadder == false) {
								player.velocityX = 0;
							}
							aPressed = false;
						}
						break;

					case SDL_QUIT:
						quit = 1;
						break;
					};
				};
				frames++;
			}
		}
		else {
			SDL_FreeSurface(screen);
			screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
				0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
			if (isMenuOn == true && menuChoice==0) {
				DrawSpacedString(screen, 500, 200, "MENU", charset);
				DrawString(screen, 500, 300, "N - NEW GAME", charset);
				DrawString(screen, 500, 400, "L - CHOOSE LEVEL", charset);
				DrawString(screen, 500, 500, "H - HIGHSCORES", charset);
				DrawString(screen, 500, 600, "ESC - QUIT", charset);
				SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
				SDL_RenderCopy(renderer, scrtex, NULL, NULL);
				SDL_RenderPresent(renderer);

				while (SDL_PollEvent(&event)) {
					switch (event.type) {
					case SDL_KEYDOWN:
						switch (event.key.keysym.sym) {
						case(SDLK_ESCAPE): //quit
							quit = 1;
							break;
						case(SDLK_n)://new game 
							startNewGame(&player, &barrels, barrelCount, true, &barrelController);
							isGameStopped = false;
							isMenuOn = false;

							SDL_RenderClear(renderer); //reseting variables
							t1 = SDL_GetTicks();
							menuChoice = 0;
							worldTime = 0;
							lastSpawn = 0;
							levelLoaded = false;
							level = 1;
							break;
						case(SDLK_l):
							SDL_RenderClear(renderer); //navigating through menu
							menuChoice = 1;
							break;
						case(SDLK_h):
							SDL_RenderClear(renderer); //navigating through menu
							menuChoice = 2;
							break;
						}
						break;

					}
				}
			}
			else if (menuChoice == 1) {
				menuLevels(&screen, charset, &renderer, &event, &scrtex);

				while (SDL_PollEvent(&event)) {
					switch (event.type) {
					case SDL_KEYDOWN:
						switch (event.key.keysym.sym) {
						case SDLK_1: //level 1
							levelLoaded = false;
							menuChoice = 0;
							level = 1;
							isMenuOn = false;
							isGameStopped = false;
							startNewGame(&player, &barrels, barrelCount, true, &barrelController);
							SDL_RenderClear(renderer);
							t1 = SDL_GetTicks();
							break;
						case SDLK_2: //level 2
							levelLoaded = false;
							menuChoice = 0;
							level = 2;
							isMenuOn = false;
							isGameStopped = false;
							startNewGame(&player, &barrels, barrelCount, true, &barrelController);
							SDL_RenderClear(renderer);
							t1 = SDL_GetTicks();
							break;
						case SDLK_3://level 3
							levelLoaded = false;
							menuChoice = 0;
							isMenuOn = false;
							isGameStopped = false;
							level = 3;
							startNewGame(&player, &barrels, barrelCount, true, &barrelController);
							SDL_RenderClear(renderer);
							t1 = SDL_GetTicks();
							break;
						case SDLK_m: //go to menu
							menuChoice = 0;
							break;
						}
						SDL_RenderClear(renderer);
						break;
					}
				}
			}
			else if (menuChoice == 2) { //highscores View
				highScoreView(screen, charset,&step);
				SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
				SDL_RenderCopy(renderer, scrtex, NULL, NULL);
				SDL_RenderPresent(renderer);

				while (SDL_PollEvent(&event)) {
					switch (event.type) {
					case SDL_KEYDOWN:
						switch (event.key.keysym.sym) {
						case SDLK_m:
							menuChoice = 0;
							break;

						case SDLK_LEFT:
							if (step > 4) {
							step -= 5;
							}
							break;
						case SDLK_RIGHT:
							step += 5;
							break;
						}
						SDL_RenderClear(renderer);
						break;
					}
				}
			}
			else if (menuChoice == 3) { //adding a highscore
				
				while (SDL_PollEvent(&event)) {
					if (event.type == SDL_KEYDOWN) {
						if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(nickname) > 0) {
							nickname[strlen(nickname) - 1] = '\0';
						}
						if (event.key.keysym.sym == SDLK_RETURN && strlen(nickname) > 0) {
							saveScore(player.points,nickname);
							delta = 0;
							menuChoice = 0;
							isMenuOn = true;
							isGameStopped = true;
							level = 1;
							levelLoaded = false;
							SDL_RenderClear(renderer);
						}
					}
					else if (event.type == SDL_TEXTINPUT) {
						if (strlen(nickname) + strlen(event.text.text) < 128) {
							strcat(nickname, event.text.text);

						}
					}
				}

				DrawString(screen, 300, 500, nickname, charset);
				highScoreMenu(&screen, charset, &renderer, &event, &scrtex, player.points);
				SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
				SDL_RenderCopy(renderer, scrtex, NULL, NULL);
				SDL_RenderPresent(renderer);
			}
			else {
				lostLife(&player, screen, charset);
				SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
				SDL_RenderCopy(renderer, scrtex, NULL, NULL);
				SDL_RenderPresent(renderer);

				while (SDL_PollEvent(&event)) {
					switch (event.type) {
					case SDL_KEYDOWN:
						switch (event.key.keysym.sym) {
						case(SDLK_y): //continue
							t1 = SDL_GetTicks();
							SDL_RenderClear(renderer);
							startNewGame(&player, &barrels, barrelCount, false, &barrelController);
							isGameStopped = false;
							isMenuOn = false;
							lastSpawn = 0;
							break;
						case(SDLK_m): //go to menu
							startNewGame(&player, &barrels, barrelCount, true, &barrelController);
							isMenuOn = true;
							worldTime = 0;
							lastSpawn = 0;
							showPointsTime = 0;
							SDL_RenderClear(renderer);
							break;
						}
						break;

					}
				}

			}
		}
	}

	//free everything
	SDL_FreeSurface(monkey);
	SDL_FreeSurface(monkey_left);
	SDL_FreeSurface(monkey_right);
	SDL_FreeSurface(portal);
	SDL_FreeSurface(character);
	SDL_FreeSurface(character_right);
	SDL_FreeSurface(character_left);
	SDL_FreeSurface(character_climb);
	SDL_FreeSurface(characterJump);
	SDL_FreeSurface(character_right1);
	SDL_FreeSurface(character_left1);
	SDL_FreeSurface(character_climb2);
	SDL_FreeSurface(character_right2);
	SDL_FreeSurface(character_left2);
	SDL_FreeSurface(heart);
	SDL_FreeSurface(banana);

	SDL_DestroyTexture(characterTexture);
	SDL_DestroyTexture(character_rightTexture1);
	SDL_DestroyTexture(character_leftTexture1);
	SDL_DestroyTexture(character_climbTexture2);
	SDL_DestroyTexture(character_rightTexture2);
	SDL_DestroyTexture(character_leftTexture2);
	SDL_DestroyTexture(monkeyTexture);
	SDL_DestroyTexture(character_rightTexture);
	SDL_DestroyTexture(character_leftTexture);
	SDL_DestroyTexture(bufor);
	SDL_DestroyTexture(monkeyBufor);
	SDL_DestroyTexture(character_climbTexture);
	SDL_DestroyTexture(monkey_leftTexture);
	SDL_DestroyTexture(monkey_rightTexture);
	SDL_DestroyTexture(portalTexture);
	SDL_DestroyTexture(jumpTexture);
	SDL_DestroyTexture(bananaTexture);
	
	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
};

