#pragma once

namespace CLI
{
	struct Coord {
		int x, y;
		Coord(int x, int y):x(x),y(y){}
		Coord():x(0),y(0){}
		bool operator==(Coord & other) { return x == other.x && y == other.y; }
		bool operator!=(Coord & other) { return !operator==(other); }
		void Set(int x, int y){this->x=x;this->y=y;}
		void Set(const Coord& c){this->x=c.x;this->y=c.y;}
	};

	/** manages ouput written into a 2D (C)ommand (L)ine (I)nterface console */
	class CLIBuffer {
	public:
		/** the 'pixel' unit */
		struct Pel {
			static const int FLAG_NOCOLOR = 1;
			char letter, fcolor, bcolor; unsigned char flag;
			inline void Set(char letter, unsigned char fcolor, unsigned char bcolor) {
				this->letter = letter; this->fcolor = fcolor; this->bcolor = bcolor; this->flag = 0;
			}
			inline void Set(char letter, unsigned char fcolor, unsigned char bcolor, unsigned char flag) {
				this->letter = letter; this->fcolor = fcolor; this->bcolor = bcolor; this->flag = flag;
			}
			bool operator==(Pel & o) { return letter == o.letter && fcolor == o.fcolor && bcolor == o.bcolor; }
			bool operator!=(Pel & o) { return !operator==(o); }
		};
	private:
		static const char WHITESPACE = ' ';
		static const int TABSIZE = 4;
		/** where the cursor is */
		Coord m_cursor;
		/** how big the buffer is */
		Coord m_size;
		/** the actual buffer */
		Pel * m_mainBuffer;
		/** what was printed last, a draw optimization to reduce redraws */
		Pel * m_historyBuffer;
		/** if characters go to the next line when over the right boundary */
		bool m_wrap;

		inline int CorrectIndex(Coord c){return c.x+c.y*m_size.x;}
		inline int CorrectIndex(){return CorrectIndex(m_cursor);}

	public:
		Coord getCursorPosition() { return m_cursor; }

		bool IsChangedSinceLastDraw(Coord c) {
			if(!m_historyBuffer) return true;
			int i = CorrectIndex(c);
			return m_mainBuffer[i] != m_historyBuffer[i];
		}

		/** @return maximum input (exclusive) values for {@link #getAt(CLI::Coord)} */
		Coord GetSize() {return m_size;}

		/** @return if given coordinate is out of bounds, return NULL */
		Pel * GetAt(Coord c) {
			return (c.x < 0 || c.x >= m_size.x || c.y < 0 || c.y >= m_size.y)
				? 0 : &m_mainBuffer[CorrectIndex(c)];
		}

		void SetAt(CLI::Coord c, char letter, unsigned int fcolor, unsigned int bcolor) {
			Pel* p = GetAt(c);
			if(p){p->Set(letter, fcolor, bcolor);}
		}
		/**
		 * @param a_numberOfBuffers 0 for no buffering (use platform buffer),
		 * 1 to manage a buffer internally (required for non-standard i/o)
		 * 2 to keep track of what is printed, reducing redraws where possible
		 */
		void ReinitializeDoubleBuffer(int a_numberOfBuffers, Coord a_size);

		int GetNumberOfBuffers() { return (m_historyBuffer != 0)?2:1; }

		bool InternalDoubleBufferConsistencyEnforcement(Coord a_size) {
			if(a_size != m_size) {
				ReinitializeDoubleBuffer(GetNumberOfBuffers(), a_size);
				return true;
			}
			return false;
		}

		/** @param c DO NOT put in \0, \a, \t, \b, \n, \r, or -1 */
		void putcharDirect(char c, unsigned char fcolor, unsigned char bcolor) {
			//int i = CorrectIndex(m_cursor);
			if(m_cursor.y >= 0 && m_cursor.y < m_size.y) {
				if(m_cursor.x >= 0 && m_cursor.x < m_size.x) {
					//m_mainBuffer[i].Set(c, fcolor, bcolor);
					SetAt(m_cursor, c, fcolor, bcolor);
				}
				++m_cursor.x;
				if(m_wrap && m_cursor.x >= m_size.x) {
					m_cursor.x = 0;
					++m_cursor.y;
				}
			}
		}
		void PrintTotallyEmptyChar();

		int TabWidthFrom(int column, int tabSize) {
			return (tabSize+1)-((column+(tabSize+1))%tabSize);
		}

		void Move(int row, int col){m_cursor.Set(col, row);}

		void Move(const Coord& location){m_cursor.Set(location);}

		void putchar(char c, unsigned char fcolor, unsigned char bcolor) {
			switch(c){
			case '\b':
				m_cursor.x--;
				if(m_cursor.x < 0){
					m_cursor.x = 0;
				}
				break;
			case '\t':{
				int col = m_cursor.x;
				int tabSize = TabWidthFrom(col, TABSIZE);
				for(int i = 0; i < tabSize && col < m_size.x; ++i, ++col)
					this->putcharDirect(WHITESPACE,fcolor,bcolor);
			}	break;
			case '\n': {
				int col = m_cursor.x;
				int row = m_cursor.y;
				for(; col < m_size.x; ++col)
					PrintTotallyEmptyChar();
				m_cursor.y = row+1;
				m_cursor.x = 0;
			}	break;
			case '\r':
				m_cursor.x = 0;
				break;
			default:
				this->putcharDirect(c,fcolor,bcolor);
				break;
			}
		}

		void Draw();

		void Release();

		CLIBuffer():m_mainBuffer(0),m_historyBuffer(0),m_wrap(true){}

		~CLIBuffer() { Release(); }
	};
}
