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