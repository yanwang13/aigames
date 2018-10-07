#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include "board.h"
#include "action.h"
#include "episode.h"
class episode; // forward declaration

class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('='));
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
		}
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	//virtual action take_action(const board& b) { return action(); }
	virtual action take_action(const episode& b) { return action(); }
	virtual bool check_for_win(const board& b) { return false; }

public:
	virtual std::string property(const std::string& key) const { return meta.at(key); }
	virtual void notify(const std::string& msg) { meta[msg.substr(0, msg.find('='))] = { msg.substr(msg.find('=') + 1) }; }
	virtual std::string name() const { return property("name"); }
	virtual std::string role() const { return property("role"); }

protected:
	typedef std::string key;
	struct value {
		std::string value;
		operator std::string() const { return value; }
		template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
		operator numeric() const { return numeric(std::stod(value)); }
	};
	std::map<key, value> meta;
};

class random_agent : public agent {
public:
	random_agent(const std::string& args = "") : agent(args) {
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
	}
	virtual ~random_agent() {}

protected:
	std::default_random_engine engine;
};

/**
 * random environment
 * add a new random tile to an empty cell
 * 2-tile: 90%
 * 4-tile: 10%
 */
 class tile_bag{
public:
	tile_bag() : bag({false,false,false}) {}
	void set(int num) {
		bag[num] = true;
		if(bag[0] && bag[1] && bag[2] )
			bag = {false,false,false};
		return;
	}
	bool get(int num) {
		if(bag[num])
			return true;
		else
			return false;
	}
private:
	std::array<bool, 3> bag;
};

class rndenv : public random_agent {
public:
	rndenv(const std::string& args = "") : random_agent("name=random role=environment " + args),
		space({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }), popup(1, 3) {}
		//slide_up({12, 13, 14, 15}), slide_down({0,1,2,3}), slide_left({3,7,11,15}), slide_right({0,4,8,12}), popup(1, 3) {}

	//virtual action take_action(const board& after) {
	virtual action take_action(const episode& game) {
		const board& after = game.state();
		
		if(game.ep_moves.size()<=9) { //initial 9 tiles
			std::shuffle(space.begin(), space.end(), engine);
			for (int pos : space) {
				if (after(pos) != 0) continue;
				board::cell tile;
				while(true){
					tile = popup(engine);
					if (env_tile_bag.get(tile-1)) continue; //see whether the tile has been used
					else break; // grad a new tile
				}
				env_tile_bag.set(tile-1); //take the tile out of the bag
				return action::place(pos, tile);
			}
			return action();
		} else {
			std::array<int,4> slide_space;
			//const char* cc = game.ep_moves.back().code.code;
			//const char* opc = "URDL";
			//unsigned oper = std::find(opc, opc + 4, *(++cc)) - opc;
			switch (game.ep_moves.back().code.code & 0b11) {
				case 0: slide_space = {12, 13, 14, 15}; //slide up 0
					break;
				case 1: slide_space = {0, 4, 8, 12}; //slide right 1
					break;
				case 2: slide_space = {0, 1, 2, 3}; //slide down 2
					break;
				case 3: slide_space = {3, 7, 11, 15}; //slide left 3
					break;
				default:
					std::cout << "no such action! " << (game.ep_moves.back().code.code & 0b11)<< "\n";
			}
						
			std::shuffle(slide_space.begin(), slide_space.end(), engine);
			for (int pos : slide_space) {
				if (after(pos) != 0) continue;
				board::cell tile;
				while(true){
					tile = popup(engine);
					if (env_tile_bag.get(tile-1)) continue; //see whether the tile has been used
					else break; // grad a new tile
				}
				env_tile_bag.set(tile-1); //take the tile out of the bag
				return action::place(pos, tile);
			}
			return action();
			
		}
	}

private:
	std::array<int, 16> space;
	//std::array<int, 4> slide_up, slide_down, slide_left, slide_right;
	std::uniform_int_distribution<int> popup;
	tile_bag env_tile_bag;
};

/**
 * dummy player
 * select a legal action randomly
 */
class player : public random_agent {
public:
	player(const std::string& args = "") : random_agent("name=dummy role=player " + args),
		opcode({ 0, 1, 2, 3 }) {}

	//virtual action take_action(const board& before) {
	virtual action take_action(const episode& game) {
		const board& before = game.state();
		std::shuffle(opcode.begin(), opcode.end(), engine);
		for (int op : opcode) {
			board::reward reward = board(before).slide(op);
			if (reward != -1) return action::slide(op);
		}
		return action();
	}

private:
	std::array<int, 4> opcode;
};
