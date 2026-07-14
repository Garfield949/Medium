///
/// @file main.cpp
/// @brief 程序主入口
///
#include "App.h"

int main() {
  Medium::App app;

  if(app.init()) app.loop();

  app.quit();

  return 0;
}