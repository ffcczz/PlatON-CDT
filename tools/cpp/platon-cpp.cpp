#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <fstream>

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

#include "platon/utils.hpp"

#include "options.hpp"

#if defined(__WIN32) || defined(__WIN32__) || defined(WIN32)
#define CLANG "clang"
#else
#define CLANG "clang-7"
#endif

const std::string kCompilerName = "platon-cpp";

using namespace llvm;

int main(int argc, const char** argv) {
  for (auto i = 0; i < argc; i++) {
    if (argv[i] == std::string("-v")) {
      platon::cdt::runtime::exec_subprogram(CLANG, {"-v"});
      return 0;
    }
  }

  cl::SetVersionPrinter([](llvm::raw_ostream& os) {
    os << kCompilerName << " version "
       << "${VERSION_FULL}"
       << "\n";
  });
  cl::ParseCommandLineOptions(
      argc, argv, kCompilerName + " (Platon C++ -> WebAssembly compiler)");
  auto opts = CreateOptions();

  if (opts.abigen) {
    platon::cdt::runtime::exec_subprogram("platon-abigen", opts.abigen_opts);
    if (!llvm::sys::fs::exists(opts.abi_filename)) {
      return -1;
    }
  }

  std::vector<std::string> outputs;
  try {
    for (auto input : opts.inputs) {
      std::vector<std::string> new_opts = opts.compiler_opts;
      llvm::SmallString<64> res;
      llvm::sys::path::system_temp_directory(true, res);
      std::string tmp_file = std::string(res.c_str()) + "/" +
                             llvm::sys::path::filename(input).str();
      std::string output;

      if (llvm::sys::fs::exists(tmp_file)) {
        input = tmp_file;
      }
      output = tmp_file + ".o";

      new_opts.insert(new_opts.begin(), input);

      if (!opts.link) {
        output = opts.output_filename.empty() ? "a.out" : opts.output_filename;
      }

      new_opts.insert(new_opts.begin(), "-o " + output);
      outputs.push_back(output);
      std::cout << "clang  new_opts start  \n";
      for(auto newOPt = new_opts.begin(); newOPt != new_opts.end(); newOPt++){
          std::cout << *newOPt  <<  "     ";
      }
      std::cout << std::endl;

      if (!platon::cdt::runtime::exec_subprogram(CLANG, new_opts)) {
        llvm::sys::fs::remove(tmp_file);
        return -1;
      }
      //llvm::sys::fs::remove(tmp_file);
    }
  } catch (std::runtime_error& err) {
    llvm::errs() << err.what() << '\n';
    return -1;
  }

  if (opts.link) {
    std::vector<std::string> new_opts = opts.ld_opts;
    for (const auto& output : outputs) {
      new_opts.insert(new_opts.begin(), std::string(" ") + output + " ");
    }

    if (opts.abigen) {
      std::fstream fs(opts.exports_filename);
      std::string line;
      while (std::getline(fs, line)) {
        new_opts.emplace_back(" --export " + line + " ");
      }
    } else {
      new_opts.emplace_back(" --export _Z4mainiPPc ");
    }

    std::cout <<  "  exports_filename  " <<  opts.exports_filename  << std::endl;
    std::cout <<  "  abi_filename  " <<  opts.abi_filename  << std::endl;
    std::cout <<  "  abidef_output  " <<  opts.abidef_output  << std::endl;
    std::cout <<  "  output_filename  " <<  opts.output_filename  << std::endl;
    std::cout << "exports_filename   "  << opts.exports_filename << std::endl;
    std::cout << "ld  new_opts start  \n";
    for(auto newOPt = new_opts.begin(); newOPt != new_opts.end(); newOPt++){
        std::cout << *newOPt  <<  "     ";
    }
    std::cout << std::endl;
    if (!platon::cdt::runtime::exec_subprogram("platon-ld", new_opts)) {
      for (const auto& output : outputs) {
        llvm::sys::fs::remove(output);
      }
      return -1;
    }

    if (!llvm::sys::fs::exists(opts.output_filename)) {
      return -1;
    }
  }
  return 0;
}
