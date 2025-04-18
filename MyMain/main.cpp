#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

#include "../libWad/Wad.h"

using namespace std;

int main() {
    Wad *testWad = Wad::loadWad("sample1.wad");
    Wad::printTree(testWad->getRoot());

    vector<string> entries;
    cout << testWad->getDirectory("/", &entries);
    return 0;
}