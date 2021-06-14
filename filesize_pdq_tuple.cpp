#include <string>
#include <tuple>
#include "pdqsort/pdqsort.h"

int main(int argc, char* argv[]) {
    std::tuple<std::string, int, float> tuples[256];
    pdqsort(&tuples[0], &tuples[256]);
    return 0;
}
