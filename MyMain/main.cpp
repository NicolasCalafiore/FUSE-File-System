#include <cstdint>
#include <fstream>
#include <iostream>

#include "../libWad/Wad.h"

using namespace std;

int main() {
    Wad *testWad = Wad::loadWad("sample1.wad");

    std::string testPath = "/Ex";
    testWad->createDirectory(testPath);
    testPath += "/ad";
    testWad->createDirectory(testPath );
    Wad::printTree(testWad->getRoot());

    cout << testWad->isContent(testPath) << endl;
    return 0;
}
