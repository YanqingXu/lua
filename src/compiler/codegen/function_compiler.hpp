#pragma once

/**
 * @file function_compiler.hpp
 * @brief 代码生成器的函数原型编译边界
 */

#include "common/types.hpp"
#include "compiler/ast.hpp"
#include "compiler/codegen/codegen_context.hpp"

namespace Lua {

class CodeGenerator;
class Proto;

/**
 * @brief 负责函数级代码生成生命周期
 *
 * 代码生成器仍是公共外观。函数编译器集中处理子函数原型创建、参数绑定、上值
 * 元数据、闭包上值指令与局部调试元数据附加。返回的函数原型指针由垃圾回收器管理，属于
 * 非拥有型观察指针。
 */
class FunctionCompiler {
public:
    explicit FunctionCompiler(CodeGenerator& owner) noexcept
        : owner_(owner) {}

    [[nodiscard]] CompiledFunction compile(const Vec<Str>& params, bool isVararg, const Vec<StmtPtr>& body,
                                           i32 linedefined = 0, i32 lastlinedefined = 0);

    void emitClosureUpvalues(const Vec<UpvalueCapture>& upvalues);
    void attachDebugMetadata();

private:
    CodeGenerator& owner_;
};

}  // namespace Lua
