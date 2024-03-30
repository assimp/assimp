#include <iostream>
#include "nfd.hpp"

/* this test should compile on all supported platforms */
/* this demonstrates the thin C++ wrapper */

int main() {
    // initialize NFD
    NFD::Guard nfdGuard;

    // auto-freeing memory
    NFD::UniquePath outPath;

    // prepare filters for the dialog
    nfdfilteritem_t filterItem[2] = {{"Source code", "c,cpp,cc"}, {"Headers", "h,hpp"}};

    // show the dialog
    nfdresult_t result = NFD::OpenDialog(outPath, filterItem, 2);
    if (result == NFD_OKAY) {
        std::cout << "Success!" << std::endl << outPath.get() << std::endl;
    } else if (result == NFD_CANCEL) {
        std::cout << "User pressed cancel." << std::endl;
    } else {
        std::cout << "Error: " << NFD::GetError() << std::endl;
    }

    // NFD::Guard will automatically quit NFD.
    return 0;
}
