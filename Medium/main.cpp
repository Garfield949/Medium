///
/// @file main.cpp
/// @brief 程序主入口
///
#include <iostream>

#include "Application.h"

int main() {
    Medium::Application app(1100, 900, "Medium Canvas");

    if(!app.init()) return -1;

    try {
        app.loop();
    } catch (const std::exception& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}