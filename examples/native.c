#include "../include/RozmAPI.h"
#include <stdio.h>
int main(void) {
    RozmHandle tts = NULL;
    int result = Rozm_CreateW(NULL, &tts);
    if (!result) {
        result = Rozm_SaveWavW(tts,
            L"\u041f\u0440\u0438\u0432\u0456\u0442, \u0441\u0432\u0456\u0442\u0435!",
            1, 5, L"native-speech.wav");
    }
    if (result) fprintf(stderr, "%s\n", Rozm_GetLastError());
    Rozm_Destroy(tts);
    return result;
}
