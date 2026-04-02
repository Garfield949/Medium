///
/// @file   Config.cpp
/// @author Garfield949
/// @date   2026/4/2 15:22
/// @brief  
///
#include "Config.h"

#include <iostream>
#include <string_view>
#include <fmt/base.h>

// 定义该宏禁止 toml++ 抛出异常
#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>

using namespace std::string_view_literals;

namespace Medium {

  AppCfg loadAppConfig(const std::filesystem::path& toml_dir) {
    toml::parse_result result = toml::parse_file((toml_dir / "app.toml").string());
    if(!result) {
      fmt::print(stderr, "\nloadAppConfig - toml++::{}\n", result.error().description());
      fmt::print("Falling back to default.\n");
    }

    toml::table toml_app = result.table();

    auto node_window = toml_app["window"];
    // 若 title 为空，则运算符 operator""sv 会自动将 "" 里的字符串转为一个 string_view.
    // 模板函数 operator""sv 的声明在命名空间 std::string_view_literals 内。
    std::optional<std::string> title = node_window["title"].value_or("Medium Canvas");
    std::optional<u32>         width  = node_window["width"].value_or(1440);
    std::optional<u32>         height = node_window["height"].value_or(900);

    auto node_render = toml_app["render"];
    std::optional<u32> shadow_map_res = node_render["shadow_map_res"].value_or(2048);

    return {
      WindowCfg {
        *title, *width, *height
      },
      RenderCfg {
        *shadow_map_res
      }
    };
  }

} // namespace Medium
