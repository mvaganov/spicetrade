#pragma once
#include "platform_conio.h"
#include "list.h"
#include "cards.h"
#include "state_base.h"
#include "player.h"
#include "graphics.h"
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
	//Queue<GameState*> * m_stateQueue;
	GameState * m_state;

	int throttle;
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

	// how many cards to display vertically at once (can scroll)
	int handDisplayCount = 10;
	int goldLeft = 5, silverLeft = 5;
	int maxInventory = 10;
	static const int COUNT_OBJECTIVE_TO_WIN = 6;

	static const int achievementCards = 5; // TODO rename COUNT_OBJECTIVE_FIELD
	static const int marketCards = 6; // TODO rename COUNT_MARKET_FIELD

	static const int MOVE_CANCEL = 1, MOVE_UP = 2, MOVE_LEFT = 3, MOVE_DOWN = 4, MOVE_RIGHT = 5, MOVE_ENTER = 6, MOVE_ENTER2 = 7, MOVE_COUNT = 7;

	void ConvertKeyToPlayerMove(int key, int& out_player, int& out_move) {
		const int p0[] = {'q','w','a','s','d','e','x'};
		const int p1[] = {'u','i','j','k','l','o','m'};
		const int p2[] = {'\n',CLI::KEY::UP,CLI::KEY::LEFT,CLI::KEY::DOWN,CLI::KEY::RIGHT,'\n','\r'};
		const int p3[] = {'-','8','4','2','6','0', '5'};
		const int* moves[] = { p0, p1, p2, p3 };
		const int num_player_controls = sizeof(moves)/sizeof(moves[0]);
		out_player = out_move = -1;
		for(int p = 0; p < num_player_controls && out_player < 0; ++p) {
			out_move = List<const int>::IndexOf(key, moves[p], 0, MOVE_COUNT);
			if(out_move != -1) {
				out_move += 1; // move codes start at 1, so 0 can be a null value
				out_player = p;
				// const char* moveNames[] = { "none","cancel","up","left","down","right","enter","enter" };
				// printf("player %d pressed %s\n", out_player, moveNames[out_move]);
			}
		}
		
	}

	void Init();

	char ColorOfRes (char icon) {
		const ResourceType* r = resourceLookup.Get (icon);
		return (r == NULL) ? CLI::COLOR::LIGHT_GRAY : r->color;
	}

	/** @return whether or not the game wants to keep running */
	bool IsRunning(){return m_running;}

	int GetTurn(){return turn;}

	void NextTurn() {
		turn++;
		currentPlayer++;
		if(currentPlayer >= players.Length()) {
			currentPlayer = 0;
		}
		for(int i = 0; i < players.Length(); ++i){
			playerUIOrder[i] = &players[(currentPlayer+i) % players.Length()];
		}
	}

	// /** add a game state to the queue */
	// void queueState(GameState * a_state){
	// 	m_stateQueue->Enqueue(a_state);
	// }

	// /** advance to the next state in the queue */
	// void nextState() {
	// 	GameState * state;
	// 	if(m_stateQueue->Count() > 0){
	// 		state = m_stateQueue->PopFront();
	// 		state->Release();
	// 		DELMEM(state);
	// 	}
	// 	if(m_stateQueue->Count() > 0){
	// 		state = m_stateQueue->PeekFront();
	// 		state->Init(this);
	// 	}
	// }

	template<typename T>
	static bool IsState(const Game& g) {
		return g.m_state != NULL && dynamic_cast<T*>(g.m_state) != NULL;
	}

	// entire template function must be in class declaration.
	template <typename T>
	static void SetState(Game& g) {
		bool oldState = Game::IsState<T>(g);
		if(oldState) { return; }
		GameState* next = NEWMEM(T);
		if(g.m_state != NULL) {
			g.m_state->Release();
			DELMEM(g.m_state);
		}
		g.m_state = next;
		next->Init(&g);
	}

	void InitStateMachine(){}

	void InitScreen(int width, int height){
		CLI::init ();
		CLI::setSize (width, height);
		//CLI::setDoubleBuffered(true);
		CLI::move (0, 0);
		CLI::fillScreen (' ');
	}

	void InitPlayers(int playerCount);

	Game(int numPlayers, int width, int height):m_state(NULL) {
		Init();
		InitScreen(width,height);
		InitPlayers(numPlayers);
	}
	void Release() {
		for(int i = 0; i < acquireBonus.Length(); ++i) {
			if(acquireBonus[i] != NULL) {
				DELMEM(acquireBonus[i]);
				acquireBonus[i] = NULL;
			}
		}
		if(m_state) {
			DELMEM(m_state);
			m_state = NULL;
		}
		CLI::release ();
	}
	~Game(){Release();}

	void Draw() {
		m_state->Draw();
	}

	void NormalDraw();

	void SetInput(int a_input) { userInput=a_input; }

	void RefreshInput();

	void ProcessInput() {
		switch (userInput) {
		case 27:
			printf("Quit!");
			m_running = false;
			break;
		default:
			m_state->ProcessInput(userInput);
		}
		userInput = 0;
	}
	
	void HandlePlayerInput(int userInput) {
		int playerID, moveID;
		ConvertKeyToPlayerMove(userInput, playerID, moveID);
		if(playerID >= this->players.Length()) {
			playerID = currentPlayer;
		}
		if(playerID >= 0) {
			Player::UpdateInput(*this, *GetPlayer(playerID), moveID);
		} else {
			printf("did not understand %d\n", userInput);
		}
	}

	// bool isAcceptingInput() {
	// 	GameState * state = m_stateQueue->Peek();
	// 	return !state->IsDone();
	// }

	void Update()
	{
		ProcessInput();
		m_state->Update();
		// GameState * state = m_stateQueue->Peek();
		// if(state->IsDone())
		// 	nextState();
		updates++;
	}

	Player * GetCurrentPlayer() {
		return playerUIOrder[0];
	}

	Player * GetPlayer(int index) {return &(players[index]);}
	int GetPlayerCount() { return players.Length(); }

	int GetCurrentPlayerIndex() { return currentPlayer; }

	bool IsEvenOnePlayerIsResourceManaging(const List<Player*>& players);

	static void UpdateObjectiveBuy(Game&g, Player& p, int userInput);
	static void UpdateAcquireMarket(Game& g, Player& p, int userInput);
	static void UpdateMarket(Game& g, Player& p, int userInput);
	static void PrintObjectives(Game& g, Coord cursor);
	static void PrintMarket(Game& g, Coord cursor);
};