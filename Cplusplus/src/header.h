#pragma once

// 非标准头文件（MSVC编译器扩展）
// #include <hash_map> // deprecated, please use unordered_map instead
// #include <hash_set> // deprecated, please use unordered_set instead
#include <xfacet>
#include <xhash>
#include <xiosbase>
#include <xlocale>
#include <xlocbuf>
#include <xlocinfo>
#include <xlocmes>
#include <xlocmon>
#include <xlocnum>
#include <xloctime>
#include <xmemory>
#include <xstring>
#include <xtr1common>
#include <xtree>
#include <xutility>

// C++98、C++03和C++11标准化的头文件
#include <cassert>
// #include <ccomplex> // deprecated in C++20
#include <cctype>
#include <cerrno>
#include <cfenv>
#include <cfloat>
#include <cinttypes>
// #include <ciso646> // deprecated in C++20
#include <climits>
#include <clocale>
#include <cmath>
#include <csetjmp>
#include <csignal>
// #include <cstdalign> // deprecated in C++17
#include <cstdarg>
// #include <cstdbool> // deprecated in C++17
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// #include <ctgmath> // deprecated in C++17
#include <ctime>
#include <cuchar>
#include <cwchar>
#include <cwctype>

// #include <codecvt> // deprecated in C++17
#include <algorithm>
#include <complex>
#include <exception>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <istream>
#include <limits>
#include <numeric>
#include <ostream>
#include <random>
#include <ratio>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <system_error>
// #include <strstream> // deprecated in C++98

#include <array>
#include <atomic>
#include <bitset>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <forward_list>
#include <functional>
#include <future>
#include <initializer_list>
#include <iterator>
#include <list>
#include <locale>  // see clocale above
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <queue>  // contais std::priority_queue
#include <regex>
#include <scoped_allocator>
#include <set>
#include <stack>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>  // contains std::unordered_multimap
#include <unordered_set>  // contains std::unordered_multiset
#include <utility>
#include <valarray>
#include <vector>

// C++14
#include <shared_mutex>

// C++17
#include <any>
#include <charconv>
#include <execution>
#include <filesystem>
#include <memory_resource>
#include <optional>
#include <string_view>
#include <variant>
#include <version>

// C++20
#include <barrier>
#include <bit>
#include <compare>
#include <concepts>
#include <coroutine>
#include <format>
#include <latch>
#include <numbers>
#include <ranges>
#include <semaphore>
#include <source_location>
#include <span>
#include <stop_token>
#include <syncstream>

// C++23
#include <expected>
#include <mdspan>
#include <print>
#include <spanstream>
#include <stacktrace>
#include <stdfloat>
// #include <flat_set>
// #include <flat_map>
#include <generator>

// C++26
// #include <inplace_vector>
// #include <hive>

#ifdef _WIN32
#include <windows.h>
#include <immintrin.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
