#include "pdqsort/pdqsort.h"

int main(int argc, char* argv[]) {
    int a[256];
    for (int i = 0; i < 256; i++) {
        a[i] = 0;
    }
    pdqsort(&a[0], &a[256]);
    return 0;
}
