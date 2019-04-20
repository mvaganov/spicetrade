#pragma once

template<typename DATA>
class State {
  public:
	DATA data;
	bool m_stateDone;
	void SetDone (bool a_isDone) { m_stateDone = a_isDone; }
	bool IsDone () { return m_stateDone; }
	virtual void Init (const DATA& a_data) {
		data = a_data;
		m_stateDone = false;
	}
	virtual void Draw () {}
	virtual void ProcessInput (int key) {}
	virtual void Release () {}
	virtual const char* GetName () = 0;
	virtual ~State () {}
};


class Game;
class Player;

struct GamePlayer {
	Game* g;
	Player* p;
	GamePlayer():g(NULL),p(NULL){}
	GamePlayer(Game* g, Player* p):g(g),p(p){}
};

class GameState : public State<Game*>{};

class PlayerState : public State<GamePlayer>{};
