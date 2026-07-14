///
/// @file   RenderCommand.h
/// @author Garfield949
/// @date   2026/3/18 23:33
/// @brief  该文件定义 MeshNode 待提交给渲染队列的指令，提交单位为 SubMesh
///
#pragma once

#include "Base/Types.h"

namespace Medium {

class Material;
class Mesh;
struct SubMesh;

/// @struct RenderCommand
/// @details 1. Mesh 用于让 SubMesh 查询索引和实际顶点数据；
///          2. SubMesh 为当前命令待渲染的网格；
///          3. 相同材质的网格是绑定在一个 SubMesh 的；
///          4. 提交 SubMesh 所属 MeshNode 的世界矩阵，用于后续计算；
///          5. 渲染队列根据实际情况将 sort_key 作为条件进行指令排序；
struct RenderCommand {
  // 注意，下列指针均为底层 const，修饰的是它们指向的对象而非它们自身，所以编译器不会删除默认赋值运算符。
  const Mesh*     mesh     = nullptr;
  const SubMesh*  submesh  = nullptr;
  const Material* material = nullptr;
  Mat4f mat_world;
  f32   sort_key;
};

} // namespace Medium
