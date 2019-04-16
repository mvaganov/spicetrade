#include "platform_conio.h"
#include "list.h"
#include "cards.h"
#include "state_base.h"
#include "player.h"

class Game
{
	/** who the current active player is */
	int currentPlayer;
	/** whether or not the game wants to keep running */
	bool m_running;
	/** stores the next user input to process */
	int userInput;
	/** what turn it is right now */
	int turn;
	/** how many updates have happened */
	unsigned int updates;
	/** the current (and pending) UI */
	Queue<GameState*> * m_stateQueue;

public:
	List<Player> players;
	/** @return whether or not the game wants to keep running */
	bool isRunning(){return m_running;}
	/** get a color for a frame-dependent color animation */
	int flashColor(int style, Graphics * g);

	int getTurn(){return turn;}

	void nextTurn(){
		turn++;
		currentPlayer++;
		if(currentPlayer >= players.Length())
			currentPlayer = 0;
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

	void initStateMachine();

	Game(int numPlayers, int width, int height);
	void release();
	~Game(){release();}

	void draw(Graphics * g);

	void setInput(int a_input);
	void processInput();

	bool isAcceptingInput(){
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
		return &(players[currentPlayer]);
	}

	int getCurrentPlayerIndex(){return currentPlayer;}
};