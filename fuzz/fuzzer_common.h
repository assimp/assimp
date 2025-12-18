#pragma once

#include <assimp/Importer.hpp>
#include <assimp/importerdesc.h>
#include <cstring>
#include <vector>

namespace AssimpFuzz {

// Unregisters all loaders except the ones matching the given extension.
// Returns true if at least one loader was kept.
inline bool ForceFormat(Assimp::Importer& importer, const char* targetExtension) {
    size_t count = importer.GetImporterCount();
    std::vector<Assimp::BaseImporter*> toRemove;
    bool found = false;

    for (size_t i = 0; i < count; ++i) {
        const aiImporterDesc* desc = importer.GetImporterInfo(i);
        Assimp::BaseImporter* imp = importer.GetImporter(i);
        
        if (!desc || !imp) continue;

        // Check if the importer supports the target extension
        // mFileExtensions is a space-separated list (e.g., "obj mod")
        // We wrap target in spaces or check bounds to be precise, 
        // but for fuzzing, a simple strstr is usually sufficient 
        // if the target string is unique enough (e.g. "gltf", "obj").
        // A more robust check:
        
        bool isTarget = false;
        const char* extList = desc->mFileExtensions;
        if (!extList) {
            toRemove.push_back(imp);
            continue;
        }
        const size_t targetLen = strlen(targetExtension);

        const char* p = extList;
        while ((p = strstr(p, targetExtension)) != nullptr) {
            // Check boundaries
            const char prev = (p == extList) ? ' ' : *(p - 1);
            const char next = *(p + targetLen);
            
            if (prev == ' ' && (next == ' ' || next == '\0')) {
                isTarget = true;
                break;
            }
            p++;
        }

        if (isTarget) {
            found = true;
        } else {
            toRemove.push_back(imp);
        }
    }

    for (auto* imp : toRemove) {
        importer.UnregisterLoader(imp);
    }

    return found;
}

}
