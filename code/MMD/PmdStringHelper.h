#pragma once
#include <map>
#include <string>
#include <assimp/DefaultIOSystem.h>
using namespace::Assimp;
namespace pmd {
	class PmdStringHelper
	{
	public:
        typedef std::map<uint16_t, uint16_t> ConvMap;
		static ConvMap convMap;
		static bool convMapReady;
		static void SetupConvMap();
		static uint16_t ConvertChar(uint16_t index);
		static std::string Sj2Utf8(const std::string &input);
        static std::string ReadString(IOStream *stream, unsigned int size, bool utf16 = false);
	};
}

