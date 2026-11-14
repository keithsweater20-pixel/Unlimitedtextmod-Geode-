// Press Limit: the maths and the object-string builder. No Geode or Cocos in here,
// so it can be compiled and tested anywhere.
#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace pl {

// A counter resets to 0 (and the next counter goes up by 1) when it reaches this: the biggest
// 32-bit signed integer. So the counters together are a number written in base kBase.
constexpr uint32_t kBase = 2147483647u;
constexpr size_t kMaxDigits = 16;                // at most 16 counters
constexpr size_t kCapBits = 31 * kMaxDigits;     // anything bigger counts as "never reached"

// ---------------------------------------------------------------- big numbers
struct Big {
	std::vector<uint32_t> d; // little-endian, base 2^32, no leading zero words
};

inline void trim(Big& a) {
	while (!a.d.empty() && a.d.back() == 0) a.d.pop_back();
}
inline Big fromU64(uint64_t v) {
	Big b;
	while (v) {
		b.d.push_back(static_cast<uint32_t>(v));
		v >>= 32;
	}
	return b;
}
inline bool isZero(Big const& a) { return a.d.empty(); }
inline size_t bitLength(Big const& a) {
	if (a.d.empty()) return 0;
	uint32_t top = a.d.back();
	size_t n = 0;
	while (top) {
		++n;
		top >>= 1;
	}
	return 32 * (a.d.size() - 1) + n;
}
// value if it fits in 64 bits
inline bool toU64(Big const& a, uint64_t& out) {
	if (a.d.size() > 2) return false;
	out = 0;
	for (size_t i = a.d.size(); i-- > 0;) out = (out << 32) | a.d[i];
	return true;
}
inline Big add(Big const& a, Big const& b) {
	Big r;
	uint64_t carry = 0;
	for (size_t i = 0; i < std::max(a.d.size(), b.d.size()) || carry; ++i) {
		uint64_t s = carry;
		if (i < a.d.size()) s += a.d[i];
		if (i < b.d.size()) s += b.d[i];
		r.d.push_back(static_cast<uint32_t>(s));
		carry = s >> 32;
	}
	return r;
}
inline Big mul(Big const& a, Big const& b) {
	if (a.d.empty() || b.d.empty()) return {};
	Big r;
	r.d.assign(a.d.size() + b.d.size(), 0);
	for (size_t i = 0; i < a.d.size(); ++i) {
		uint64_t carry = 0;
		for (size_t j = 0; j < b.d.size() || carry; ++j) {
			uint64_t cur = r.d[i + j] + carry;
			if (j < b.d.size()) cur += static_cast<uint64_t>(a.d[i]) * b.d[j];
			r.d[i + j] = static_cast<uint32_t>(cur);
			carry = cur >> 32;
		}
	}
	trim(r);
	return r;
}
// a /= m, returns the remainder
inline uint32_t divmodSmall(Big& a, uint32_t m) {
	uint64_t rem = 0;
	for (size_t i = a.d.size(); i-- > 0;) {
		uint64_t cur = (rem << 32) | a.d[i];
		a.d[i] = static_cast<uint32_t>(cur / m);
		rem = cur % m;
	}
	trim(a);
	return static_cast<uint32_t>(rem);
}

// ---------------------------------------------------------------- numbers with "infinite"
struct Num {
	bool inf = false;
	Big v;
};
inline Num infinite() {
	Num n;
	n.inf = true;
	return n;
}
inline Num finite(Big b) {
	Num n;
	if (bitLength(b) > kCapBits) return infinite();
	n.v = std::move(b);
	return n;
}
inline Num smallNum(uint64_t v) { return finite(fromU64(v)); }

inline Num nAdd(Num const& a, Num const& b) {
	if (a.inf || b.inf) return infinite();
	return finite(add(a.v, b.v));
}
inline Num nMul(Num const& a, Num const& b) {
	if ((!a.inf && isZero(a.v)) || (!b.inf && isZero(b.v))) return smallNum(0);
	if (a.inf || b.inf) return infinite();
	return finite(mul(a.v, b.v));
}
inline bool isVal(Num const& a, uint64_t v) {
	uint64_t x;
	return !a.inf && toU64(a.v, x) && x == v;
}

// a ^ e
inline Num nPow(Num const& a, Num const& e) {
	if (isVal(e, 0)) return smallNum(1);
	if (isVal(a, 0)) return smallNum(0);
	if (isVal(a, 1)) return smallNum(1);
	if (a.inf || e.inf) return infinite();
	uint64_t ev;
	if (!toU64(e.v, ev) || ev >= kCapBits) return infinite(); // a >= 2 here, so 2^496 is already too big
	Num result = smallNum(1), base = a;
	while (ev) {
		if (ev & 1) {
			result = nMul(result, base);
			if (result.inf) return result;
		}
		ev >>= 1;
		if (ev) {
			base = nMul(base, base);
			if (base.inf) return base;
		}
	}
	return result;
}

// a (n up-arrows) b  :  n=1 is a^b, n=2 is a^^b (a tower of b copies of a), and so on
inline Num hyper(Num const& a, int n, Num const& b) {
	if (n <= 1) return nPow(a, b);
	if (isVal(b, 0)) return smallNum(1);
	if (isVal(a, 1)) return smallNum(1);
	if (isVal(a, 0)) {
		uint64_t bv;
		return (toU64(b.v, bv) && !b.inf && (bv & 1)) ? smallNum(0) : smallNum(1);
	}
	if (a.inf) return infinite();
	if (b.inf) return infinite();
	uint64_t count;
	if (!toU64(b.v, count) || count > 100000) return infinite();
	Num r = a;
	for (uint64_t i = 1; i < count; ++i) {
		r = hyper(a, n - 1, r);
		if (r.inf) return r;
	}
	return r;
}

// ---------------------------------------------------------------- expression parser
// Numbers and + * ^ (any number of ^ in a row = Knuth arrows) and ( ).
// Names: E_0 = epsilon naught, l_w = lowercase omega, a_n = aleph null, O_w = Omega.
// All the "infinite" names count as never reached.
class Parser {
public:
	explicit Parser(std::string text) : s_(std::move(text)) {}

	bool parse(Num& out, std::string& error) {
		out = expr();
		skip();
		if (ok_ && i_ < s_.size()) fail(std::string("Unexpected '") + s_[i_] + "'");
		error = err_;
		return ok_;
	}

private:
	std::string s_, err_;
	size_t i_ = 0;
	bool ok_ = true;
	int depth_ = 0;

	void fail(std::string m) {
		if (ok_) err_ = std::move(m);
		ok_ = false;
	}
	void skip() {
		while (i_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[i_]))) ++i_;
	}
	Num expr() {
		Num v = term();
		for (;;) {
			skip();
			if (ok_ && i_ < s_.size() && s_[i_] == '+') {
				++i_;
				v = nAdd(v, term());
			} else {
				return v;
			}
		}
	}
	Num term() {
		Num v = power();
		for (;;) {
			skip();
			if (ok_ && i_ < s_.size() && (s_[i_] == '*' || s_[i_] == 'x')) {
				++i_;
				v = nMul(v, power());
			} else {
				return v;
			}
		}
	}
	Num power() {
		Num base = atom();
		skip();
		int arrows = 0;
		while (ok_ && i_ < s_.size() && s_[i_] == '^') {
			++arrows;
			++i_;
		}
		if (arrows == 0 || !ok_) return base;
		Num top = power(); // right associative
		return hyper(base, arrows, top);
	}
	Num atom() {
		skip();
		if (i_ >= s_.size()) {
			fail("Something is missing at the end");
			return smallNum(0);
		}
		char c = s_[i_];
		if (c == '(') {
			if (++depth_ > 20) {
				fail("Too many brackets");
				return smallNum(0);
			}
			++i_;
			Num v = expr();
			skip();
			if (i_ < s_.size() && s_[i_] == ')') ++i_;
			else fail("Missing )");
			--depth_;
			return v;
		}
		if (std::isdigit(static_cast<unsigned char>(c))) {
			Big v;
			Big ten = fromU64(10);
			size_t digits = 0;
			while (i_ < s_.size() && std::isdigit(static_cast<unsigned char>(s_[i_]))) {
				if (++digits > 300) {
					fail("That number is too long");
					return smallNum(0);
				}
				v = add(mul(v, ten), fromU64(static_cast<uint64_t>(s_[i_] - '0')));
				++i_;
			}
			return finite(std::move(v));
		}
		if (std::isalpha(static_cast<unsigned char>(c))) {
			std::string name;
			while (i_ < s_.size() && (std::isalnum(static_cast<unsigned char>(s_[i_])) || s_[i_] == '_')) {
				name += static_cast<char>(std::tolower(static_cast<unsigned char>(s_[i_])));
				++i_;
			}
			if (name == "e_0" || name == "l_w" || name == "a_n" || name == "o_w") return infinite();
			fail("Unknown name '" + name + "'. Use E_0, l_w, a_n or O_w");
			return smallNum(0);
		}
		fail(std::string("Unexpected '") + c + "'");
		return smallNum(0);
	}
};

// ---------------------------------------------------------------- counters
struct Digits {
	std::vector<uint32_t> d; // d[0] is the counter that every click goes into
	bool infinite = false;   // too big (or infinite): the limit can never really be reached
};

inline Digits toDigits(Num const& n) {
	Digits out;
	if (n.inf) {
		out.infinite = true;
		out.d.assign(kMaxDigits, kBase - 1);
		return out;
	}
	Big v = n.v;
	while (!isZero(v)) out.d.push_back(divmodSmall(v, kBase));
	if (out.d.size() > kMaxDigits) {
		out.infinite = true;
		out.d.assign(kMaxDigits, kBase - 1);
	}
	return out;
}

inline std::string digitsText(Digits const& dg) {
	std::string s;
	for (size_t i = dg.d.size(); i-- > 0;) {
		s += std::to_string(dg.d[i]);
		if (i) s += " | ";
	}
	return s;
}

// ---------------------------------------------------------------- the objects
struct Ids {
	int nextGroup = 1000;
	int nextItem = 1000;
	int group() { return nextGroup++; }
	int item() { return nextItem++; }
};

struct Plan {
	std::string objects; // GD level string
	int count = 0;       // how many objects
	int counters = 0;
	std::string error;
};

namespace id {
constexpr int Block = 1;
constexpr int Toggle = 1049;
constexpr int Touch = 1595;
constexpr int CountTrigger = 1611;
constexpr int InstantCount = 1811;
constexpr int Pickup = 1817;
} // namespace id

class Builder {
public:
	Builder(float x, float y) : x_(x), y_(y) {}

	std::string str() const { return out_.str(); }
	int count() const { return n_; }

	// one object; `props` is like "57,1001,62,1". Triggers go in a row; the block sits above the row.
	void put(int objectID, std::string const& props, bool inRow = true) {
		float px = inRow ? x_ + 30.f * col_++ : x_;
		float py = inRow ? y_ : y_ + 60.f;
		out_ << "1," << objectID << ",2," << px << ",3," << py;
		if (!props.empty()) out_ << ',' << props;
		out_ << ';';
		++n_;
	}

private:
	std::ostringstream out_;
	float x_, y_;
	int col_ = 0;
	int n_ = 0;
};

inline std::string spawnProps(int group) { return "57," + std::to_string(group) + ",62,1,87,1"; }

// Builds: a block, and the triggers that count presses and remove the block at the limit.
inline Plan build(Digits const& dg, Ids& ids, float x, float y) {
	Plan p;
	if (dg.d.empty()) {
		p.error = "The limit is 0, so there is nothing to count. Use 1 or more.";
		return p;
	}
	size_t k = dg.d.size();
	std::vector<int> item(k);
	for (auto& it : item) it = ids.item();
	int gBlock = ids.group(), gTouch = ids.group(), gToggle = ids.group();

	Builder b(x, y);
	auto S = [](int v) { return std::to_string(v); };

	b.put(id::Block, "57," + S(gBlock), false);

	// every press: touch trigger -> group that adds 1 to counter 0
	b.put(id::Touch, "11,1,51," + S(gTouch));
	b.put(id::Pickup, spawnProps(gTouch) + ",80," + S(item[0]) + ",77,1");

	// when counter i reaches the max it goes back to 0 and counter i+1 goes up by 1
	for (size_t i = 0; i + 1 < k; ++i) {
		int gCarry = ids.group();
		b.put(id::CountTrigger, "80," + S(item[i]) + ",77," + std::to_string(kBase) + ",51," + S(gCarry) + ",56,0,104,1");
		b.put(id::Pickup, spawnProps(gCarry) + ",80," + S(item[i]) + ",77,0,139,1");
		b.put(id::Pickup, spawnProps(gCarry) + ",80," + S(item[i + 1]) + ",77,1");
	}

	// the limit check: counter 0 reaches its digit, then each higher counter is tested to match
	std::vector<int> check(k);
	for (size_t j = 1; j < k; ++j) check[j] = ids.group();
	std::string first = "80," + S(item[0]) + ",77," + std::to_string(dg.d[0]) + ",56,0";
	first += (k == 1) ? ",51," + S(gToggle) : ",51," + S(check[1]) + ",104,1";
	b.put(id::CountTrigger, first);
	for (size_t j = 1; j < k; ++j) {
		int next = (j + 1 < k) ? check[j + 1] : gToggle;
		b.put(id::InstantCount, spawnProps(check[j]) + ",80," + S(item[j]) + ",77," + std::to_string(dg.d[j]) + ",88,0,51," + S(next) + ",56,0");
	}

	// at the limit: the block is switched off
	b.put(id::Toggle, spawnProps(gToggle) + ",51," + S(gBlock) + ",56,0");

	p.objects = b.str();
	p.count = b.count();
	p.counters = static_cast<int>(k);
	return p;
}

} // namespace pl
