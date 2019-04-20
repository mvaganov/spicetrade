#include "state_base.h"
#include "game.h"

// reconcile resources down to 10
class ResWaste : public PlayerState {
	virtual const char* GetName () { return "resource management"; };
	virtual void Init (const GamePlayer& a_data) {
		PlayerState::Init(a_data);
		Player& p = *data.p;
		p.inventoryPrediction.Copy(p.inventory);
	}
	virtual void Release () { }
	virtual void ProcessInput (int key) {
		Player::UpdateInventory(*data.g, *data.p, key);
	}
};

// upgrade and upgrade prediction
class ResUpgrade : public PlayerState {
	virtual const char* GetName () { return "upgrading resources"; };
	virtual void Init (const GamePlayer& a_data) {
		PlayerState::Init(a_data);
		Player& p = *data.p;
		p.inventoryPrediction.Copy(p.inventory);
		p.upgradesMade = 0;
	}
	virtual void Release () {
		Player& p = *data.p;
		p.inventory.Copy(p.inventoryPrediction);
		p.upgradeChoices = 0;
		p.upgradesMade = 0;		
	}
	virtual void ProcessInput (int key) {
		Player::UpdateUpgrade(*data.g, *data.p, key);
	}
};

// selecting card to play
// predicting card impact on inventory
// re-ordering cards
class HandManage : public PlayerState {
	virtual const char* GetName () { return "card management"; };
	virtual void Init (const GamePlayer& a_data) {
		PlayerState::Init(a_data);
		Player& p = *data.p;
		p.selected.Clear();
		p.handPrediction.Copy(p.hand);
		p.inventoryPrediction.Copy(p.inventory);
	}
	virtual void Release () {
		Player& p = *data.p;
		p.selected.Clear();
		p.handPrediction.Copy(p.hand);
		p.playedPrediction.Copy(p.played);
		p.inventoryPrediction.Copy(p.inventory);
	}
	virtual void ProcessInput (int key) {
		Player::UpdateHand (*data.g, *data.p, key, data.g->handDisplayCount);
	}
};

class CardBuy : public PlayerState {
	virtual const char* GetName () { return "selecting next card"; };
	virtual void Init (const GamePlayer& a_data) { PlayerState::Init(a_data); }
	virtual void Release () {}
	virtual void ProcessInput (int key) {
		Game::UpdateMarket(*data.g, *data.p, key);
	}
};

class CardBuyDeep : public PlayerState {
	virtual const char* GetName () { return "acquiring next card"; };
	virtual void Init (const GamePlayer& a_data) {
		PlayerState::Init(a_data);
		Game& g = *data.g;
		Player& p = *data.p;
		p.marketCursor = 0; // move cursor back to 0 so the user knows where to start paying...
		g.resourcePutInto.SetAll(-1);
		for(int i = 0; i < p.marketCardToBuy; ++i) {
			if(g.acquireBonus[i] == NULL) {
				g.acquireBonus[i] = NEWMEM(List<int>(g.collectableResources.Count(), 0));
			}
		}

	}
	virtual void Release () {}
	virtual void ProcessInput (int key) {
		Game::UpdateAcquireMarket(*data.g, *data.p, key);
	}
};

class ObjectiveBuy : public PlayerState {
	virtual const char* GetName () { return "selecting objective"; };
	virtual void Init (const GamePlayer& a_data) { PlayerState::Init(a_data); }
	virtual void Release () {}
	virtual void ProcessInput (int key) {
		Game::UpdateObjectiveBuy(*data.g, *data.p, key);
	}
};
