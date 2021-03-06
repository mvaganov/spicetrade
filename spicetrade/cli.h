#pragma once

#include "clibuffer.h"

// TODO make a getstring or getline function that puts getchar input into a given string array of a given size

// stores input/output buffers for the CLI
class CLIBuffer;

/** used to make CLI fails fast and loud if it gets into a bad situations */
//#define CLI_DEBUG

/** tests dirty cell, showing which tiles are being redrawn */
//#define TEST_DIRTY_CELL

/**
* (C)ommand (L)ine (I)nterface
* A Windows/Linux abstraction layer for color and cursor position control.
*
* Work In Progress
*
* @author Michael Vaganov
*/
namespace CLI {
	/** version information */
	extern const char *version;

	/** a Command Line Interface object. multiple can be created */
	class BufferManager {
		/** if creating a non-standard i/o or using double buffering */
		CLIBuffer * m_buff;
		/** input buffer variables used for multi-byte key inputs, like arrow keys */
		unsigned int m_inputBufferCount;
		/** remember color changes */
		int m_fcolor, m_bcolor;
		/** queues key input entered by the user */
		int m_inputBuffer[40];
	public:
		/** creates a non-double-buffered BufferManager */
		BufferManager();
		~BufferManager();

		/** move the cursor to the given location in the console */
		void move(int row, int col);

		/** as stdio.h's printf, built for this CLI environment */
		int printf(const char *fmt, ...);

		/** use CLI::COLOR values for foreground and background */
		void setColor(int foreground, int background);

		/** @return true if at least one byte is in the input buffer */
		bool kbhit();

		/**
		* an additional abstraction layer for platform_conio.h which normalizes multibyte inputs
		* @return key press from input buffer, -1 none. {@link kbhit()}
		*/
		int getchar();

		/** @param a_numbytes how much to absorb of the (beginning end) input buffer */
		void inputBufferConsume(unsigned int numBytes);

		/** put a single character at the cursor (console output), advancing it */
		void putchar(const int a_letter);

		/** printing a string */
		void puts( const char *str, const int size );

		/**
		* @param text what text to draw
		* @param a_x rectangle x
		* @param a_y rectangle y
		* @param a_w rectangle width
		* @param a_h rectangle height
		* @param wrapping whether or not to wrap to the next line if the text goes over the edge
		*/
		void drawText(const char * text, int a_x, int a_y, int a_w, int a_h, bool wraping);

		/** fills the given rectangle with the given character, using the current fcolor and bcolor */
		void fillRect(int x, int y, int width, int height, char fill);

		/** determine how big the screen the user asked for is */
		Coord getSize();

		/** @return number of columns. may be set with {@link #setSize(int,int)} */
		int getWidth();

		/** @return number of rows. may be set with {@link #setSize(int,int)} */
		int getHeight();

		/** force {@link #getWidth()} and {@link #getHeight()}. -1 for default */
		void setSize(int width, int height);

		/** fills entire screen ({@link #getWidth()} x {@link #getHeight()}) */
		void fillScreen(char a_letter);

		/** @return foreground color */
		int getFcolor();

		/** @return background color */
		int getBcolor();

		/** use CLI::COLOR values for foreground */
		inline void setColor(int foreground){setColor(foreground, getBcolor());}

		/** @return true if initialized by {@link #init()} */
		bool isInitialized();

		/**
		* @return 0 for no output buffer (using platform),
		* 1 to manage a buffer internally (required for non-standard i/o),
		* 2 to keep track of what is printed, reducing redraws where possible
		*/
		int getBufferCount();

		/**
		* @param a_numberOfBuffers 0 for no output buffer (use platform),
		* 1 to manage a buffer internally (required for non-standard i/o),
		* 2 to keep track of what is printed, reducing redraws where possible
		*/
		void setBufferCount(int a_numberOfBuffers);

		/** resets color values to default */
		void resetColor();

		/** @return the output buffer (may be null if using default platform) */
		CLIBuffer * getOutputBuffer() { return m_buff; }

		/** @return how many bytes this CLI is storing temporarily */
		int getInputBufferCount() {return m_inputBufferCount;}

		/** @param a_count how many bytes this CLI is storing temporarily */
		void setInputBufferCount(int a_count){m_inputBufferCount=a_count;}

		/** temporary input storage. note: not const, so modifiable */
		int* getInputBuffer(){return m_inputBuffer;}

		/** redraw the buffer */
		void Draw();
	};

	/**
	* @return the current CLI environment object. Useful if there
	* is a multi-tabbed command-line interface, or a game with multiple CLI
	* accessible.
	*/
	BufferManager * getCLI();

	/**
	* Sets the current CLI environment object to another one. Useful if there
	* is a multi-tabbed command-line interface, or an app with multiple CLI
	* accessible.
	*
	* Be sure to delete the current CLI before setting a new one!
	*/
	void setCLI(BufferManager * a_CLI);

	/** call before using CLI functions, {@link #release()} expected later */
	void init();

	/** call after done using CLI functions, after {@link #init()} */
	void release();

	/** move the cursor to the given location in the console */
	void move(int row, int col);
	inline void move(CLI::Coord pos) { move(pos.y, pos.x); }

	/** @return key press from input buffer, -1 none. {@link kbhit()} */
	int getchar();

	/** @return key press from input buffer, waits if none. {@link kbhit()} */
	int getcharBlocking();

	inline int getch() { return getcharBlocking(); }

	/** @param a_ms waits this many ms, otherwise: {@link #getcharBlocking()} */
	int getcharBlocking(int a_ms);

	Coord GetCursorLocation();

	/** refresh the screen */
	void refresh();

	/** as stdio.h's printf, built for this CLI environment */
	int printf(const char *fmt, ...);

	/** use CLI::COLOR values for foreground and background */
	void setColor(int foreground, int background);

	// /** pause the CPU for the given number of milliseconds */
	// void sleep(int ms);

	// /** how many milliseconds since {@link #init()} */
	// long long upTimeMS();

	/** @return true if at least one byte is in the input buffer */
	bool kbhit();

	/** put a single character at the cursor (console output), advancing it */
	void putchar(int a_letter);

	/** @return number of columns. may be set with {@link #setSize(int,int)} */
	int getWidth();

	/** @return number of rows. may be set with {@link #setSize(int,int)} */
	int getHeight();

	/** force {@link #getWidth()} and {@link #getHeight()}. -1 for default */
	void setSize(int width, int height);

	/** fills entire screen ({@link #getWidth()} x {@link #getHeight()}) */
	void fillScreen(char a_letter);

	/** @return foreground color */
	int getFcolor();

	/** @return background color */
	int getBcolor();

	/** use CLI::COLOR values for foreground */
	inline void setColor(int foreground){setColor(foreground, getBcolor());}

	/** @return true if initialized by {@link #init()} */
	bool isInitialized();

	/** @return if using double buffering */
	bool isDoubleBuffered();

	/** @param a_doubleBuffered whether or not to enable double buffering */
	void setDoubleBuffered(bool a_doubleBuffered);

	/** resets color values to default */
	void resetColor();

	/** color constants, used by {@link #setColor(int, int)} */
	namespace COLOR
	{
		const int
			DEFAULT = -1,
			BLACK = 0,
#ifdef _WIN32
			RED = (1<<2),
			GREEN = (1<<1),
			BLUE = (1<<0),
#else
			RED = (1<<0),
			GREEN = (1<<1),
			BLUE = (1<<2),
#endif
			INTENSITY = (1<<3);
		const int
			YELLOW = RED|GREEN,
			CYAN = GREEN|BLUE,
			MAGENTA = BLUE|RED,
			LIGHT_GRAY = RED|GREEN|BLUE,
			DARK_GRAY = INTENSITY,
			WHITE = RED|GREEN|BLUE|INTENSITY;
#define BRIGHT_(c)	BRIGHT_##c = c|INTENSITY
		const int
			BRIGHT_(RED),
			BRIGHT_(YELLOW),
			BRIGHT_(GREEN),
			BRIGHT_(CYAN),
			BRIGHT_(BLUE),
			BRIGHT_(MAGENTA);
#undef BRIGHT_
		/**
		 * pass to setColor function. Only works in linux/unix TTY
		 * @param R, G, B values must be between 0 and 5 (inclusive)
		 */
		inline int RGB8bit(int R,int G,int B){return (16+(B+(G*6)+(R*36)));}
		/**
		 * pass to setColor function. Only works in linux/unix
		 * @param gray must be between 0 and 23 (inclusive)
		 */
		inline int GRAYSCALE24(int gray){return (232+gray);}
	}

	/** key codes that can be returned from getchar */
	namespace KEY
	{
		const int
		BACKSPACE = 127,
		UP = 0402,
		LEFT = 0403,
		DOWN = 0404,
		RIGHT = 0405,
		HOME = 0406,
		F0 = 0410,
#define KEY_F_(n)	F##n = F0+n
#define KEY_F_MASS(a,b,c,d)	KEY_F_(a), KEY_F_(b), KEY_F_(c), KEY_F_(d)
		KEY_F_MASS(1,2,3,4),
		KEY_F_MASS(5,6,7,8),
		KEY_F_MASS(9,10,11,12),
#undef KEY_F_MASS
#undef KEY_F_
		DELETEKEY = 0512,	// not named DELETE because WinNT defines DELETE already as a global macro...
		INSERT = 0513,
		END = 0520,
		PAGE_DOWN = 0522,
		PAGE_UP = 0523,
		ESCAPE = 033;
	}
}
