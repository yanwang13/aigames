#pragma once
#include <array>
#include <iostream>
#include <iomanip>
#include <cmath>
/**
 * array-based board for threes 2x3
 *
 * index (1-d form):
 *  (0)  (1)  (2)
 *  (3)  (4)  (5)
 *
 */
class board2x3 {
public:
	typedef uint32_t cell;
	//typedef std::array<cell, 4> row;
	//typedef std::array<row, 4> grid;
	typedef std::array<cell, 3> row;
	typedef std::array<row, 2> grid;
	typedef uint64_t data;
	typedef int reward;

public:
	board2x3() : tile(), attr(0) {}
	board2x3(const grid& b, data v = 0) : tile(b), attr(v) {}
	board2x3(const board2x3& b) = default;
	board2x3& operator =(const board2x3& b) = default;

	operator grid&() { return tile; }
	operator const grid&() const { return tile; }
	row& operator [](unsigned i) { return tile[i]; }
	const row& operator [](unsigned i) const { return tile[i]; }
	cell& operator ()(unsigned i) { return tile[i / 3][i % 3]; }
	const cell& operator ()(unsigned i) const { return tile[i / 3][i % 3]; }

	data info() const { return attr; }
	data info(data dat) { data old = attr; attr = dat; return old; }

public:
	bool operator ==(const board2x3& b) const { return tile == b.tile; }
	bool operator < (const board2x3& b) const { return tile <  b.tile; }
	bool operator !=(const board2x3& b) const { return !(*this == b); }
	bool operator > (const board2x3& b) const { return b < *this; }
	bool operator <=(const board2x3& b) const { return !(b < *this); }
	bool operator >=(const board2x3& b) const { return !(*this < b); }

public:

	/**
	 * place a tile (index value) to the specific position (1-d form index)
	 * return 0 if the action is valid, or -1 if not
	 */
	reward place(unsigned pos, cell tile) {
		if (pos >= 6) return -1;
		if (tile != 1 && tile != 2 && tile!=3) return -1;
		operator()(pos) = tile;
		return 0;
	}

	/**
	 * apply an action to the board
	 * return the reward of the action, or -1 if the action is illegal
	 */
	reward slide(unsigned opcode) {
		switch (opcode & 0b11) {
		case 0: return slide_up();
		case 1: return slide_right();
		case 2: return slide_down();
		case 3: return slide_left();
		default: return -1;
		}
	}

	reward slide_left() {
		board2x3 prev = *this;
		reward score = 0;
		for (int r = 0; r < 2; r++) {
			auto& row = tile[r];
			//int top = 0, hold = 0;
			bool combine = true;
			for (int c = 0; c < 2; c++) {
				unsigned base = row[c];
				if (base==0 && row[c+1]!=0) {
					row[c] = row[c+1];
					row[c+1] = 0;
					combine = false;
				} else if ( base==0 && base==row[c+1] && combine ){
					combine = false;
				} else if( base>=3 && base==row[c+1] && combine){
					row[c] = ++base;
					row[c+1] = 0;
					score += (1 << row[c]);
					combine = false;
				} else if( ((base + row[c+1])==3) && combine){
					row[c] = 3;
					row[c+1] = 0;
					score += (1 << row[c]);
					combine = false;
				}
			}
		}
		return (*this != prev) ? score : -1;
	}
	reward slide_right() {
		reflect_horizontal();
		reward score = slide_left();
		reflect_horizontal();
		return score;
	}
	reward slide_up() {
		board2x3 prev = *this;
		reward score = 0;

		for (int c = 0; c < 3; c++) {
			unsigned base = tile[0][c];
			if (base==0 && tile[1][c]!=0) {
				tile[0][c] = tile[1][c];
				tile[1][c] = 0;
			} else if( base>=3 && base==tile[1][c]){
				tile[0][c] = ++base;
				tile[1][c] = 0;
				score += (1 << tile[0][c]);
			} else if( ((base + tile[1][c])==3)){
				tile[0][c] = 3;
				tile[1][c] = 0;
				score += (1 << tile[0][c]);
			}
		}
		return (*this != prev) ? score : -1;
	}
	reward slide_down() {
		reflect_vertical();
		reward score = slide_up();
		reflect_vertical();
		return score;
	}

	void reflect_horizontal() {
		std::swap(tile[0][0], tile[0][2]);
		std::swap(tile[1][0], tile[1][2]);
	}

	void reflect_vertical() {
		//std::swap(tile[0][0], tile[1][0]);
		//std::swap(tile[0][1], tile[1][1]);
		//std::swap(tile[0][2], tile[1][2]);
		std::swap(tile[0], tile[1]);
	}

	/**
	 * rotate the board clockwise by given times
	 */
	/*void rotate(int r = 1) {
		switch (((r % 4) + 4) % 4) {
		default:
		case 0: break;
		case 1: rotate_right(); break;
		case 2: reverse(); break;
		case 3: rotate_left(); break;
		}
	}*/

	//void rotate_right() { transpose(); reflect_horizontal(); } // clockwise
	//void rotate_left() { transpose(); reflect_vertical(); } // counterclockwise
	//void reverse() { reflect_horizontal(); reflect_vertical(); }

public:
	friend std::ostream& operator <<(std::ostream& out, const board2x3& b) {
		/*out << "+------------------------+" << std::endl;
		for (auto& row : b.tile) {
			out << "|" << std::dec;
			for (auto t : row) out << std::setw(6) << t;//((1 << t) & -2u);
			out << "|" << std::endl;
		}
		out << "+------------------------+" << std::endl;*/
		for (auto&  row : b.tile){
			for (auto t : row)
				out << (t > 3 ? ((1<<(t-3))*3) : t ) << " ";
		}
		return out;
	}
	friend std::istream& operator >>(std::istream& in, board2x3& b) {
		for (int i = 0; i < 2*3; i++) {
			while (!std::isdigit(in.peek()) && in.good()) in.ignore(1);
			in >> b(i);
			b(i) = b(i) > 3 ? log2(b(i)/3)+3 : b(i);
		}
		return in;
	}

private:
	grid tile;
	data attr;
};
