#include "state_base.h"
#include "game.h"

// GameNormal : GameState
class GameNormal : public GameState {
	virtual std::string GetName () { return "regular play"; };
	virtual void Init (Game*const& a_data) {
		GameState::Init(a_data);
	}
	virtual void Draw() { data->NormalDraw(); }
	virtual void ProcessInput (int key) {
		data->HandlePlayerInput(key);
	}
};

// GameOver : GameState
class GameOver : public GameState {
	virtual std::string GetName () { return "game over"; };
	virtual void Init (Game*const& a_data) {
		GameState::Init(a_data);
		CalculateWinners();
	}
	void CalculateWinners();
	virtual void Draw() { data->NormalDraw(); }
};

// GameNearlyOver : GameState
class GameNearlyOver : public GameState {
	int lastTurn;
	virtual std::string GetName () { return "final round"; };
	virtual void Init (Game*const& a_data) {
		GameState::Init(a_data);
		lastTurn = data->GetTurn();
	}
	virtual void Update() {
		if(data->GetTurn() != lastTurn) {
			Game& g = *data;
			// if every player is finished managing their resources...
			if(g.IsEvenOnePlayerIsResourceManaging(g.playerUIOrder) == false) {
				// GAME OVER!
				Game::SetState<GameOver>(*data);
			}
		}
	}
	virtual void Draw() { data->NormalDraw(); }
	virtual void ProcessInput (int key) {
		data->HandlePlayerInput(key);
	}
};