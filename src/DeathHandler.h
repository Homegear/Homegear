/*

  Copyright (c) 2012, Samsung R&D Institute Russia
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

/*! @file death_handler.h
 *  @brief Declaration of the SIGSEGV/SIGABRT handler which prints the debug stack
 *  trace.
 *  @author Markovtsev Vadim <gmarkhor@gmail.com>
 *  @version 1.0
 *  @license Simplified BSD License
 *  @copyright 2012 Samsung R&D Institute Russia, 2016 Moscow Institute of Physics and Technology
 */

/*! @mainpage SIGSEGV/SIGABRT handler which prints the debug stack trace.
 *  Example
 *  =======
 *  ~~~~{.cc}
 *  #include "death_handler.h"
 *
 *  int main() {
 *    Debug::DeathHandler dh;
 *    int* p = NULL;
 *    *p = 0;
 *    return 0;
 *  }
 *  ~~~~
 *
 *  Underlying code style is very similar to [Google C++ Style Guide](http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml). It is checked with cpplint.py.
 */

#ifndef DEATH_HANDLER_H_
#define DEATH_HANDLER_H_

#include <stddef.h>
#include <unistd.h>

// We have to override malloc() and free()
extern "C" {
void* __malloc_impl(size_t size);
#ifdef __linux__
void* malloc(size_t size) throw();
void free(void* ptr) throw();
#elif defined(__APPLE__)
void* __malloc_zone(struct _malloc_zone_t* zone, size_t size);
void __free_zone(struct _malloc_zone_t* zone, void* ptr);
#endif
}

#ifdef __linux__
// Comment this out on systems without quick_exit()
#define QUICK_EXIT
#endif

namespace Debug {

/// @brief This class installs a SEGFAULT signal handler to print
/// a nice stack trace and (if requested) generate a core dump.
/// @details In DeathHandler's constructor, a SEGFAULT signal handler
/// is installed via sigaction(). If your program encounters a segmentation
/// fault, the call stack is unwinded with backtrace(), converted into
/// function names with line numbers via addr2line (fork() + execlp()).
/// Addresses from shared libraries are also converted thanks to dladdr().
/// All C++ symbols are demangled. Printed stack trace includes the faulty
/// thread id obtained with pthread_self() and each line contains the process
/// id to distinguish several stack traces printed by different processes at
/// the same time.
class DeathHandler {
 public:
  typedef ssize_t (*OutputCallback)(const char*, size_t);

  /// @brief Installs the SIGSEGV/etc. signal handler.
  /// @param altstack If true, allocate and use a dedicated signal handler stack.
  /// backtrace() will report nothing then, but the handler will survive a stack
  /// overflow.
  DeathHandler(bool altstack = false);
  /// @brief This is called on normal program termination. Previously installed
  /// SIGSEGV and SIGABRT signal handlers are removed.
  ~DeathHandler();

  /// @brief Sets the value of cleanup property.
  /// @details If cleanup is set to true, program attempts to run all static
  /// destructors and atexit() callbacks before terminating. If
  /// generate_core_dump is set to true, this property is ignored.
  /// @note Default value of this property is true.
  bool cleanup() const;

  /// @brief Returns the value of cleanup property.
  /// @details If cleanup is set to true, program attempts to run all static
  /// destructors and atexit() callbacks before terminating. If
  /// generate_core_dump is set to true, this property is ignored.
  /// @note Default value of this property is true.
  void set_cleanup(bool value);

  /// @brief Returns the current value of generate_core_dump property.
  /// @details If generate_core_dump is set to true, a core dump will
  /// be generated when the program terminates. This behavior
  /// is implemented using abort(). cleanup property is ignored.
  /// @note You need to set ulimit to a value different from the default 0
  /// (for example, "ulimit -c unlimited") to enable core dumps generation
  /// on your system.
  /// @note Default value of this property is true.
  bool generate_core_dump() const;

  /// @brief Sets the value of generate_core_dump property.
  /// @details If generate_core_dump is set to true, a core dump will
  /// be generated when the program terminates. This behavior
  /// is implemented using abort(). cleanup property is ignored.
  /// @note You need to set ulimit to a value different from the default 0
  /// (for example, "ulimit -c unlimited") to enable core dumps generation
  /// on your system.
  /// @note Default value of this property is true.
  void set_generate_core_dump(bool value);

#ifdef QUICK_EXIT
  /// @brief Returns the value of quick_exit property.
  /// @details If quick_exit is set to true, program will be terminated with
  /// quick_exit() call. generate_core_dump and cleanup properties are
  /// ignored.
  /// @note Default value is false.
  bool quick_exit() const;

  /// @brief Sets the value of quick_exit property.
  /// @details If quick_exit is set to true, program will be terminated with
  /// quick_exit() call. generate_core_dump and cleanup properties are
  /// ignored.
  /// @note Default value is false.
  void set_quick_exit(bool value);
#endif

  /// @brief Returns the depth of the stack trace.
  /// @note Default value is 16.
  int frames_count() const;

  /// @brief Sets the depth of the stack trace. Accepted range is 1..100.
  /// @note Default value is 16.
  void set_frames_count(int value);

  /// @brief Returns the value indicating whether to shorten stack trace paths
  /// by cutting off the common root between each path and the current working
  /// directory.
  /// @note Default value is true.
  bool cut_common_path_root() const;

  /// @brief Sets the value indicating whether to shorten stack trace paths
  /// by cutting off the common root between each path and the current working
  /// directory.
  /// @note Default value is true.
  void set_cut_common_path_root(bool value);

  /// @brief Returns the value indicating whether to shorten stack trace paths
  /// by cutting off the relative part (e.g., "../../..").
  /// @note Default value is true.
  bool cut_relative_paths() const;

  /// @brief Sets the value indicating whether to shorten stack trace paths
  /// by cutting off the relative part (e.g., "../../..").
  /// @note Default value is true.
  void set_cut_relative_paths(bool value);

  /// @brief Returns the value indicating whether to append the process id
  /// to each stack trace line.
  /// @note Default value is false.
  bool append_pid() const;

  /// @brief Sets the value indicating whether to append the process id
  /// to each stack trace line.
  /// @note Default value is false.
  void set_append_pid(bool value);

  /// @brief Returns the value indicating whether to color the output
  /// with ANSI escape sequences.
  /// @note Default value is true.
  bool color_output() const;

  /// @brief Sets the value indicating whether to color the output
  /// with ANSI escape sequences.
  /// @note Default value is true.
  void set_color_output(bool value);

  /// @brief Returns the value indicating whether to do a thread-safe
  /// stack trace printing, stopping all running threads by forking.
  /// @note Default value is true.
  bool thread_safe() const;

  /// @brief Sets the value indicating whether to do a thread-safe stack trace
  /// printing, stopping all running threads by forking.
  /// @note Default value is true.
  void set_thread_safe(bool value);

  /// @brief Returns the current output callback.
  /// @note Default value is write to stderr.
  OutputCallback output_callback() const;

  /// @brief Changes output callback (that is, how to write the trace, etc.).
  /// @note Default value is write to stderr.
  void set_output_callback(OutputCallback value);

 private:
  friend void* ::__malloc_impl(size_t);
#ifdef __linux__
  friend void* ::malloc(size_t) throw();
  friend void ::free(void*) throw();
#elif defined(__APPLE__)
  friend void* ::__malloc_zone(struct _malloc_zone_t*, size_t);
  friend void ::__free_zone(struct _malloc_zone_t*, void*);
#endif
  /// @brief Reentrant printing to stderr.
  inline static void print(const char* msg, size_t len = 0);

  /// @brief The size of the preallocated memory to use in the signal handler.
  static const size_t kNeededMemory;

  static void HandleSignal(int sig, void* info, void* secret);

  /// @brief Used to workaround backtrace() usage of malloc().
  static void* malloc_;
  static void* free_;
  static bool heap_trap_active_;

  static bool generate_core_dump_;
  static bool cleanup_;
#ifdef QUICK_EXIT
  static bool quick_exit_;
#endif
  static int frames_count_;
  static bool cut_common_path_root_;
  static bool cut_relative_paths_;
  static bool append_pid_;
  static bool color_output_;
  static bool thread_safe_;
  static OutputCallback output_callback_;
  /// @brief The preallocated memory to use in the signal handler.
  static char* memory_;
};

}  // namespace Debug
#endif  // DEATH_HANDLER_H_
