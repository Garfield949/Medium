///
/// @file   Config.cpp
/// @author Garfield949
/// @date   2026/4/2 15:22
/// @brief  
///
#include "Config.h"

// 定义该宏禁止 toml++ 抛出异常
#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>
#include <fmt/base.h>

namespace fs = std::filesystem;

namespace Medium {

AppCfg AppCfg::loadFromToml(const fs::path& config_dir) {
  const fs::path toml_path = config_dir / "app_config.toml";
  
  // 初始化为默认配置
  AppCfg app_cfg{};
  app_cfg.related_toml_path = toml_path;

  // 若配置文件不存在，则创建新的 app_config.toml 并注入默认配置
  if(!fs::exists(toml_path)) {
    app_cfg.saveToTomlFile();
    return app_cfg;
  }

  // 解析失败原因一般是 toml 语法错误，直接返回默认配置，toml++ 会打印错误原因
  toml::parse_result app_toml = toml::parse_file(toml_path.string());
  if(!app_toml) {
    fmt::print(stderr, "\nloadAppConfig - toml++::{}\n", app_toml.error().description());
    fmt::print("Falling back to default.\n");
    return app_cfg;
  }

  toml::table toml_app = app_toml.table();

  // value 返回的是 std::optional<T>
  // 注：若 toml 里写了类似 width = -1 这样的值，那么进行 u32 转换是不安全的，value 会返回一个
  // std::nullopt，if 判断失败而回退到默认值。
  auto window = toml_app["window"];
  if(auto v = window["title"].value<std::string>()) app_cfg.window.title  = *v;
  if(auto v = window["width"].value<u32>())         app_cfg.window.w  = *v;
  if(auto v = window["height"].value<u32>())        app_cfg.window.h = *v;

  auto shadow = toml_app["render"]["shadow"];
  if(auto v = shadow["resolution"].value<u32>()) app_cfg.render.shadow.shadow_map_res = *v;

  return app_cfg.sanitized();
}  

// 注：toml++ 的底层是 std::map，只支持字典序，所以不论往 toml::table 里 insert 的顺序是
// 怎样，最终序列化出来键值都只会按字典序排，不利于阅读。所以这里直接手动实现序列化 (toml11 支
// 持有序表，但是那个库太重就不考虑了）。
std::string AppCfg::toTomlString() const {
  return fmt::format(
  "[window]\n"
  "title = \"{}\"\n"
  "width = {}\n"
  "height = {}\n\n"
  "[render.shadow]\n"
  "resolution = {}\n",
  window.title, window.w, window.h,
  render.shadow.shadow_map_res);
}

AppCfg AppCfg::sanitized() const noexcept {
  AppCfg res = *this;
  
  res.window.w = std::max(Config::kMinWindowWidth,  window.w);
  res.window.h = std::max(Config::kMinWindowHeight, window.h);
  
  res.render.shadow.shadow_map_res =
    std::max(Config::kMinShadowRes, render.shadow.shadow_map_res);
  
  return res;
}

// 暂不考虑写文件的异常处理，感觉不会出问题
void AppCfg::saveToTomlFile() const {
  // 暂不考虑 create_directories, 该方法的作用是若指定目录的前几级目录都不存在，会一并创建
  if(!fs::exists(related_toml_path.parent_path()))
    fs::create_directory(related_toml_path.parent_path());

  std::ofstream out{related_toml_path};
  out << toTomlString();
}

} // namespace Medium
