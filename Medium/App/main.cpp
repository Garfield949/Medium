///
/// @file main.cpp
/// @brief 程序主入口
///
#include <iostream>

#include "App.h"

int main() {
  Medium::App app;

  if(!app.init()) return -1;

  try {
    app.loop();
  } catch (const std::exception& e) {
    std::cerr << "Runtime Error: " << e.what() << std::endl;
    return -1;
  }

  return 0;
}