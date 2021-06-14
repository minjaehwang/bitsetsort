#include <string>
#include <tuple>
#include "bitsetsort.h"

int main(int argc, char* argv[]) {
    std::tuple<std::string, int, float> tuples[256];
    stdext::bitsetsort(&tuples[0], &tuples[256]);
    return 0;
}
