#pragma once
class Player {
public:
	int direction;
	int lives;
	int velocityX;
	int velocityY;
	int accelerationX;
	int accelerationY;
	bool isOnLadder;
	int points;
	double x;
	double y;
	Player() {
		isOnLadder = false; 

		direction = 0;
		velocityX = 0;
		velocityY = 0;
		x = 260	;
		y = 750; 
		lives = 3;
		points = 0;
		accelerationY = 90;
	};
	void jump() {
		velocityY = -260;
	};
	void jumpLeft() {
		velocityY = -260;
		velocityX =-260;
	};
	void jumpRight() {
		velocityY = -260;
		velocityX = 260;
	};
	void left() {
		velocityX = -200;
	};
	void right() {
		velocityX = 200;
	};
	void climb() {
		velocityY = -100;
		velocityX = 0;
		isOnLadder = true;
	};
	void getLower() {
		velocityY = 100;
	}
};

struct Cords {
	int x;
	int y;
};

struct Scores {
	char nickName[128];
	int points;
};


class Barrel {
public:
	double x;
	double y;
	int velocityX;
	int gravity;
	float rotationAngle;
	bool isFalling;
	int lastBeam;
	int currentBeam;

	enum Direction { LEFT, RIGHT } direction;
	
	Barrel(){
		x = 0;
		y=0;
		velocityX = 100;
		gravity = 500;
		direction = LEFT;
		rotationAngle = 20;
		lastBeam = -1;
		currentBeam = -1;
	}
	void updateRotation(float delta) {
		rotationAngle += velocityX * delta; // Adjust as needed based on your game's logic
	}
	void changeDirection() {
		if (lastBeam != -1) {
			if (direction == LEFT) {
				direction = RIGHT;
				velocityX = -100;
			}
			else {
				direction = LEFT;
				velocityX = 100;
			}
		}
	}

	Direction checkDirection(int screenWidth) {
		if(x<screenWidth/2)
			return LEFT;
		else
			return RIGHT;

	}
};
