// Copyright 2018 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef MEMORY_TOOLS__COUNT_FUNCTION_OCCURENCES_IN_BACKTRACE_HPP_
#define MEMORY_TOOLS__COUNT_FUNCTION_OCCURENCES_IN_BACKTRACE_HPP_

#include "./safe_fwrite.hpp"

#if defined(_WIN32)

// Include nothing for now.

#else  // defined(_WIN32)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <execinfo.h>
#include <stdexcept>

#endif  // defined(_WIN32)

namespace osrf_testing_tools_cpp
{
namespace memory_tools
{

class not_implemented : public std::logic_error
{
public:
  not_implemented() : std::logic_error("This function is not implemented.") {}
};

template<typename T>
struct is_function_pointer
: std::integral_constant<bool,
    std::is_pointer<T>::value && std::is_function<typename std::remove_pointer<T>::type>::value
  >
{};

#if defined(_WIN32)

struct count_function_occurrences_in_backtrace_is_implemented : std::false_type {};

template<int MaxBacktraceSize>
size_t
impl_count_function_occurrences_in_backtrace(void * function_address)
{
  throw not_implemented();
}

#else  // defined(_WIN32)

struct count_function_occurrences_in_backtrace_is_implemented : std::true_type {};

#define MY_DEBUG 0

#if MY_DEBUG
static inline
void
print_dl_info(void * function_address)
{
  Dl_info info;
  if (!dladdr(function_address, &info)) {
    SAFE_FWRITE(stderr, "dladdr failed\n");
    exit(1);
  }
  printf("Dl_info for %p:\n", function_address);
  printf("  dli_fname: %s\n", info.dli_fname);
  printf("  dli_fbase: %p\n", info.dli_fbase);
  printf("  dli_sname: %s\n", info.dli_sname);
  printf("  dli_saddr: %p\n", info.dli_saddr);
}
#endif

template<int MaxBacktraceSize>
size_t
impl_count_function_occurrences_in_backtrace(void * function_address)
{
  void * address_list[MaxBacktraceSize];
  int address_length = backtrace(address_list, sizeof(address_list) / sizeof(void *));

  if (0 == address_length) {
    SAFE_FWRITE(stderr, "backtrace() failed\n");
    exit(1);
  }

  size_t number_of_occurences = 0;
  int number_of_failed_dladdr = 0;
  for (int i = 0; i < address_length; ++i) {
    Dl_info info;
    if (!dladdr(address_list[i], &info)) {
      // do nothing, sometimes this fails on syscalls
      number_of_failed_dladdr++;
#if MY_DEBUG
      printf("failed to get dl_info for %p\n", address_list[i]);
#endif
      continue;
    }
#if MY_DEBUG
    printf("comparing %p to %p\n\n", function_address, info.dli_saddr);
    print_dl_info(function_address);
    printf("\n");
    print_dl_info(address_list[i]);
    printf("\n");
#endif
    if (function_address == info.dli_saddr) {
      number_of_occurences++;
    }
  }
  if (number_of_failed_dladdr == address_length) {
    SAFE_FWRITE(stderr, "all calls to dladdr failed, probably something wrong\n");
    exit(1);
  }

  return number_of_occurences;
}

#endif  // defined(_WIN32)

/// Return the number of times a given function pointer occurs the backtrace.
/**
 * This function will only look back into the backtrace as far as
 * `MaxBacktraceSize` allows.
 *
 * \return number of times a function appears in the backtrace
 * \throws osrf_testing_tools_cpp::memory_tools::not_implemented if not implemented
 */
template<
  int MaxBacktraceSize = 64,
  typename FunctionPointerT,
  typename = typename std::enable_if<is_function_pointer<FunctionPointerT>::value>::type>
size_t
count_function_occurrences_in_backtrace(FunctionPointerT function_address)
{
  return impl_count_function_occurrences_in_backtrace<MaxBacktraceSize>(
    reinterpret_cast<void *>(function_address));
}

}  // namespace memory_tools
}  // namespace osrf_testing_tools_cpp

#endif  // MEMORY_TOOLS__COUNT_FUNCTION_OCCURENCES_IN_BACKTRACE_HPP_
