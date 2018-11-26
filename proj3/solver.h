#pragma once
#include <iostream>
#include <algorithm>
#include <cmath>
#include "board2x3.h"
#include <numeric>
#include "action.h"

#define ROW 2
#define COLUMN 3
#define MAX_INDEX 6
#define TABLE_SIZE MAX_INDEX*MAX_INDEX*MAX_INDEX*MAX_INDEX*MAX_INDEX*MAX_INDEX

class state_type {
public:
	enum type : char {
		before  = 'b',
		after   = 'a',
		illegal = 'i'
	};

public:
	state_type() : t(illegal) {}
	state_type(const state_type& st) = default;
	state_type(state_type::type code) : t(code) {}

	friend std::istream& operator >>(std::istream& in, state_type& type) {
		std::string s;
		if (in >> s) type.t = static_cast<state_type::type>((s + " ").front());
		return in;
	}

	friend std::ostream& operator <<(std::ostream& out, const state_type& type) {
		return out << char(type.t);
	}

	bool is_before()  const { return t == before; }
	bool is_after()   const { return t == after; }
	bool is_illegal() const { return t == illegal; }

private:
	type t;
};

class state_hint {
public:
	state_hint(const board& state) : state(const_cast<board&>(state)) {}

	char type() const { return state.info() ? state.info() + '0' : 'x'; }
	operator board::cell() const { return state.info(); }

public:
	friend std::istream& operator >>(std::istream& in, state_hint& hint) {
		while (in.peek() != '+' && in.good()) in.ignore(1);
		char v; in.ignore(1) >> v;
		hint.state.info(v != 'x' ? v - '0' : 0);
		return in;
	}
	friend std::ostream& operator <<(std::ostream& out, const state_hint& hint) {
		return out << "+" << hint.type();
	}

private:
	board& state;
};


class solver {
public:
	typedef float value_t;

public:
	class answer {
	public:
		answer(value_t min = 0.0/0.0, value_t avg = 0.0/0.0, value_t max = 0.0/0.0) : min(min), avg(avg), max(max) {}
	    friend std::ostream& operator <<(std::ostream& out, const answer& ans) {
	    	return !std::isnan(ans.avg) ? (out << ans.min << " " << ans.avg << " " << ans.max) : (out << "-1") << std::endl;
		}
	public:
		const value_t min, avg, max;
	};

	class tile_bag{
	public:
		tile_bag() : bag({false,false,false}) {}
		void set(int num) {
			bag[num] = true;
			if(bag[0] && bag[1] && bag[2] ){
				bag = {false,false,false};
			}
			return;
		}
		void unset(int num) {
			
			if(!bag[0] && !bag[1] && !bag[2] ){
				bag = {true, true, true};
			}
			bag[num] = false;
			return;
		}
		bool get(int num) {
			if(bag[num])
				return true;
			else
				return false;
		}
		void reset(){
			bag = {false,false,false};
		}
	private:
		std::array<bool, 3> bag;
	};
	/*class bag
	{
	public:
		bag(){ status = 0; };
		update_status(int tile){

		}
		~bag();
		
	public:
		int status
	};*/

public:
	solver(const std::string& args) {
		// TODO: explore the tree and save the result
		for(auto it = before_state.begin(); it != before_state.end(); ++it )
			*it = -1.0;
		for(auto it = after_state.begin(); it != after_state.end(); ++it )
			*it = -1.0;

		for(int pos = 0; pos < ROW*COLUMN ; ++pos){
			for(int tile = 1; tile <4 ; ++tile){
				board2x3 board;
				action::place(pos, tile).apply(board);
				for(int h = 1; h <4 ;++h){
					if(h!=tile)
						get_before_expect(board, h);
				}				
			}
		}

		std::cout << "solver is initialized." << std::endl << std::endl;

//		std::cout << "feel free to display some messages..." << std::endl;
	}

	answer solve(const board& cur_board, state_type type = state_type::before) {
		// TODO: find the answer in the lookup table and return it
		//       do NOT recalculate the tree at here

		// to fetch the hint (if type == state_type::after, hint will be 0)
		board::cell hint = state_hint(state);

		// for a legal state, return its three values.
//		return { min, avg, max };

		//check if it is illegal board
		for(int i = 0; i < 2*3; ++i){
			if(cur_board(i) >= MAX_INDEX || cur_board(i) < 0){
				std::cout << " illegal board\n";
				return answer(-1, -1, -1);//{};
			}
		}

		/*if(type.is_before() && is_legal_before_state(cur_board)){
			return get_before_expect(cur_board);
		}
		else if(type.is_after() && is_legal_after_state(cur_board)){
			return get_after_expect(cur_board);
		}*/

		if(type.is_before())
			return before_state[board(0)][board(1)][board(2)][board(3)][board(4)][board(5)][hint];
		else{
			for(int i = 0 ;i < 4; ++i){ //valid answer according to last action
				answer &ans = after_state[board(0)][board(1)][board(2)][board(3)][board(4)][board(5)][hint][i];
				if(ans.avg > -1)
					return ans;
			}
			return answer(-1, -1, -1);
		}

// for a legal state, return its three values.
//		return { min, avg, max };
// for an illegal state, simply return {}
//		return {};
	}
private:

	//answer get_before_expect(board2x3 board, int hint){
	void get_before_expect(board2x3 board, int hint){
		answer &expect = get_value(board, hint, 0, true); // get the table reference //before_state[board(0)][board(1)][board(2)][board(3)][board(4)][board(5)][hint];

		//value_t ans_max = sum = 0.0;
		//value_t ans_min = 1000000;
		//value_t ans = -1;
		answer tmp(-1, -1, -1);
		answer ans(-1, -1, -1);
		bool terminal = true;

		for(int op: {0, 1, 2, 3}){ //up right down left
			board2x3 b = board;
			int reward = b.slide(op);

			if(reward != -1){
				terminal = false;

				get_after_expect(b, hint, op);
				tmp = after_state[board(0)][board(1)][board(2)][board(3)][board(4)][board(5)][hint][op];

				if(tmp.avg > ans.avg)
					ans = tmp;
				/*sum += tmp;
				if(tmp.max > ans_max)
					ans_max = tmp.max;
				if(tmp.min < ans_min)
					ans_min = tmp.min;
				*/
			}
		}

		if(terminal){
			temp_expect = calculate_expect(board, true);
			expect = answer(temp_expect, temp_expect, temp_expect);
		}

		else
			expect = ans;

		return;
	}

	//answer get_after_expect(board2x3 board, int hint, int last_op){
	void get_after_expect(board2x3 board, int hint, int last_op){
	    answer &expect = get_value(board, hint, last_op, false);

		// decide what tile can be put (tile bag)

		//according to the last action decide where can put the tile
		std::array<int,4> slide_space;
		switch (last_op) {
			case 0: slide_space = {0, 1, 2}; //slide up 0
				break;
			case 1: slide_space = {0, 3}; //slide right 1
				break;
			case 2: slide_space = {0, 1, 2}; //slide down 2
				break;
			case 3: slide_space = {2, 5}; //slide left 3
				break;
			default:
				std::cout << "no such action! " << (last_op & 0b11)<< "\n";
		}
						
		//std::shuffle(slide_space.begin(), slide_space.end(), engine);

		value_t ans_max = sum = 0.0;
		value_t ans_min = 1000000;
		int count  = 0;

		bag.set(tile-1);//set hint tile to be used
		for(auto pos = slide_space.begin(); pos != slide_space.end(); ++pos){ //place the hint tile


			int result = action::place(pos, hint).apply(board);
			if(result != -1){
				for(int new_hint: {1, 2, 3}){//new hint
					if(bag.get(new_hint-1))
						continue;
					else { //grab new tile
						get_before_expect(board, new_hint);
						tmp = before_state[board(0)][board(1)][board(2)][board(3)][board(4)][board(5)][new_hint];

						if(tmp.avg > -1){
							ans.max = tmp.max > ans.max ? tmp.max : ans.max;
							ans.min = tmp.min < ans.min ? tmp.min : ams.min;
							sum += tmp.avg;
							++count;
						}
					}
						
				} 
					
				++count;
			}
		}
		bag.unset(tile-1);

		expect = answer(ans_min, sum/float(count), ans_max);

		return;
	}

	float calculate_expect(board2x3 board/*, bool terminal*/){

		/*if(terminal){
			float sum = 0;
			int tile;
			for(int i= 0; i < 6 ; ++i){
				tile = board(i) > 3? ((1 << (board(i)-3))*3) : board(i);
				sum += pow( 3 ,log2(tile/3) + 1);
			}

			return sum;
		}*/

		//else{
			float sum = 0;
			int tile;
			for(int i= 0; i < 6 ; ++i){				
				if(tile >= 3){
					tile = board(i) > 3? ((1 << (board(i)-3))*3) : board(i);
					sum += pow( 3 ,log2(tile/3) + 1);
				}
			}

			return sum;
		//}
	}

	&answer get_value(board2x3 board, int hint, int op, bool state_type){
		if(state_type)
			return before_state[board(0)][board(1)][board(2)][board(3)][board(4)][board(5)][hint];
		else
			return after_state[board(0)][board(1)][board(2)][board(3)][board(4)][board(5)][hint][op];
	}

private:
	// TODO: place your transposition table here
	//std::vector<answer> before_state;
	//std::vector<answer> after_state;
	answer before_state[MAX_INDEX][MAX_INDEX][MAX_INDEX][MAX_INDEX][MAX_INDEX][MAX_INDEX][3]; //[board][hint]
	answer after_state[MAX_INDEX][MAX_INDEX][MAX_INDEX][MAX_INDEX][MAX_INDEX][MAX_INDEX][3][4]; //[board][hint][last action]
	tile_bag bag;
};


