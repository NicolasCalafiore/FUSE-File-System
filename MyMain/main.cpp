#include <cstdint>
#include <fstream>
#include <iostream>

#include "../libWad/Wad.h"

using namespace std;

int main() {
    Wad *testWad = Wad::loadWad("sample1.wad");
    Wad::printTree(testWad->getRoot());
    cout << testWad->isContent("/Gl/ad/os/cake.jpg") << endl;

    return 0;
}
