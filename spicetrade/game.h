#pragma once
#include "platform_conio.h"
#include "list.h"
#include "cards.h"
#include "state_base.h"
#include "player.h"
#include "platform_random.h"

class Game
{
	/** who the current active player is */
	int currentPlayer;
	/** whether or not the game wants to keep running */
	bool m_running;
	/** what turn it is right now */
	int turn;
	/** how many updates have happened */
	unsigned int updates;
	/** the current (and pending) UI */
	Queue<GameState*> * m_stateQueue;

public:
	/** stores the next user input to process */
	int userInput; // TODO make private
	List<Player> players;
	List<Player*> playerUIOrder;
	Dictionary<char, const ResourceType*> resourceLookup;
	// code -> index-in-inventory
	Dictionary<char, int> resourceIndex;
	VList<const ResourceType*> collectableResources;
	VList<const PlayAction*> play_deck;
	VList<const Objective*> achievement_deck;
	List<const Objective*> achievements;
	List<const PlayAction*> market;
	List<List<int>*> acquireBonus;
	List<int> resourcePutInto; // used during acquire and upgrade UI

	bool running = true;
	// how many cards to display vertically at once (can scroll)
	int handDisplayCount = 10;
	int goldLeft = 5, silverLeft = 5;
	int maxInventory = 10;

	static const int achievementCards(){return 5;}
	static const int marketCards(){return 6;}

	void init() {
		running = true;
		handDisplayCount = 10;
		goldLeft = 5;
		silverLeft = 5;
		maxInventory = 10;

		for (int i = 0; i < g_len_resources; ++i) {
			resourceLookup.Set (g_resources[i].icon, &(g_resources[i]));
			if (g_resources[i].type == ResourceType::Type::resource) {
				collectableResources.Add (&(g_resources[i]));
				resourceIndex.Set (g_resources[i].icon, i);
			}
		}
		play_deck.EnsureCapacity(g_len_play_deck);
		for (int i = 0; i < g_len_play_deck; ++i) {
			play_deck.Add (&(g_play_deck[i]));
		}
		platform_shuffle (play_deck.GetData (), 0, play_deck.Count ());
		achievement_deck.EnsureCapacity(g_len_objective);
		for (int i = 0; i < g_len_objective; ++i) {
			achievement_deck.Add (&(g_objective[i]));
		}
		platform_shuffle (achievement_deck.GetData (), 0, achievement_deck.Count ());

		achievements.SetLength(achievementCards());
		achievements.SetAll(NULL);
		for (int i = 0; i < achievements.Length(); ++i) {
			if(achievement_deck.Count() > 0) {
				achievements.Set(i, achievement_deck.PopLast ());
			}
		}
		market.SetLength(marketCards());
		market.SetAll(NULL);
		for (int i = 0; i < market.Length(); ++i) {
			if(play_deck.Count() > 0){
				market.Set(i, play_deck.PopLast ());
			}
		}

		acquireBonus.SetLength(market.Length());
		acquireBonus.SetAll(NULL);
		resourcePutInto.SetLength(market.Length()-1);
	}

	char ColorOfRes (char icon) {
		const ResourceType* r = resourceLookup.Get (icon);
		return (r == NULL) ? CLI::COLOR::LIGHT_GRAY : r->color;
	}

	/** @return whether or not the game wants to keep running */
	bool isRunning(){return m_running;}
	/** get a color for a frame-dependent color animation */
	int flashColor(int style, Graphics * g);

	int getTurn(){return turn;}

	void nextTurn(){
		turn++;
		currentPlayer++;
		if(currentPlayer >= players.Length()) {
			currentPlayer = 0;
		}
		for(int i = 0; i < players.Length(); ++i){
			playerUIOrder[i] = &players[(currentPlayer+i) % players.Length()];
		}
	}
	Player * getPlayer(int index){return &(players[index]);}
	int getPlayerCount(){return players.Length();}

	/** add a game state to the queue */
	void queueState(GameState * a_state){
		m_stateQueue->Enqueue(a_state);
	}

	/** advance to the next state in the queue */
	void nextState() {
		GameState * state;
		if(m_stateQueue->Count() > 0){
			state = m_stateQueue->PopFront();
			state->release();
			DELMEM(state);
		}
		if(m_stateQueue->Count() > 0){
			state = m_stateQueue->PeekFront();
			state->init(this);
		}
	}

	void initStateMachine(){}

	void initScreen(int width, int height){
		CLI::init ();
		CLI::setSize (width, height);
		CLI::fillScreen (' ');
		CLI::move (0, 0);
	}

	void initPlayers(int playerCount) {
		players.SetLength(playerCount);
		playerUIOrder.SetLength(players.Length());
		for(int i = 0; i < playerCount; ++i) {
			std::string name = "player "+std::to_string(i);
			players[i].Set(name, maxInventory);
			playerUIOrder[i] = &players[i];
		}
		currentPlayer = 0;
		// TODO change the order as the players turn changes. order should be who-is-going-next, with the current-player at the top.
	}

	Game(int numPlayers, int width, int height){
		init();
		initScreen(width,height);
		initPlayers(numPlayers);
	}
	void release(){}
	~Game(){release();}

	void draw(Graphics * g){}

	void setInput(int a_input){ userInput=a_input; }
	void processInput(){
		// figure out which player the input is for
		// route the input to the right player
	}

	bool isAcceptingInput() {
		GameState * state = m_stateQueue->Peek();
		return !state->isDone();
	}

	void update()
	{
		processInput();
		GameState * state = m_stateQueue->Peek();
		if(state->isDone())
			nextState();
		updates++;
	}

	Player * getCurrentPlayer(){
		return playerUIOrder[0];
	}

	int getCurrentPlayerIndex(){return currentPlayer;}
};