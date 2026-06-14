///
/// @file   Log.h
/// @author Garfield949
/// @date   2026/6/14 22:30
/// @brief  
///
#pragma once

#include <string_view>

#include <fmt/base.h>
#include <fmt/color.h>

inline void printLogPrefix(std::string_view type_name,
                           std::string_view func_name,
                           bool err) {
  auto color = err ? fmt::color::red : fmt::color::green;
  fmt::print(fmt::emphasis::bold | fg(color), "{}::{} - ", type_name, func_name);
}
