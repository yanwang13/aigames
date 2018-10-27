#pragma once
#include <iostream>
#include <vector>
#include <utility>
#include "board.h"

//std::ostream& info = std::cout;
//std::ostream& error = std::cerr;
std::ostream& debug = std::cout;

class board;
/**
 * before state and after state wrapper
 */
class state {
friend class agent;
public:
	state(int opcode = -1)
		: opcode(opcode), score(-1), esti(-std::numeric_limits<float>::max()) {}
	state(const board& b, int opcode = -1)
		: opcode(opcode), score(-1), esti(-std::numeric_limits<float>::max()) { assign(b); }
	state(const state& st) = default;
	state& operator =(const state& st) = default;

public:
	board after_state() const { return after; }
	board before_state() const { return before; }
	float value() const { return esti; }
	int reward() const { return score; }
	int action() const { return opcode; }

	void set_before_state(const board& b) { before = b; }
	void set_after_state(const board& b) { after = b; }
	void set_value(float v) { esti = v; }
	void set_reward(int r) { score = r; }
	void set_action(int a) { opcode = a; }

public:
	bool operator ==(const state& s) const {
		return (opcode == s.opcode) && (before == s.before) && (after == s.after) && (esti == s.esti) && (score == s.score);
	}
	bool operator < (const state& s) const {
		if (before != s.before) throw std::invalid_argument("state::operator<");
		return esti < s.esti;
	}
	bool operator !=(const state& s) const { return !(*this == s); }
	bool operator > (const state& s) const { return s < *this; }
	bool operator <=(const state& s) const { return !(s < *this); }
	bool operator >=(const state& s) const { return !(*this < s); }

public:

	/**
	 * assign a state (before state), then apply the action (defined in opcode)
	 * return true if the action is valid for the given state
	 */
	bool assign(const board& b) {
		//debug << "assign " << name() << std::endl << b;
		after = before = b;
		score = after.slide(opcode);
		esti = score;
		return score != -1;
	}

	/**
	 * call this function after initialization (assign, set_value, etc)
	 *
	 * the state is invalid if
	 *  estimated value becomes to NaN (wrong learning rate?)
	 *  invalid action (cause after == before or score == -1)
	 */
	bool is_valid() const {
		if (std::isnan(esti)) {
			std::cerr << "numeric exception" << std::endl;
			std::exit(1);
		}
		return after != before && opcode != -1 && score != -1;
	}

	const char* name() const {
		static const char* opname[4] = { "up", "right", "down", "left" };
		return (opcode >= 0 && opcode < 4) ? opname[opcode] : "none";
	}

    friend std::ostream& operator <<(std::ostream& out, const state& st) {
		out << "moving " << st.name() << ", reward = " << st.score;
		if (st.is_valid()) {
			out << ", value = " << st.esti << std::endl << st.after;
		} else {
			out << " (invalid)" << std::endl;
		}
		return out;
	}
private:
	board before;
	board after;
	int opcode;
	int score;
	float esti;
};
