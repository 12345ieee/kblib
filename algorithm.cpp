#include "kblib/algorithm.h"
#include "kblib/stats.h"
#include "catch.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>

TEST_CASE("erase") {
	const auto equal = [](auto a, auto b) {
		return std::equal(std::begin(a), std::end(a), std::begin(b), std::end(b));
	};
	SECTION("erase and erase_if") {
		const std::vector<int> erase_test{2, 2, 3, 4, 5, 7, 8, 11};
		const std::vector<int> no_2s = {3, 4, 5, 7, 8, 11};
		auto erase_copy = erase_test;
		kblib::erase(erase_copy, 2);
		REQUIRE(equal(no_2s, erase_copy));
		erase_copy = erase_test;
		const std::vector<int> no_evens = {3, 5, 7, 11};
		kblib::erase_if(erase_copy, [](int x) { return (~x) & 1; });
		REQUIRE(equal(no_evens, erase_copy));
	}
}

TEST_CASE("find family") {
	// TODO
}

TEST_CASE("get_max family") {
	// TODO
}

TEST_CASE("general algorithms") {
	// TODO
}

template <typename T, std::size_t N>
constexpr bool sort_test(kblib::trivial_array<T, N> val) noexcept {
	kblib::trivial_array<T, N> out{};
	kblib::insertion_sort_copy(val.begin(), val.end(), out.begin(), out.end());
	kblib::insertion_sort(val.begin(), val.end());
	return true;
}

TEST_CASE("sort") {
	constexpr kblib::trivial_array<int, 7> input{{3, 7, 4, 3, 1, 9, 5}};
	const auto goal = [&] {
		auto copy = input;
		std::sort(copy.begin(), copy.end());
		return copy;
	}();

	[[gnu::unused]] auto print_arr = [&](auto c) {
		for (const auto& v : c) {
			std::cout << v << ", ";
		}
		std::cout << '\n';
	};

#if KBLIB_USE_CXX17
	static_assert(sort_test(input), "insertion_sort should be constexpr");
#endif

	SECTION("insertion_sort") {
		auto input_copy = input;
		kblib::insertion_sort(input_copy.begin(), input_copy.end());
		REQUIRE(input_copy == goal);
		static_assert(noexcept(kblib::insertion_sort(
		                  input_copy.begin(), input_copy.end())),
		              "insertion_sort for array<int> should be noexcept");
	}
	SECTION("insertion_sort_copy") {
		std::remove_const<decltype(input)>::type output;
		kblib::insertion_sort_copy(input.begin(), input.end(), output.begin(),
		                           output.end());
		REQUIRE(output == goal);
		static_assert(noexcept(kblib::insertion_sort_copy(
		                  input.begin(), input.end(), output.begin(),
		                  output.end())),
		              "insertion_sort_copy for array<int> should be noexcept");
	}
	SECTION("adaptive_insertion_sort_copy") {
		std::remove_const<decltype(input)>::type output;
		kblib::adaptive_insertion_sort_copy(input.begin(), input.end(),
		                                    output.begin(), output.end());
		REQUIRE(output == goal);
	}
	SECTION("insertion_sort is stable") {
		std::minstd_rand rng;
		std::uniform_int_distribution<int> dist(0, 8);
		for ([[gnu::unused]] auto _i : kblib::range(100)) {
			// sort based on first key, second is used to distinguish between equal elements
			std::vector<std::pair<int, int>> inputs;
			auto pcomp = [](auto a, auto b){return a.first < b.first;};

			{
				std::map<int, int> counts;
				for ([[gnu::unused]] auto _j : kblib::range(100)) {
					auto r = dist(rng);
					inputs.push_back({r, counts[r]++});
				}
			}

			kblib::insertion_sort(inputs.begin(), inputs.end(), pcomp);

			{
				std::map<int, int> counts;
				for (auto p : inputs) {
					REQUIRE(p.second == counts[p.first]++);
				}
			}
		}
	}
	SECTION("insertion_sort on random data") {
		std::minstd_rand rng;
		std::uniform_int_distribution<int> dist(0, 65535);
		for ([[gnu::unused]] auto _i : kblib::range(100)) {
			std::vector<int> input;
			std::generate_n(std::back_inserter(input), 100, [&]{return dist(rng);});

			auto output_std = input;
			std::sort(output_std.begin(), output_std.end());
			decltype(input) output_kblib(input.size());
			kblib::insertion_sort_copy(input.cbegin(), input.cend(), output_kblib.begin(), output_kblib.end());
			REQUIRE(output_std == output_kblib);
		}
	}
}
TEST_CASE("insertion sort performance") {
	SECTION("insertion_sort_copy on sorted data is fast") {
		auto time_per = [](std::size_t size) {
			std::vector<int> input(size);
			decltype(input) output(size);
			std::iota(input.begin(), input.end(), 0);

			auto start = std::chrono::high_resolution_clock::now();
			kblib::insertion_sort_copy(input.cbegin(), input.cend(), output.begin(), output.end());
			auto end = std::chrono::high_resolution_clock::now();
			auto duration = end - start;
			return static_cast<double>(duration.count());
		};

		auto time_fast = time_per(30)/30;
		double time_slow = time_per(10000)/10000;
		double error = time_slow / time_fast;
		std::cout<<time_fast<<'\t'<<time_slow<<'\t'<<error<<"\n";
		// Can't overshoot the bound by more than 5%:
		REQUIRE(error < 1.05);
	}
	SECTION("insertion_sort_copy on reverse sorted data is slow") {
		auto time_per = [](std::size_t size) {
			std::vector<int> input(size);
			decltype(input) output(size);
			std::iota(input.rbegin(), input.rend(), 0);

			auto start = std::chrono::high_resolution_clock::now();
			kblib::insertion_sort_copy(input.cbegin(), input.cend(), output.begin(), output.end());
			auto end = std::chrono::high_resolution_clock::now();
			auto duration = end - start;
			return static_cast<double>(duration.count());
		};

		auto time_fast = time_per(30)/(30*30);
		auto time_slow = time_per(1000)/(1000*1000);
		auto error = time_slow / time_fast;
		std::cout<<time_fast<<'\t'<<time_slow<<'\t'<<error<<"\n";
		// Can't overshoot the bound by more than 5%:
		REQUIRE(error < 1.05);
	}
	SECTION("adaptive_insertion_sort_copy on sorted data is fast") {
		auto time_per = [](std::size_t size) {
			std::vector<int> input(size);
			decltype(input) output(size);
			std::iota(input.begin(), input.end(), 0);

			auto start = std::chrono::high_resolution_clock::now();
			kblib::adaptive_insertion_sort_copy(input.cbegin(), input.cend(), output.begin(), output.end());
			auto end = std::chrono::high_resolution_clock::now();
			auto duration = end - start;
			return static_cast<double>(duration.count());
		};

		auto time_fast = time_per(30)/(30);
		auto time_slow = time_per(10'000)/(10'000);
		double error = time_slow / time_fast;
		std::cout<<time_fast<<'\t'<<time_slow<<'\t'<<error<<"\n";
		// Can't overshoot the bound by more than 5%:
		REQUIRE(error < 1.05);
	}
	SECTION("adaptive_insertion_sort_copy on reverse sorted data is fast") {
		auto time_per = [](std::size_t size) {
			std::vector<int> input(size);
			decltype(input) output(size);
			std::iota(input.rbegin(), input.rend(), 0);

			auto start = std::chrono::high_resolution_clock::now();
			kblib::adaptive_insertion_sort_copy(input.cbegin(), input.cend(), output.begin(), output.end());
			auto end = std::chrono::high_resolution_clock::now();
			auto duration = end - start;
			return static_cast<double>(duration.count());
		};

		auto time_fast = time_per(30)/(30);
		auto time_slow = time_per(10'000)/(10'000);
		double error = time_slow / time_fast;
		std::cout<<time_fast<<'\t'<<time_slow<<'\t'<<error<<"\n";
		// Can't overshoot the bound by more than 5%:
		REQUIRE(error < 1.05);
	}
	SECTION("insertion_sort_copy on mostly sorted data is fast") {
		std::minstd_rand rng;
		auto time_per = [&](std::size_t size, int noise) {
			std::uniform_int_distribution<int> dist(-noise, noise);
			std::vector<int> input(size);
			decltype(input) output(size);
			for (auto i : kblib::range(size)) {
				input[i] = i + dist(rng);
			}

			auto start = std::chrono::high_resolution_clock::now();
			kblib::insertion_sort_copy(input.cbegin(), input.cend(), output.begin(), output.end());
			auto end = std::chrono::high_resolution_clock::now();
			auto duration = end - start;
			return static_cast<double>(duration.count());
		};

		auto n = 10000;
		auto v = 250;
		auto time_fast = time_per(n, 0)/(n);
		auto time_slow = time_per(n, v)/(n);
		auto ratio = time_slow / time_fast;
		auto error = ratio/(n/v);
		std::cout<<time_fast<<'\t'<<time_slow<<'\t'<<error<<"\n";
		// Can't overshoot the bound by more than 5%:
		REQUIRE(error < 1.05);
	}
}

TEST_CASE("zip") {
	SECTION("non-overlapping") {
		std::vector<int> input1{1, 2, 3, 4, 5, 6};
		std::vector<short> input2{2, 3, 4, 5, 6, 7};

		for (auto t : kblib::zip(input1.begin(), input1.end(), input2.begin())) {
			auto& a = std::get<0>(t);
			auto& b = std::get<1>(t);
			REQUIRE(a + 1 == b);
		}
	}
	SECTION("identical") {
		std::vector<int> input{1, 2, 3, 4, 5, 6, 7, 8};
		for (auto t : kblib::zip(input.begin(), input.end(), input.begin())) {
			auto& a = std::get<0>(t);
			auto& b = std::get<1>(t);
			REQUIRE(&a == &b);
		}
	}
	SECTION("overlapping") {
		std::vector<int> input{1, 2, 3, 4, 5, 6, 7, 8};
		for (auto t :
		     kblib::zip(input.begin(), input.end() - 1, input.begin() + 1)) {
			auto& a = std::get<0>(t);
			auto& b = std::get<1>(t);
			REQUIRE(a + 1 == b);
		}
	}
}
