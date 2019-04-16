#pragma once
struct Vector2{
	int x, y;
	Vector2(int x, int y):x(x),y(y){}
	Vector2():x(0),y(0){}
	bool operator==(Vector2 & other) {
		return x == other.x && y == other.y;
	}
	bool operator!=(Vector2 & other) {
		return !operator==(other);
	}
	void set(int x, int y){this->x=x;this->y=y;}
};
/** used by the flood fill algorithm to determine how many steps from the origin a point is */
struct Step : public Vector2 {
	int step;
	Step(int x, int y, int step):Vector2(x,y),step(step){}
	Step():Vector2(0,0),step(0){}
	Step(Vector2 v, int step):Vector2(v.x,v.y),step(step){}
	Step(Vector2 v):Vector2(v.x,v.y),step(0){}
};
typedef Vector2 Coord;