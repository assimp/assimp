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

        nfdpathsetsize_t numPaths;
        NFD_PathSet_GetCount(outPaths, &numPaths);

        nfdpathsetsize_t i;
        for (i = 0; i < numPaths; ++i) {
            nfdchar_t* path;
            NFD_PathSet_GetPath(outPaths, i, &path);
            printf("Path %i: %s\n", (int)i, path);

            // remember to free the pathset path with NFD_PathSet_FreePath (not NFD_FreePath!)
            NFD_PathSet_FreePath(path);
        }

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
