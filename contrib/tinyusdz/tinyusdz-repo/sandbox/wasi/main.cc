#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include "tinyusdz.hh"

static std::string GetFileExtension(const std::string &filename) {
  if (filename.find_last_of(".") != std::string::npos)
    return filename.substr(filename.find_last_of(".") + 1);
  return "";
}

static std::string str_tolower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 // static_cast<int(*)(int)>(std::tolower)         // wrong
                 // [](int c){ return std::tolower(c); }           // wrong
                 // [](char c){ return std::tolower(c); }          // wrong
                 [](unsigned char c) { return std::tolower(c); }  // correct
  );
  return s;
}

bool LoadModelFromString(const std::vector<uint8_t> &content,
                         const std::string &filename, tinyusdz::Stage *stage) {
  std::string ext = str_tolower(GetFileExtension(filename));

  std::string warn;
  std::string err;

  bool ret = tinyusdz::LoadUSDFromMemory(content.data(), content.size(), filename, stage, &warn, &err);
  if (!warn.empty()) {
    std::cerr << "WARN : " << warn << "\n";
  }
  if (!err.empty()) {
    std::cerr << "ERR : " << err << "\n";
  }

  if (!ret) {
    std::cerr << "Failed to load USD(USDA, USDC or USDZ) file: " << filename << "\n";
    return false;
  }

  return true;
}

#if 0
// It looks WASI does not provide fopen(), so use open() to read file content.
std::vector<uint8_t> ReadFile(const char *arg) {
  std::ostringstream ss;

  ssize_t n;
  char buf[BUFSIZ];

  int in = open(arg, O_RDONLY);
  if (in < 0) {
    fprintf(stderr, "error opening input %s: %s\n", arg, strerror(errno));
    exit(1);
  }

  while ((n = read(in, buf, BUFSIZ)) > 0) {
    char *ptr = buf;
    ss.write(ptr, n);
  }

  std::string s = ss.str();
  size_t len = s.size();

  std::vector<uint8_t> dst;
  dst.resize(len);

  //std::cout << "input = " << s << "\n";

  memcpy(dst.data(), s.data(), len);

  return dst;
}
#endif

int main(int argc, char **argv) {
  std::cout << "bora\n";

  if (argc > 1) {
    std::string filename = argv[1];
#if 1
    tinyusdz::Stage stage;
    {
      std::string warn;
      std::string err;
      bool ret = tinyusdz::LoadUSDFromFile(filename, &stage, &warn, &err);

      if (!warn.empty()) {
        std::cerr << "WARN : " << warn << "\n";
      }
      if (!err.empty()) {
        std::cerr << "ERR : " << err << "\n";
      }

      if (!ret) {
        return EXIT_FAILURE;
      }
    }
#else
    //std::vector<uint8_t> content = ReadFile(filename.c_str());
    //if (content.empty()) {
    //  std::cerr << "File is empty or failed to read: " << filename << "\n";
    //}

    //tinyusdz::Stage stage;
    //bool ret = LoadModelFromString(content, filename, &stage);
    //if (!ret) {
    //  std::cerr << "Load failed.\n";
    //  return EXIT_FAILURE;
    //}
#endif

    std::cout << "Load oK\n";

    std::string outs = stage.ExportToString();
    std::cout << outs;
  } else {
    std::cout << "Need input USD filename(.usda/.usdc/.usdz)\n";
  }

  return 0;
}
