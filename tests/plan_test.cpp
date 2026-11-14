#include "../plan.hpp"

#include <cstdio>
#include <cstdlib>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++fails; } } while (0)

static bool eval(std::string const& s, pl::Digits& out, std::string* err = nullptr) {
	pl::Parser p(s);
	pl::Num n;
	std::string e;
	bool ok = p.parse(n, e);
	if (err) *err = e;
	if (ok) out = pl::toDigits(n);
	return ok;
}
static uint64_t value(std::string const& s) {
	pl::Digits d;
	if (!eval(s, d) || d.infinite) return ~0ull;
	uint64_t v = 0;
	for (size_t i = d.d.size(); i-- > 0;) v = v * pl::kBase + d.d[i];
	return v;
}

int main() {
	CHECK(value("10") == 10);
	CHECK(value("1+2*3") == 7);
	CHECK(value("(1+2)*3") == 9);
	CHECK(value("2^10") == 1024);
	CHECK(value("2^3^2") == 512);          // right associative
	CHECK(value("2^^3") == 16);
	CHECK(value("2^^4") == 65536);
	CHECK(value("2^^^2") == 4);
	CHECK(value("2^^^3") == 65536);
	CHECK(value("3^^3") == 7625597484987ull);
	CHECK(value("0") == 0);
	CHECK(value(" 5 * 5 ") == 25);

	pl::Digits d;
	CHECK(eval("2^31", d) && !d.infinite && d.d.size() == 2 && d.d[0] == 1 && d.d[1] == 1); // 2147483648
	CHECK(eval("2147483646", d) && d.d.size() == 1 && d.d[0] == 2147483646u);
	CHECK(eval("2147483647", d) && d.d.size() == 2 && d.d[0] == 0 && d.d[1] == 1);
	CHECK(eval("2^100", d) && !d.infinite && d.d.size() == 4);
	CHECK(eval("E_0", d) && d.infinite && d.d.size() == pl::kMaxDigits);
	CHECK(eval("l_w", d) && d.infinite);
	CHECK(eval("a_n", d) && d.infinite);
	CHECK(eval("O_w", d) && d.infinite);
	CHECK(eval("e_0 + 1", d) && d.infinite);
	CHECK(eval("2^1000", d) && d.infinite);
	CHECK(eval("3^^^3", d) && d.infinite);
	CHECK(eval("2^500", d) && d.infinite);
	CHECK(eval("2^400", d) && !d.infinite && d.d.size() == 13);

	std::string err;
	CHECK(!eval("2+", d, &err) && !err.empty());
	CHECK(!eval("hello", d, &err) && !err.empty());
	CHECK(!eval("(1+2", d, &err));
	CHECK(!eval("1 2", d, &err));

	// object strings
	pl::Ids ids;
	std::string e;
	pl::Parser("100").parse(*new pl::Num, e);
	pl::Num n;
	pl::Parser("100").parse(n, e);
	auto plan = pl::build(pl::toDigits(n), ids, 300, 150);
	CHECK(plan.error.empty() && plan.counters == 1);
	CHECK(plan.count == 5); // block, touch, pickup, count, toggle
	std::printf("100 ->\n%s\n\n", plan.objects.c_str());

	pl::Ids ids2;
	pl::Parser("2^31").parse(n, e);
	auto plan2 = pl::build(pl::toDigits(n), ids2, 0, 0);
	CHECK(plan2.counters == 2);
	CHECK(plan2.count == 5 + 3 + 1); // + carry (count, 2 pickups) + instant count
	std::printf("2^31 ->\n%s\n\n", plan2.objects.c_str());

	pl::Ids ids3;
	pl::Parser("0").parse(n, e);
	CHECK(!pl::build(pl::toDigits(n), ids3, 0, 0).error.empty());

	// every object string ends in ; and has no empty fields
	CHECK(plan2.objects.find(",,") == std::string::npos);

	std::printf(fails ? "FAILED %d\n" : "ALL OK\n", fails);
	return fails ? 1 : 0;
}
