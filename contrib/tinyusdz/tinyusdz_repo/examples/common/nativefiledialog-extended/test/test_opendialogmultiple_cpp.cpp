#include "nfd.hpp"

#include <iostream>

/* this test should compile on all supported platforms */
/* this demonstrates the thin C++ wrapper */

int main() {
    // initialize NFD
    NFD::Guard nfdGuard;

    // auto-freeing memory
    NFD::UniquePathSet outPaths;

    // prepare filters for the dialog
    nfdfilteritem_t filterItem[2] = {{"Source code", "c,cpp,cc"}, {"Headers", "h,hpp"}};

    // show the dialog
    nfdresult_t result = NFD::OpenDialogMultiple(outPaths, filterItem, 2);
    if (result == NFD_OKAY) {
        std::cout << "Success!" << std::endl;

        nfdpathsetsize_t numPaths;
        NFD::PathSet::Count(outPaths, numPaths);

        nfdpathsetsize_t i;
        for (i = 0; i < numPaths; ++i) {
            NFD::UniquePathSetPath path;
            NFD::PathSet::GetPath(outPaths, i, path);
            std::cout << "Path " << i << ": " << path.get() << std::endl;
        }
    } else if (result == NFD_CANCEL) {
        std::cout << "User pressed cancel." << std::endl;
    } else {
        std::cout << "Error: " << NFD::GetError() << std::endl;
    }

    // NFD::Guard will automatically quit NFD.
    return 0;
}
