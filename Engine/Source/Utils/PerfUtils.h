#pragma once

#include <chrono>
#include <stdint.h>

// Performance
// Example:
/*
//     BENCH("SqrtTest", {
//         const float res = std::sqrt(30.f);
//         });
*/

namespace Utils
{
	// Performance
	class Benchmark {
	public:
		std::chrono::time_point<std::chrono::high_resolution_clock> t0, t1;
		eastl::string name;

		Benchmark(eastl::string& inName) {
			t0 = std::chrono::high_resolution_clock::now();
			name = inName;
		}
		~Benchmark() {
			t1 = std::chrono::high_resolution_clock::now();
			auto start = std::chrono::time_point_cast<std::chrono::microseconds>(t0).time_since_epoch().count();
			auto end = std::chrono::time_point_cast<std::chrono::microseconds>(t1).time_since_epoch().count();
			int64_t res = end - start;

			std::cout << name.c_str() << " took: " << res << "us (" << res * 0.001 << "ms)\n";
		}
	};

	class BenchmarkCode {
	public:
		std::chrono::time_point<std::chrono::high_resolution_clock> t0, t1;
		int64_t* d;
		BenchmarkCode(int64_t* res) : d(res) {
			t0 = std::chrono::high_resolution_clock::now();
		}
		~BenchmarkCode() {
			t1 = std::chrono::high_resolution_clock::now();
			auto start = std::chrono::time_point_cast<std::chrono::microseconds>(t0).time_since_epoch().count();
			auto end = std::chrono::time_point_cast<std::chrono::microseconds>(t1).time_since_epoch().count();
			*d = end - start;
		}
	};
}

#define BENCH_CODE(TITLE,CODEBLOCK)																				\
	  int64_t __time__##__LINE__ = 0;																			\
	  { Utils::BenchmarkCode bench(&__time__##__LINE__);														\
		  CODEBLOCK																								\
	  }																											\
	  std::cout << TITLE << " took: " <<__time__##__LINE__ << "us (" << __time__##__LINE__ * 0.001 << "ms)\n";

#define BENCH_SCOPE(TITLE)																						\
	  Utils::Benchmark bench(eastl::string(TITLE));																\
