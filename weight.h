/**
 * Framework for 2048 & 2048-like Games (C++ 11)
 * use 'g++ -std=c++11 -O3 -g -o 2048 2048.cpp' to compile the source
 *
 * Author: Hung Guei (moporgic)
 *         Computer Games and Intelligence (CGI) Lab, NCTU, Taiwan
 *         http://www.aigames.nctu.edu.tw
 */

#pragma once
#include <iostream>
#include <vector>
#include <utility>

std::ostream& info = std::cout;
std::ostream& error = std::cerr;
//std::ostream& debug = std::cout;

class weight {
public:
	weight() {}
	weight(size_t len) : value(len) {}
	weight(weight&& f) : value(std::move(f.value)) {}
	weight(const weight& f) = default;

	weight& operator =(const weight& f) = default;
	float& operator[] (size_t i) { return value[i]; }
	const float& operator[] (size_t i) const { return value[i]; }
	size_t size() const { return value.size(); }

public: // should be implemented

	/**
	 * estimate the value of a given board
	 */
	virtual float eval(const board& b) const = 0;
	/**
	 * update the value of a given board, and return its updated value
	 */
	virtual float update(const board& b, float u) = 0;
	/**
	 * get the name of this feature
	 */
	virtual std::string name() const = 0;

public:
	/**
	 * dump the detail of weight table of a given board
	 */
	virtual void dump(const board& b, std::ostream& out = info) const {
		out << b << "eval = " << eval(b) << std::endl;
	}

	friend std::ostream& operator <<(std::ostream& out, const weight& w) {
		auto& value = w.value;
		uint64_t size = value.size();
		out.write(reinterpret_cast<const char*>(&size), sizeof(uint64_t));
		out.write(reinterpret_cast<const char*>(value.data()), sizeof(float) * size);
		return out;
	}
	friend std::istream& operator >>(std::istream& in, weight& w) {
		auto& value = w.value;
		uint64_t size = 0;
		in.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));
		value.resize(size);
		in.read(reinterpret_cast<char*>(value.data()), sizeof(float) * size);
		return in;
	}

protected:
	std::vector<float> value;//15^4 wieght table
};

/**
 * the pattern feature
 * including isomorphic (rotate/mirror)
 *
 * index:
 *  0  1  2  3
 *  4  5  6  7
 *  8  9 10 11
 * 12 13 14 15
 *
 * usage:
 *  pattern({ 0, 1, 2, 3 })
 *  pattern({ 0, 1, 2, 3, 4, 5 })
 */
class iso_pattern : public weight {
public:
	iso_pattern(const std::vector<int>& p, int iso = 8) : weight(/*1 << (p.size() * 4)*/15*15*15*15*15*15), iso_last(iso) {
		if (p.empty()) {
			error << "no pattern defined" << std::endl;
			std::exit(1);
		}

		//set isomorphic
		for (int i = 0; i < 8; i++) {
			//board idx = 0xfedcba9876543210ull;
			//board idx = (board)({{0,1,2,3,4},{5,6,7,8},{9,10,11,12},{13,14,15,16}});
			board idx;
			idx.index_board();
			//for(int i=0 ; i<16 ; ++i)
			//	int tmp = idx.place(i,i);

			if (i >= 4) idx.reflect_horizontal();
			idx.rotate(i);
			for (int t : p) {
				pattern[i].push_back(idx(t));
			}
		}
	}
	iso_pattern(){}
	iso_pattern(const iso_pattern& p) = default;
	//pattern(const pattern& p) = delete;
	virtual ~iso_pattern() {}
	iso_pattern& operator =(const iso_pattern& p) = delete;

public:

	/**
	 * estimate the value of a given board
	 */
	virtual float eval(const board& b) const {
		float value = 0;
		for (int i = 0; i < iso_last; i++) {
			size_t index = indexof(pattern[i], b);
			value += operator[](index);
		}
		return value;
	}

	/**
	 * update the value of a given board, and return its updated value
	 */
	virtual float update(const board& b, float u) {
		float u_split = u / iso_last;
		float value = 0;
		for (int i = 0; i < iso_last; i++) {
			size_t index = indexof(pattern[i], b);
			operator[](index) += u_split;
			value += operator[](index);
		}
		return value;
	}

	/**
	 * get the name of this feature
	 */
	virtual std::string name() const {
		return std::to_string(pattern[0].size()) + "-tuple pattern " + nameof(pattern[0]);
	}

public:

	/*
	 * set the isomorphic level of this pattern
	 * 1: no isomorphic
	 * 4: enable rotation
	 * 8: enable rotation and reflection
	 */
	void set_isomorphic(int i = 8) { iso_last = i; }

	/**
	 * display the weight information of a given board
	 */
	
	void dump(const board& b, std::ostream& out = info) const {
		for (int i = 0; i < iso_last; i++) {
			out << "#" << i << ":" << nameof(pattern[i]) << "(";
			size_t index = indexof(pattern[i], b);
			for (size_t i = 0; i < pattern[i].size(); i++) {
				out << std::hex << ((index >> (4 * i)) & 0x0f);
			}
			out << std::dec << ") = " << operator[](index) << std::endl;
		}
	}
	

protected:

	size_t indexof(const std::vector<int>& patt, const board& b) const {
		size_t index = 0;
		for (size_t i = 0; i < patt.size(); i++)
			index |= b(patt[i]) << (4*i);
		return index;
	}

	std::string nameof(const std::vector<int>& patt) const {
		std::stringstream ss;
		ss << std::hex;
		std::copy(patt.cbegin(), patt.cend(), std::ostream_iterator<int>(ss, ""));
		return ss.str();
	}

	std::array<std::vector<int>, 8> pattern;
	int iso_last;
	//std::vector<int> patt;
};
