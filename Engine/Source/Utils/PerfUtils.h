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

// Performance
class Benchmark {
public:
	std::chrono::time_point<std::chrono::high_resolution_clock> t0, t1;
	int64_t* d;
	Benchmark(int64_t* res) : d(res) {
		t0 = std::chrono::high_resolution_clock::now();
	}
	~Benchmark() {
		t1 = std::chrono::high_resolution_clock::now();
		auto start = std::chrono::time_point_cast<std::chrono::microseconds>(t0).time_since_epoch().count();
		auto end = std::chrono::time_point_cast<std::chrono::microseconds>(t1).time_since_epoch().count();
		*d = end - start;
	}
};

#define BENCH(TITLE,CODEBLOCK)							\
	  int64_t __time__##__LINE__ = 0;					\
	  { Utils::Benchmark bench(&__time__##__LINE__);	\
		  CODEBLOCK										\
	  }													\
	  std::cout << TITLE << " took: " <<__time__##__LINE__ << "us (" << __time__##__LINE__ * 0.001 << "ms)\n";
