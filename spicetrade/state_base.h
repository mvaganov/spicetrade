#pragma once
#include "graphics.h"

class Game;

class GameState {
  public:
	Game* m_game;
	bool m_stateDone;
	void setDone (bool a_isDone) { m_stateDone = a_isDone; }
	bool isDone () { return m_stateDone; }
	virtual void init (Game* a_g) {
		m_game = a_g;
		m_stateDone = false;
	}
	virtual void draw (Graphics* g) {}
	virtual void processInput (int key) {}
	virtual void release () {}
	virtual const char* getName () = 0;
	virtual ~GameState () {}
};
