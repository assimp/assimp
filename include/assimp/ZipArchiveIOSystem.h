#ifndef AI_ZIPARCHIVEIOSYSTEM_H_INC
#define AI_ZIPARCHIVEIOSYSTEM_H_INC

#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>

namespace Assimp {
    class ZipArchiveIOSystem : public IOSystem {
    public:
        //! Open a Zip using the proffered IOSystem
        ZipArchiveIOSystem(IOSystem* pIOHandler, const char *pFilename, const char* pMode = "r");
        ZipArchiveIOSystem(IOSystem* pIOHandler, const std::string& rFilename, const char* pMode = "r");
        virtual ~ZipArchiveIOSystem();
        bool Exists(const char* pFilename) const override;
        char getOsSeparator() const override;
        IOStream* Open(const char* pFilename, const char* pMode = "rb") override;
        void Close(IOStream* pFile) override;

        // Specific to ZIP
        //! The file was opened and is a ZIP
        bool isOpen() const;

        //! Get the list of all files with their simplified paths
        //! Intended for use within Assimp library boundaries
        void getFileList(std::vector<std::string>& rFileList) const;

        //! Get the list of all files with extension (must be lowercase)
        //! Intended for use within Assimp library boundaries
        void getFileListExtension(std::vector<std::string>& rFileList, const std::string& extension) const;

        static bool isZipArchive(IOSystem* pIOHandler, const char *pFilename);
        static bool isZipArchive(IOSystem* pIOHandler, const std::string& rFilename);

    private:
        class Implement;
        Implement *pImpl = nullptr;
    };
} // Namespace Assimp

#endif // AI_ZIPARCHIVEIOSYSTEM_H_INC
