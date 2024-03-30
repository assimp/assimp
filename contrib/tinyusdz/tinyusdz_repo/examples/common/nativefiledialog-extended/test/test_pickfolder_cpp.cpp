#include "nfd.hpp"

#include <iostream>

/* this test should compile on all supported platforms */
/* this demonstrates the thin C++ wrapper */

int main() {
    // initialize NFD
    NFD::Guard nfdGuard;

    // auto-freeing memory
    NFD::UniquePath outPath;

    // show the dialog
    nfdresult_t result = NFD::PickFolder(outPath);
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
