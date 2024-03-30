#include <fstream>
#include <iostream>

#include "io-util.hh"
#include "stream-reader.hh"
#include "usd-to-json.hh"
#include "usda-reader.hh"

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "Need input.usda/.usdc/.usdz\n";
    exit(-1);
  }

  std::string filename = argv[1];

  tinyusdz::Stage stage;
  std::string warn;
  std::string err;

  bool ret = tinyusdz::LoadUSDFromFile(filename, &stage, &warn, &err);

  if (!warn.empty()) {
    std::cerr << "WARN: " << warn << "\n";
  }

  if (!err.empty()) {
    std::cerr << "ERR: " << err << "\n";
  }

  if (!ret) {
    std::cerr << "Failed to load USD file: " << filename << "\n";
    return EXIT_FAILURE;
  }

  nonstd::expected<std::string, std::string> jret = ToJSON(stage);

  if (!jret) {
    std::cerr << jret.error();
    return -1;
  }

  std::cout << *jret << "\n";

  return 0;
}
