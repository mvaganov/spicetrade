#include "gamestate.h"
#include "playerstate.h"

void GameOver::CalculateWinners() {
	Game& g = *data;
	List<int> scores(g.players.Length());
	int winScore = 0;
	// find out the winning score
	for(int i = 0; i < g.players.Length(); ++i) {
		int s = Player::CalculateScore(g.players[i]);
		scores[i] = s;
		if(s > winScore) { winScore = s; }
	}
	// mark everyone who won with the winner state
	for(int i = 0; i < g.players.Length(); ++i) {
		Player& p = g.players[i];
		Finished* f = Player::SetUIState<Finished>(g, p);
		int s = scores[i];
		f->FinalMessage(s, s == winScore);
	}
}