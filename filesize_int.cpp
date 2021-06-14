#include "bitsetsort.h"

int main(int argc, char* argv[]) {
    int a[256];
    for (int i = 0; i < 256; i++) {
        a[i] = 0;
    }
    stdext::bitsetsort(&a[0], &a[256]);
    return 0;
}
