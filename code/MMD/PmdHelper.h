#ifndef PMDHELPER_INC
#define PMDHELPER_INC
#include <string>
#include <unordered_map>
#include <assimp/IOStream.hpp>
namespace pmd {
    class PmdHelper {
    public:
        static std::string ShitfJISToUtf8(std::string input);
        static std::string ReadString(Assimp::IOStream* ioStream, int size);
        static std::unordered_map<uint16_t, uint16_t> Sjis2Unicode;
    };
}
#endif //PMDHELPER_INC
