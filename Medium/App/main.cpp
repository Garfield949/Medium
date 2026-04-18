///
/// @file main.cpp
/// @brief 程序主入口
///
#include <iostream>

#include "App.h"

int main() {
  App app;

  if(!app.init()) return -1;

  app.loop();

  app.quit();

  return 0;
}