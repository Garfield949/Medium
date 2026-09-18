C++ 特性：

- 头文件的全局变量前加 inline 可避免多重定义，编译器只会生成一个实例；

- 将一个右值绑定到非 const 左值引用的语义：我可能要改变这个临时对象，然而临时值在被改完后就被销毁了，没有意义，所以标准禁止了该做法；
  将一个右值绑定到 const 左值引用的语义：我只对改临时对象进行观察，不对其更改。标准规定会将该临时对象的生命周期延长直至该引用的作用
  域结束；

- 若类型 A 重载了 += 运算符，那么 A1 += A2; 这样的语句会被翻译为 A1.operator+=(A2);
  重载函数的返回值 A& 是为了实现链式运算，比如：
  A a1, a2, a3; a1 += a2 += a3; 被翻译为 a1.operator+=(a2.operator+=(a3)); 内层返回 a2 的引用。

- 命名空间别名：namespace fs = std::filesystem; 注意不能写为 using fs = std::filesystem; std::filesystem 并不是一个具
  体类型；

- std::lround: 将浮点数四舍五入，并将结果转为一个 long 有符号整数；

- 聚合体 Aggregates, 纯数据结构。条件：
  - 无继承关系；
  - 所有成员为public；
  - 无显式构造函数。

- std::vector 的 resize 和 reserve 的区别：
  - resize 分配指定大小的内存并对所有元素进行初始化，即同时改变 capacity 和 size;
  - reserve 只分配指定大小的内存，不进行初始化，只改变 capacity.

- std::unique_ptr<T> data; 若 data 指向一块连续内存，则 unique_ptr 默认析构行为(delete)是未定义行为，可能只会释放首地址的第
  一个 T，其余的全部泄露。
  - 因为模板参数暗示 data 指向的是一个单独的 T;
  - 若 data 指向的是一个数组，则应写为 std::unique_ptr<T[]>，此时默认析构行为是 delete[]，会正确释放所有元素。

- Color(u8 r, u8 g, u8 b, u8 a = 255)
    : data{(static_cast<u32>(a) << 24) | (static_cast<u32>(r) << 16) |
           (static_cast<u32>(g) << 8)  |  static_cast<u32>(b)} {}
  u8 左移会被隐式提升为一个 int. 比方说：
                                    u8 a = 255; u32 b = a << 24;
  a 的位模式为 0xFF，提升后变成 (int)0x0000 00FF. 这里的坑是，左移 24 位会恰好使符号位翻转：0xFF00 0000，这是 UB. 虽然后续位
  模式 0xFF00 0000 还是大概率会被正确解释为一个 u32，但 UB 就让人很不爽。所以这里认为地提升为一个 u32，就不存在符号位这个东西，也
  就没有 UB 这个说法了。

- 四则运算中，若一个操作数为浮点，另一个为整型，那么整型操作数会被隐式地转换为浮点数参与运算。