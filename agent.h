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
#include "state.h"
#include "weight.h"

//std::ostream& info = std::cout;
//std::ostream& error = std::cerr;
//std::ostream& debug = std::cout;

class episode; // forward declaration
class weight;

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
		if(bag[0] && bag[1] && bag[2] ){
			bag = {false,false,false};
		}
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

class rndenv : public random_agent {
public:
	rndenv(const std::string& args = "") : random_agent("name=random role=environment " + args),
		space({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }), popup(1, 3), env_tile_bag() {}
		//slide_up({12, 13, 14, 15}), slide_down({0,1,2,3}), slide_left({3,7,11,15}), slide_right({0,4,8,12}), popup(1, 3) {}

	//virtual action take_action(const board& after) {
	virtual action take_action(const episode& game) {
		const board& after = game.state();
		if(game.ep_moves.size()<=9) { //initial 9 tiles
			std::shuffle(space.begin(), space.end(), engine);
			board::cell tile;
			
			while(true){
				tile = popup(engine);
				if (env_tile_bag.get(tile-1)) continue; //see whether the tile has been used
				else break; // grad a new tile
			}
			for (int pos : space) {
				if (after(pos) != 0) continue;				
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
			
			board::cell tile;
			while(true){
				tile = popup(engine);
				if (env_tile_bag.get(tile-1)) continue; //see whether the tile has been used
				else break; // grad a new tile
			}
			env_tile_bag.set(tile-1); //take the tile out of the bag
			for (int slide_pos : slide_space) {
				if (after(slide_pos) != 0) continue;
				
				return action::place(slide_pos, tile);
			}
			return action();	
		}
	}
	
	virtual void close_episode(const std::string& flag = "") {
		env_tile_bag.reset();
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


class TD_player : public agent {
public:
	TD_player(const std::string& args = "") : agent(args), alpha(0.0125) {
		//if (meta.find("init") != meta.end()) // pass init=... to initialize the weight
		//	init_weights(meta["init"]);
		init_weights();
		if (meta.find("load") != meta.end()) // pass load=... to load from a specific file
			load_weights(meta["load"]);
		if (meta.find("alpha") != meta.end())
			alpha = float(meta["alpha"]);
	}
	virtual ~TD_player() {
		if (meta.find("save") != meta.end()) // pass save=... to save to a specific file
			save_weights(meta["save"]);
	}

public:
	void add_feature(iso_pattern* patt) {
		net.push_back(*patt);

		info << patt->name() << ", size = " << patt->size();
		size_t usage = patt->size() * sizeof(float);
		if (usage >= (1 << 30)) {
			info << " (" << (usage >> 30) << "GB)";
		} else if (usage >= (1 << 20)) {
			info << " (" << (usage >> 20) << "MB)";
		} else if (usage >= (1 << 10)) {
			info << " (" << (usage >> 10) << "KB)";
		}
		info << std::endl;
	}

	/**
	 * accumulate the total value of given state
	 */
	float evaluation(const board& b) const {
		//debug << "estimate " << std::endl << b;
		float value = 0;
		for (auto& wght : net) {
			value += wght.eval(b);
		}
		return value;
	}

	/**
	 * update the value of given state and return its new value
	 */
	float update(const board& b, float u) {
		//debug << "update " << " (" << u << ")" << std::endl << b;
		//debug << b;
		float u_split = u / net.size();
		float value = 0;
		for (auto& wght : net) {
			value += wght.update(b, u_split);
		}
		return value;
	}

	/**
	 * select a best move of a before state b
	 *
	 * return should be a state whose
	 *  before_state() is b
	 *  after_state() is b's best successor (after state)
	 *  action() is the best action
	 *  reward() is the reward of performing action()
	 *  value() is the estimated value of after_state()
	 *
	 * you may simply return state() if no valid move
	 */
	//action select_best_move(const episode& game) {
	virtual action take_action(const episode& game){
		const board& b = game.state();
		state after[4] = { 0, 1, 2, 3 }; // up, right, down, left
		state* best = after;
		for (state* move = after; move != after + 4; move++) {
			if (move->assign(b)) {
				move->set_value(move->reward() + evaluation(move->after_state()));
				if (move->value() > best->value())
					best = move;
			} else {
				move->set_value(-std::numeric_limits<float>::max());
			}
			//debug << "test " << *move;
		}
		//debug << "best move: " << *best;
		path.push_back(*best);
		if(best->reward()==-1) return action();
		else return action::slide(best->action());
	}
	/*virtual action select_best_move(const episode& game) { //choose the move that cause best value
		const board& before = game.state()
		int best_reward = 0;
		float best_v = MIN_FLOAT;
		for(int op:{0, 1, 2, 3}){
			int tmp_reward = before.slide(op);
			if(tmp_reward != -1){
				float tmp_v = get_next_step() + tmp_reward;
				if(tmp_v > best_v){
					best_op = op;
					best_v = tmp_v;
					best_reward = tmp_reward
				}
			}
		}
		
		board after(before);
		after_state.push_back(board(before.));
		if(best_reward==-1) return action();
		else return action::slide(op);
	}*/

	/**
	 * update the tuple network by an episode
	 *
	 * path is the sequence of states in pisode,
	 * the last entry in path (path.back()) is the final state
	 *
	 * for example, a 2048 games consists of
	 *  (initial) s0 --(a0,r0)--> s0' --(popup)--> s1 --(a1,r1)--> s1' --(popup)--> s2 (terminal)
	 *  where sx is before state, sx' is after state
	 *
	 * its path would be
	 *  { (s0,s0',a0,r0), (s1,s1',a1,r1), (s2,s2,x,-1) }
	 *  where (x,x,x,x) means (before state, after state, action, reward)
	 */
	//void update_episode(std::vector<state>& path, float alpha = 0.1) const {
	void update_episode() {
		//std::cout << "update_episode()\n";
		float exact = 0;
		for (path.pop_back() /* terminal state */; path.size(); path.pop_back()) {
			state& move = path.back();
			float error = exact - (move.value() - move.reward());
			//debug << "update error = " << error << " for after state" << std::endl << move.after_state();
			exact = move.reward() + update(move.after_state(), alpha * error);
		}
	}

	/**
	 * update the statistic, and display the status once in 1000 episodes by default
	 *
	 * the format would be
	 * 1000   mean = 273901  max = 382324
	 *        512     100%   (0.3%)
	 *        1024    99.7%  (0.2%)
	 *        2048    99.5%  (1.1%)
	 *        4096    98.4%  (4.7%)
	 *        8192    93.7%  (22.4%)
	 *        16384   71.3%  (71.3%)
	 *
	 * where (let unit = 1000)
	 *  '1000': current iteration (games trained)
	 *  'mean = 273901': the average score of last 1000 games is 273901
	 *  'max = 382324': the maximum score of last 1000 games is 382324
	 *  '93.7%': 93.7% (937 games) reached 8192-tiles in last 1000 games (a.k.a. win rate of 8192-tile)
	 *  '22.4%': 22.4% (224 games) terminated with 8192-tiles (the largest) in last 1000 games
	 */
	void make_statistic(size_t n, const board& b, int score, int unit = 1000) {
		scores.push_back(score);
		maxtile.push_back(0);
		for (int i = 0; i < 16; i++) {
			maxtile.back() = std::max(maxtile.back(), int(b(i)));
		}

		if (n % unit == 0) { // show the training process
			if (scores.size() != size_t(unit) || maxtile.size() != size_t(unit)) {
				error << "wrong statistic size for show statistics" << std::endl;
				std::exit(2);
			}
			int sum = std::accumulate(scores.begin(), scores.end(), 0);
			int max = *std::max_element(scores.begin(), scores.end());
			int stat[16] = { 0 };
			for (int i = 0; i < 16; i++) {
				stat[i] = std::count(maxtile.begin(), maxtile.end(), i);
			}
			float mean = float(sum) / unit;
			float coef = 100.0 / unit;
			info << n;
			info << "\t" "mean = " << mean;
			info << "\t" "max = " << max;
			info << std::endl;
			for (int t = 1, c = 0; c < unit; c += stat[t++]) {
				if (stat[t] == 0) continue;
				int accu = std::accumulate(stat + t, stat + 16, 0);
				info << "\t" << /*((1 << t) & -2u)*/(((1 << (t-3))*3) & -2u) << "\t" << (accu * coef) << "%";
				info << "\t(" << (stat[t] * coef) << "%)" << std::endl;
			}
			scores.clear();
			maxtile.clear();
		}
	}

	/**
	 * display the weight information of a given board
	 */
	void dump(const board& b, std::ostream& out = info) const {
		out << b << "estimate = " << evaluation(b) << std::endl;
		for (auto& wght : net) {
			out << wght.name() << std::endl;
			wght.dump(b, out);
		}
	}
//protected:
public:
	virtual void init_weights(/*const std::string& info*/) {
		//net.emplace_back(65536); // create an empty weight table with size 65536
		//net.emplace_back(65536); // create an empty weight table with size 65536
		//for(int i=0;i<8;++i)
		//	net.emplace_back(50625); // create an empty weight table with size 15^4
		// now net.size() == 2; net[0].size() == 65536; net[1].size() == 65536

		// initialize the features
		//debug << "init_weights\n" ;

		add_feature(new iso_pattern({ 0, 1, 2, 3, 4, 5 }));
		add_feature(new iso_pattern({ 4, 5, 6, 7, 8, 9 }));
		add_feature(new iso_pattern({ 0, 1, 2, 4, 5, 6 }));
		add_feature(new iso_pattern({ 4, 5, 6, 8, 9, 10 }));
		path.reserve(20000);
	}
	virtual void load_weights(const std::string& path) {
		std::ifstream in(path, std::ios::in | std::ios::binary);
		if (!in.is_open()) std::exit(-1);
		uint32_t size;
		in.read(reinterpret_cast<char*>(&size), sizeof(size));
		net.resize(size);
		for (iso_pattern& p : net) in >> p;
		in.close();
	}
	virtual void save_weights(const std::string& path) {
		std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!out.is_open()) std::exit(-1);
		uint32_t size = net.size();
		out.write(reinterpret_cast<char*>(&size), sizeof(size));
		for (iso_pattern& p : net) out << p;
		out.close();
	}


protected:
	std::vector<iso_pattern> net;
	std::vector<state> path;
	std::vector<int> scores;
	std::vector<int> maxtile;
	float alpha;
};