#pragma once

#include "vector2.h"

namespace CLI
{
	/**
	 * manages ouput written into a 2D (C)ommand (L)ine (I)nterface console
	 */
	class CLIBuffer
	{
	public:
		/** use a bunch of already written code to describe a 2D vector */
		typedef Vector2 Coord;
		/** rename to output cell or something */
		struct CLIOutputType
		{
			char letter, fcolor, bcolor, flag;
			inline void set(char letter, unsigned char fcolor, unsigned char bcolor)
			{
				this->letter = letter;
				this->fcolor = fcolor;
				this->bcolor = bcolor;
				this->flag = 0;
			}
			bool operator==(CLIOutputType & o)
			{
				return letter == o.letter
					&& fcolor == o.fcolor
					&& bcolor == o.bcolor;
			}
			bool operator!=(CLIOutputType & o)
			{
				return !operator==(o);
			}
		};
	private:
		static const char WHITESPACE = ' ';
		static const int TABSIZE = 4;
		/** where the cursor is */
		Coord m_cursor;
		/** how big the buffer is */
		Coord m_size;
		/** the actual buffer */
		CLIOutputType * m_mainBuffer;
		/** what was printed last, a draw optimization to reduce redraws */
		CLIOutputType * m_historyBuffer;
		/** if characters go to the next line when over the right boundary */
		bool m_wrap;

		inline int correctIndex(Coord c){return c.x+c.y*m_size.x;}
		inline int correctIndex(){return correctIndex(m_cursor);}

	public:
		bool changedSinceLastDraw(Coord c)
		{
			if(!m_historyBuffer)return true;
			int i = correctIndex(c);
			return m_mainBuffer[i] != m_historyBuffer[i];
		}

		/** @return maximum input (exclusive) values for {@link #getAt(Coord)} */
		Coord getSize(){return m_size;}

		CLIOutputType * getAt(Coord c)
		{
			if(c.x < 0 || c.x >= m_size.x || c.y < 0 || c.y >= m_size.y)
				return 0;
			else
				return &m_mainBuffer[correctIndex(c)];
		}

		void setAt(Coord c, char letter, unsigned int fcolor, unsigned int bcolor)
		{
			getAt(c)->set(letter, fcolor, bcolor);
		}
		/**
		 * @param a_numberOfBuffers 0 for no buffering (use platform buffer),
		 * 1 to manage a buffer internally (required for non-standard i/o)
		 * 2 to keep track of what is printed, reducing redraws where possible
		 */
		void reinitializeDoubleBuffer(int a_numberOfBuffers, Coord a_size);

		int getNumberOfBuffers() {
			return (m_historyBuffer != 0)?2:1;
		}

		bool internalDoubleBufferConsistencyEnforcement(Coord a_size)
		{
			if(a_size != m_size){
				reinitializeDoubleBuffer(getNumberOfBuffers(), a_size);
				return true;
			}
			return false;
		}

		/** @param c DO NOT put in \0, \a, \t, \b, \n, \r, or -1 */
		void putcharDirect(char c, unsigned char fcolor, unsigned char bcolor)
		{
			if(m_cursor.y >= 0 && m_cursor.y < m_size.y)
			{
				CLIOutputType * cell;
				if(m_cursor.x >= 0 && m_cursor.x < m_size.x)
				{
					cell = getAt(m_cursor);
					if(c != cell->letter
					|| cell->fcolor != (fcolor) || cell->bcolor != (bcolor))
					{
						setAt(m_cursor, c, fcolor, bcolor);
					}
				}
				++m_cursor.x;
				if(m_wrap && m_cursor.x >= m_size.x)
				{
					m_cursor.x = 0;
					++m_cursor.y;
				}
			}
		}
		void printTotallyEmptyChar();

		int tabWidthFrom(int column, int tabSize)
		{
			return (tabSize+1)-((column+(tabSize+1))%tabSize);
		}

		void move(int row, int col){m_cursor.set(col, row);}

		void putchar(char c, unsigned char fcolor, unsigned char bcolor)
		{
			switch(c){
			case '\b':
				m_cursor.x--;
				if(m_cursor.x < 0){
					m_cursor.x = 0;
				}
				break;
			case '\t':
				{
					int col = m_cursor.x;
					int tabSize = tabWidthFrom(col, TABSIZE);
					for(int i = 0; i < tabSize && col < m_size.x; ++i, ++col)
						this->putcharDirect(WHITESPACE,fcolor,bcolor);
				}break;
			case '\n':
				{
					int col = m_cursor.x;
					int row = m_cursor.y;
					for(; col < m_size.x; ++col)
						printTotallyEmptyChar();
					m_cursor.y = row+1;
					m_cursor.x = 0;
				}break;
			case '\r':
				m_cursor.x = 0;
				break;
			default:
				this->putcharDirect(c,fcolor,bcolor);
				break;
			}
		}

		void draw();

		void release();

		CLIBuffer():m_mainBuffer(0),m_historyBuffer(0),m_wrap(true){}

		~CLIBuffer()
		{
			release();
		}
	};
}
