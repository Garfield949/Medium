///
/// @file   ShaderType.h
/// @author Garfield949
/// @date   2026/3/2 13:17
/// @brief
///
#pragma once

#include "Shaders.h"

namespace Medium {

    // - Overloads, 利用 C++17 特性将多个匿名 Lambda 聚合为一个完整的 Visitor 仿函数对象。
    // - 可变参数模板：Ts... 为模板参数包；
    // - 右侧进行参数解包，实现对传入的 Lambda 进行多重继承；
    // - 要注意的背景：Lambda 是一种语法糖，实际上会被编译器翻译为一个匿名结构体；
    // - C++17 特性：using Ts::operator()... 父类 Lambda 都会有同名函数 operator()，子
    //   类直接调用会导致歧义。该特性将基类的 operator() 全部提升到子类的作用域。C++ 的重载决
    //   议根据传入的类型匹配对应的 operator().
    template<typename... Ts> struct overloads : Ts... {
        using Ts::operator()...;
        // 参数解包后会被编译器翻译为
        // using Lambda1::operator();
        // using Lambda2::operator();
        // using Lambda3::operator();
        // ....
    };

    // - C++17 引入的推导指引。在此之前必须写为 overloads<LambdaA, LambdaB>{lmda, lmdb};
    // - 现在可以写为 overloads{lmda, lmdb}; 会自动推导出 overloads<LambdaA, LambdaB>.
    template<typename... Ts>
    overloads(Ts...) -> overloads<Ts...>;

    using VariantShader = std::variant<BlinnPhongShader>;

} // namespace Medium
