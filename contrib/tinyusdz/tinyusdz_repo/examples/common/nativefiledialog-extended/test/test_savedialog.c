#include <nfd.h>

#include <stdio.h>
#include <stdlib.h>

/* this test should compile on all supported platforms */

int main(void) {
    // initialize NFD
    // either call NFD_Init at the start of your program and NFD_Quit at the end of your program,
    // or before/after every time you want to show a file dialog.
    NFD_Init();

    nfdchar_t* savePath;

    // prepare filters for the dialog
    nfdfilteritem_t filterItem[2] = {{"Source code", "c,cpp,cc"}, {"Header", "h,hpp"}};

    // show the dialog
    nfdresult_t result = NFD_SaveDialog(&savePath, filterItem, 2, NULL, "Untitled.c");
    if (result == NFD_OKAY) {
        puts("Success!");
        puts(savePath);
        // remember to free the memory (since NFD_OKAY is returned)
        NFD_FreePath(savePath);
    } else if (result == NFD_CANCEL) {
        puts("User pressed cancel.");
    } else {
        printf("Error: %s\n", NFD_GetError());
    }

    // Quit NFD
    NFD_Quit();

    return 0;
}
