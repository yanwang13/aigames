#pragma once
#include <iostream>
#include <algorithm>
#include <cmath>
#include "board2x3.h"
#include <numeric>
#include "action.h"

#define ROW 2
#define COLUMN 3
#define MAX_INDEX 10
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
	state_hint(const board2x3& state) : state(const_cast<board2x3&>(state)) {}

	char type() const { return state.info() ? state.info() + '0' : 'x'; }
	operator board2x3::cell() const { return state.info(); }

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
	board2x3& state;
};


class solver {
public:
	//typedef float value_t;
	typedef double value_t;

public:
	class answer {
	public:
		//answer() : min(-1.0/1.0), avg(-1.0/1.0), max(-1.0/1.0) {}
		answer() : min(0.0/0.0), avg(0.0/0.0), max(0.0/0.0) {} //isnan
		answer(value_t min, value_t avg, value_t max) : min(min), avg(avg), max(max) {}
		//answer(value_t min = -1.0/-1.0, value_t avg = -1.0/-1.0, value_t max = -1.0/-1.0) : min(min), avg(avg), max(max) {}
	    friend std::ostream& operator <<(std::ostream& out, const answer& ans) {
	    	return !std::isnan(ans.avg) ? (out << ans.min << " " << ans.avg << " " << ans.max) : (out << "-1"); //<< std::endl;
		}
	public:
		//const value_t min, avg, max;
		value_t min, avg, max;
	};

	class Transposition_Table
	{
	public:
		Transposition_Table(bool type):type(type){
			if(type) //before state
				table = new answer[MAX_INDEX*MAX_INDEX*MAX_INDEX*MAX_INDEX*MAX_INDEX*MAX_INDEX*3];
			else
				table = new answer[MAX_INDEX*MAX_INDEX*MAX_INDEX*MAX_INDEX*MAX_INDEX*MAX_INDEX*3*4];
		}
		~Transposition_Table(){
			delete []  table;
		}

		/*answer* get_value(board2x3 board, int hint, int op, bool type){
			long index = 0;
			for(int i = 1; i < 6 ; ++i)
				index = board(i) + (MAX_INDEX*index);

			if(type)
				return &table[ hint + index*MAX_INDEX ];
			else
				return &table[ op + (hint + index*MAX_INDEX )*3];
		}*/

		answer& get_value(board2x3 board, int hint, int op, bool type){
			int index = 0;
			for(int i = 0; i < 6 ; ++i)
				index = board(i) + (MAX_INDEX*index);

			if(type)
				return table[ hint-1 + index*3 ];
			else
				return table[ op + (hint-1 + index*3 )*4];
		}

	private:
		answer *table;
		bool type;	
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

private:
	/*answer& get_value(board2x3 board, int hint, int op, bool type){
		if(type)
			return before_state[board(0)][board(1)][board(2)][board(3)][board(4)][board(5)][hint-1];
		else
			return after_state[board(0)][board(1)][board(2)][board(3)][board(4)][board(5)][hint-1][op];
	}*/

	float calculate_expect(board2x3 board){

		float sum = 0;
		int tile = 0;
		for(int i= 0; i < 6 ; ++i){	
			tile = board(i);			
			if(tile >= 3){
				//std::cout << "tile: " << tile << std::endl;
				sum += pow( 3 ,tile-2);
			}
		}

		return sum;
	}

	/*void isomorphic_value(board2x3 board, int hint, int op, bool type, answer ans){
		//board2x3 right_hor = board;
		//board2x3 down_ver = board;
		//board2x3 hor_ver = board;

		if(type){
			board2x3 right_hor = board;
			board2x3 down_ver = board;

			right_hor.reflect_horizontal();
			answer& tmp_ans = before_state.get_value(right_hor, hint, op, type);
			tmp_ans = ans;

			down_ver.reflect_vertical();
			tmp_ans = before_state.get_value(down_ver, hint, op, type);
			tmp_ans = ans;

			right_hor.reflect_vertical();
			tmp_ans = before_state.get_value(right_hor, hint, op, type);
			tmp_ans = ans;
		}

		else{
			board2x3 right_hor = board;
			board2x3 down_ver = board;

			right_hor.reflect_horizontal();
			answer& tmp_ans = after_state.get_value(right_hor, hint, op, type);
			tmp_ans = ans;

			down_ver.reflect_vertical();
			tmp_ans = after_state.get_value(down_ver, hint, op, type);
			tmp_ans = ans;

			right_hor.reflect_vertical();
			tmp_ans = after_state.get_value(right_hor, hint, op, type);
			tmp_ans = ans;
		}

		return;
	}*/
	//void get_before_expect(board2x3 board, int hint){
	answer get_before_expect(board2x3 board, int hint){
		//answer& expect = get_value(board, hint, 0, true); // get the table reference //before_state[board(0)][board(1)][board(2)][board(3)][board(4)][board(5)][hint];
		//answer* expect = before_state.get_value(board, hint, 0, true);
		answer& expect = before_state.get_value(board, hint, 0, true);
		if(!std::isnan(expect.avg)) return expect;
		//std::cout << "get_before_expect: " << expect << std::endl;
		//std::cout << board;
		//std::cout << "hint: " << hint << std::endl << std::endl;

		answer tmp;
		answer ans; //initalize tmp ans
		ans.avg = -1;

		bool terminal = true;

		for(int op: {0, 1, 2, 3}){ //up right down left
			board2x3 b = board;
			//std::cout << "before slide > " << std::endl;
			int reward = b.slide(op);

			if(reward != -1){
				terminal = false;				
				//action::slide(op).apply(b);
				//std::cout << "after slide op: " << op << std::endl;
				//std::cout << b;
				//std::cout << "reward=  " << reward <<std::endl;

				//get_after_expect(b, hint, op);
				//tmp = after_state.get_value(b, hint, op, false);
				tmp = get_after_expect(b, hint, op);
				//std::cout << "after_state op("<< op << ") tmp: " << tmp << std::endl;
				//std::cout << "current ans >> " << ans << std::endl;
				if(!std::isnan(tmp.avg) && tmp.avg > ans.avg){//optimal action based on the average value
					//std::cout << "tmp is not nan update ans";
					ans = tmp;
					//std::cout << " >>  " << ans << std::endl;
				}
			}
		}


		if(terminal){
			//std::cout << "terminal node\n";
			float temp_expect = calculate_expect(board/*, true*/);
			expect = answer(temp_expect, temp_expect, temp_expect);
			//std::cout << "value: " << expect <<std::endl;
		}

		else
			expect = ans.avg==-1 ? answer() : ans;
		//std::cout << "BEFORE STATE RETURN >" << std::endl;
		//std::cout << board;
		//std::cout << "hint: " << hint << std::endl;
		//std::cout << "answer: " << expect <<std::endl << std::endl;
		//if(terminal) system("pause");
		//isomorphic_value(board, hint, 0, true, expect);

		return expect;
	}

	//void get_after_expect(board2x3 board, int hint, int last_op){
	answer get_after_expect(board2x3 board, int hint, int last_op){
	    //answer& expect = get_value(board, hint, last_op, false);
	    //answer* expect = after_state.get_value(board, hint, last_op, false);
	    answer& expect = after_state.get_value(board, hint, last_op, false);
	    if(!std::isnan(expect.avg)) return expect;;
		// decide what tile can be put (tile bag)
	    //std::cout << "get_after_expect: " << expect << std::endl;
		//std::cout << board;
		//std::cout << "hint: " << hint << " /last op: " << last_op << std::endl;

		//according to the last action decide where can put the tile
		const int size = last_op % 2 == 0 ? 3 : 2;
		std::array<int,3> slide_space;
		switch (last_op) {
			case 0: slide_space = {3, 4, 5}; //slide up 0
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

		//value_t ans_max = 0.0;
		value_t sum = 0.0;
		//value_t ans_min = 10000;
		int count  = 0;
		answer tmp;
		answer ans;
		bool valid = false;

		bag.set(hint-1);//set hint tile to be used
		//for(auto pos = slide_space.begin(); pos != slide_space.end(); ++pos){ //place the hint tile
		for(int i = 0; i < size ;++i){

			if(board(slide_space[i])!=0) continue;
			//int result = action::place(pos, hint).apply(board);
			else{ //only calculate those are possible
				valid = true;
				for(int new_hint: {1, 2, 3}){//new hint for the next state
					if(bag.get(new_hint-1)) continue;

					else { //grab new tile
						board2x3 b = board;
						//action::place(slide_space[i], hint).apply(b);
						b.place(slide_space[i], hint);
						//get_before_expect(b, new_hint);
						//tmp = before_state.get_value(b, new_hint, 0, true);
						tmp = get_before_expect(b, new_hint);
						//std::cout << "current ans >> " << ans << std::endl;
						if(!std::isnan(tmp.avg)/*tmp.avg > -1*/){
							//std::cout << "before_state new_hint("<< hint << ") tmp: " << tmp << std::endl;
							if(count==0){
								ans = tmp;
								sum = tmp.avg;
							}
							else{
								ans.max = tmp.max > ans.max ? tmp.max : ans.max;
								ans.min = tmp.min < ans.min ? tmp.min : ans.min;
								sum += tmp.avg;
							}
							//std::cout << "after update >>  " << ans << std::endl;							
							++count;
						}
					}
						
				} 
			}
		}
		bag.unset(hint-1);

		if(count>0)
			expect = answer(ans.min, sum/count, ans.max);
		if(!valid)
			expect = answer(-1, -1, -1);
		//isomorphic_value(board, hint, last_op, false, expect);
		//std::cout << "AFTER STATE RETURN >" << std::endl;
		//std::cout << board;
		//std::cout << "hint: " << hint << "last op: " << last_op << std::endl;
		//std::cout << after_state.get_value(board, hint, last_op, false) << std::endl;
		//std::cout << "answer: " << expect <<std::endl << std::endl;
		return expect;
	}

public:
	solver():before_state(true), after_state(false){

		//answer& expect = after_state.get_value(board2x3(), h, 0, false);
		answer expect;
		value_t sum = 0.0;
		int count  = 0;
		answer tmp;
		answer ans;
		//bool valid = false;

		for(int pos = 0; pos < ROW*COLUMN ; ++pos){
			for(int tile = 1; tile <4 ; ++tile){
				board2x3 board;
				//std::cout << board << std::endl;
				//action::place(pos, tile).apply(board);
				board.place(pos, tile);
				//std::cout << board << std::endl;
				bag.set(tile-1);
				for(int h = 1; h <4 ;++h){

					if(h==tile) continue;

					else{
						
						tmp = get_before_expect(board, h);

						if(!std::isnan(tmp.avg)/*tmp.avg > -1*/){
							//std::cout << "before_state new_hint("<< hint << ") tmp: " << tmp << std::endl;
							if(count==0){
								ans = tmp;
								sum = tmp.avg;
							}
							else{
								ans.max = tmp.max > ans.max ? tmp.max : ans.max;
								ans.min = tmp.min < ans.min ? tmp.min : ans.min;
								sum += tmp.avg;
							}
							//std::cout << "after update >>  " << ans << std::endl;							
							++count;
						}
					}


				}
				bag.unset(tile-1);				
			}
		}

		if(count>0)
			expect = answer(ans.min, sum/count, ans.max);
		//if(!valid)
		//	expect = answer(-1, -1, -1);

		board2x3 board;
		for(int i = 1 ; i < 4 ; ++i){
			//for(int j = 0 ; j < 4 ; ++j){
				answer& result = after_state.get_value(board, i, 0, false);
				result = expect;
			//}
			
		}

		std::cout << "solver is initialized." << std::endl << std::endl;

//		std::cout << "feel free to display some messages..." << std::endl;
	}

	answer solve(const board2x3& board, state_type type = state_type::before) {
		// TODO: find the answer in the lookup table and return it
		//       do NOT recalculate the tree at here

		// to fetch the hint (if type == state_type::after, hint will be 0)
		board2x3::cell hint = state_hint(board);

		// for a legal state, return its three values.
//		return { min, avg, max };
		answer invalid;
		//check if it is illegal board
		int tile;
		for(int i = 0; i < 2*3; ++i){
			tile = board(i);
			if(tile >= MAX_INDEX || tile < 0){
				std::cout << " illegal board\n";
				return invalid;//{};
			}
		}

		//answer &ans;
		if(type.is_before()){
			//ans = before_state.get_value(board, hint, 0, true);
			return before_state.get_value(board, hint, 0, true);
			//return get_before_expect(board, hint);
		}
			//return before_state[board(0)][board(1)][board(2)][board(3)][board(4)][board(5)][hint];	
			//return before_state[board(0)][board(1)][board(2)][board(3)][board(4)][board(5)][hint];
		else{
			for(int i = 0 ;i < 4; ++i){ //valid answer according to last action
				answer& ans = after_state.get_value(board, hint, i, false);
				//answer ans = get_after_expect(board, hint, i);
				if(!std::isnan(ans.avg))
					return ans;
			}
			return invalid;
		}

	}


private:
	//answer before_state[MAX_INDEX][MAX_INDEX][MAX_INDEX][MAX_INDEX][MAX_INDEX][MAX_INDEX][3]; //[board][hint]
	//answer after_state[MAX_INDEX][MAX_INDEX][MAX_INDEX][MAX_INDEX][MAX_INDEX][MAX_INDEX][3][4]; //[board][hint][last action]
	Transposition_Table before_state;
	Transposition_Table after_state;
	tile_bag bag;
};


