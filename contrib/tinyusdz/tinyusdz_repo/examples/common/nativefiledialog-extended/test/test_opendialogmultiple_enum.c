#include <nfd.h>

#include <stdio.h>
#include <stdlib.h>

/* this test should compile on all supported platforms */

int main(void) {
    // initialize NFD
    // either call NFD_Init at the start of your program and NFD_Quit at the end of your program,
    // or before/after every time you want to show a file dialog.
    NFD_Init();

    const nfdpathset_t* outPaths;

    // prepare filters for the dialog
    nfdfilteritem_t filterItem[2] = {{"Source code", "c,cpp,cc"}, {"Headers", "h,hpp"}};

    // show the dialog
    nfdresult_t result = NFD_OpenDialogMultiple(&outPaths, filterItem, 2, NULL);

    if (result == NFD_OKAY) {
        puts("Success!");

        // declare enumerator (not a pointer)
        nfdpathsetenum_t enumerator;

        NFD_PathSet_GetEnum(outPaths, &enumerator);
        nfdchar_t* path;
        unsigned i = 0;
        while (NFD_PathSet_EnumNext(&enumerator, &path) && path) {
            printf("Path %u: %s\n", i++, path);

            // remember to free the pathset path with NFD_PathSet_FreePath (not NFD_FreePath!)
            NFD_PathSet_FreePath(path);
        }

        // remember to free the pathset enumerator memory (before freeing the pathset)
        NFD_PathSet_FreeEnum(&enumerator);

        // remember to free the pathset memory (since NFD_OKAY is returned)
        NFD_PathSet_Free(outPaths);
    } else if (result == NFD_CANCEL) {
        puts("User pressed cancel.");
    } else {
        printf("Error: %s\n", NFD_GetError());
    }

    // Quit NFD
    NFD_Quit();

    return 0;
}
