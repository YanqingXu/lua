# Modern C++ Lua 解释器 - 模块化测试构建脚本

# 设置基本变量
$ProjectRoot = "e:\Programming\Compiler\lua"
$SrcDir = "$ProjectRoot\src"
$TestsDir = "$SrcDir\tests"
$BuildDir = "$ProjectRoot\build_modular_tests"

Write-Host "=== Modern C++ Lua 模块化测试构建脚本 ===" -ForegroundColor Green
Write-Host "项目根目录: $ProjectRoot" -ForegroundColor Yellow
Write-Host "测试目录: $TestsDir" -ForegroundColor Yellow

# 创建构建目录
if (!(Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
    Write-Host "创建构建目录: $BuildDir" -ForegroundColor Cyan
}

# 定义编译器设置
$CppStandard = "c++17"
$IncludePaths = @("-I", "`"$SrcDir`"")
$CompilerFlags = @("-std=$CppStandard", "-Wall", "-Wextra", "-g")

# 定义测试源文件
$TestSources = @(
    "$TestsDir\test_main.cpp",
    "$TestsDir\lexer\lexer_test.cpp",
    "$TestsDir\vm\test_vm.cpp",
    "$TestsDir\vm\value_test.cpp", 
    "$TestsDir\vm\state_test.cpp",
    "$TestsDir\parser\test_parser.cpp",
    "$TestsDir\parser\parser_test.cpp",
    "$TestsDir\parser\function_test.cpp",
    "$TestsDir\parser\forin_test.cpp",
    "$TestsDir\parser\repeat_test.cpp", 
    "$TestsDir\parser\if_statement_test.cpp",
    "$TestsDir\compiler\test_compiler.cpp",
    "$TestsDir\compiler\symbol_table_test.cpp",
    "$TestsDir\compiler\literal_compiler_test.cpp",
    "$TestsDir\compiler\variable_compiler_test.cpp",
    "$TestsDir\compiler\binary_expression_test.cpp",
    "$TestsDir\compiler\expression_compiler_test.cpp",
    "$TestsDir\compiler\conditional_compilation_test.cpp",
    "$TestsDir\gc\test_gc.cpp",
    "$TestsDir\gc\gc_integration_test.cpp",
    "$TestsDir\gc\string_pool_demo_test.cpp"
)

# 定义主项目源文件（需要链接的）
$ProjectSources = @(
    "$SrcDir\lexer\lexer.cpp",
    "$SrcDir\parser\parser.cpp",
    "$SrcDir\compiler\compiler.cpp",
    "$SrcDir\vm\value.cpp",
    "$SrcDir\vm\table.cpp", 
    "$SrcDir\vm\state.cpp",
    "$SrcDir\vm\function.cpp",
    "$SrcDir\vm\vm.cpp",
    "$SrcDir\lib\base_lib.cpp"
)

function Build-ModularTests {
    Write-Host "`n=== 开始构建模块化测试 ===" -ForegroundColor Green
    
    # 检查源文件是否存在
    $MissingSources = @()
    foreach ($source in ($TestSources + $ProjectSources)) {
        if (!(Test-Path $source)) {
            $MissingSources += $source
        }
    }
    
    if ($MissingSources.Count -gt 0) {
        Write-Host "`n⚠️  警告: 以下源文件不存在:" -ForegroundColor Yellow
        foreach ($missing in $MissingSources) {
            Write-Host "  - $missing" -ForegroundColor Red
        }
        Write-Host "`n继续构建存在的文件..." -ForegroundColor Yellow
        
        # 过滤掉不存在的文件
        $TestSources = $TestSources | Where-Object { Test-Path $_ }
        $ProjectSources = $ProjectSources | Where-Object { Test-Path $_ }
    }
    
    # 构建命令
    $OutputFile = "$BuildDir\modular_tests.exe"
    $AllSources = ($TestSources + $ProjectSources) -join " "
    
    Write-Host "`n编译命令:" -ForegroundColor Cyan
    $CompileCmd = "g++ $($CompilerFlags -join ' ') $($IncludePaths -join ' ') $AllSources -o `"$OutputFile`""
    Write-Host $CompileCmd -ForegroundColor Gray
    
    # 执行编译
    Write-Host "`n正在编译..." -ForegroundColor Yellow
    try {
        Invoke-Expression $CompileCmd
        if ($LASTEXITCODE -eq 0) {
            Write-Host "`n✅ 编译成功!" -ForegroundColor Green
            Write-Host "可执行文件位置: $OutputFile" -ForegroundColor Cyan
            return $true
        } else {
            Write-Host "`n❌ 编译失败，退出代码: $LASTEXITCODE" -ForegroundColor Red
            return $false
        }
    }
    catch {
        Write-Host "`n❌ 编译过程中发生错误: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    }
}

function Run-ModularTests {
    $OutputFile = "$BuildDir\modular_tests.exe"
    
    if (Test-Path $OutputFile) {
        Write-Host "`n=== 运行模块化测试 ===" -ForegroundColor Green
        Write-Host "执行: $OutputFile" -ForegroundColor Cyan
        
        try {
            & $OutputFile
            if ($LASTEXITCODE -eq 0) {
                Write-Host "`n✅ 测试执行完成!" -ForegroundColor Green
            } else {
                Write-Host "`n⚠️  测试执行完成，但有部分测试可能失败" -ForegroundColor Yellow
            }
        }
        catch {
            Write-Host "`n❌ 测试执行失败: $($_.Exception.Message)" -ForegroundColor Red
        }
    } else {
        Write-Host "`n❌ 可执行文件不存在: $OutputFile" -ForegroundColor Red
        Write-Host "请先运行构建命令" -ForegroundColor Yellow
    }
}

function Show-Usage {
    Write-Host "`n=== 使用说明 ===" -ForegroundColor Green
    Write-Host "本脚本支持以下操作:" -ForegroundColor Yellow
    Write-Host "  build    - 构建模块化测试"
    Write-Host "  run      - 运行已构建的测试"
    Write-Host "  all      - 构建并运行测试"
    Write-Host "  clean    - 清理构建文件"
    Write-Host ""
    Write-Host "示例:"
    Write-Host "  .\build_modular_tests.ps1 build"
    Write-Host "  .\build_modular_tests.ps1 run"  
    Write-Host "  .\build_modular_tests.ps1 all"
}

# 主执行逻辑
switch ($args[0]) {
    "build" {
        if (Build-ModularTests) {
            Write-Host "`n构建完成! 使用 '.\build_modular_tests.ps1 run' 来运行测试" -ForegroundColor Green
        }
    }
    "run" {
        Run-ModularTests
    }
    "all" {
        if (Build-ModularTests) {
            Run-ModularTests
        }
    }
    "clean" {
        if (Test-Path $BuildDir) {
            Remove-Item -Recurse -Force $BuildDir
            Write-Host "✅ 清理完成: $BuildDir" -ForegroundColor Green
        } else {
            Write-Host "构建目录不存在，无需清理" -ForegroundColor Yellow
        }
    }
    default {
        Show-Usage
    }
}
