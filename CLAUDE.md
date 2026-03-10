## 项目结构

```
EnigmaEngine/
├── Engine/
│   ├── Source/
│   │   ├── Programs/
│   │   │   └── BuildTool/          # C# 构建工具 (.NET 9.0)
│   │   ├── Runtime/
│   │   │   ├── ApplicationCore/    # 窗口管理、消息泵、平台抽象
│   │   │   ├── AsciiRenderer/      # ASCII 字符渲染器 (Y-up 坐标约定)
│   │   │   ├── Core/               # 数学库、模块系统、日志、委托、配置、全局退出机制
│   │   │   ├── Engine/             # 引擎循环、GameEngine、GameInstance、SubsystemCollection
│   │   │   ├── Launch/             # 可执行入口点
│   │   │   └── RenderCore/         # 渲染接口抽象层
│   │   └── ThirdParty/
│   │       ├── googletest/         # git submodule (v1.15.2)
│   │       └── nlohmann_json/      # header-only JSON 库
│   ├── Config/                     # 引擎基础配置 (BaseEngine.ini, BaseGame.ini)
│   ├── Plugins/
│   │   └── EnhancedInput/          # 引擎插件: 增强输入系统
│   │       ├── EnhancedInput.eplugin
│   │       └── Source/EnhancedInput/  (Public/ + Private/)
│   └── Templates/                  # 脚手架模板 (Project, Module, Plugin)
├── EnigmaArcade/                   # 示例游戏项目
│   ├── EnigmaArcade.eproject       # 项目描述符
│   ├── Config/                     # 项目配置 (DefaultEngine.ini, DefaultGame.ini)
│   ├── Source/
│   │   ├── ArcadeGameplay/         # 游戏逻辑模块 (FAsciiGameObject)
│   │   └── EnigmaArcade/           # 主游戏模块
│   └── Plugins/
│       └── ArcadeFeature/          # 游戏插件
│           ├── ArcadeFeature.eplugin
│           ├── Config/             # 插件配置 (DefaultArcadeFeature.ini)
│           └── Source/ArcadeFeature/  (Public/ + Private/)
└── Tests/                           # C++ 单元测试 + 集成测试
```

## 代码风格
- C++26, Unreal 风格: F 前缀、CORE_API DLL 导出、Allman 大括号、`///` Doxygen、`Enigma` 命名空间
- 右手 Y-up 坐标系: Forward=(0,0,-1), Up=(0,1,0), Right=(1,0,0)
- 源文件仅使用 ASCII 字符（MSVC codepage 936 下非 ASCII 会触发 C4819 警告）
- 日志规范: 使用 `ENIGMA_LOG(Category, Verbosity, Fmt, ...)` 宏，不使用 `std::printf`/`std::fprintf`
  - 每个 .cpp 文件定义静态分类: `DEFINE_LOG_CATEGORY_STATIC(LogXxx, Info, All)`
  - 级别: Fatal / Error / Warning / Info / Verbose / Debug
  - Shipping 构建自动裁剪 Verbose/Debug（零开销）

## 构建与测试
- BuildTool: `dotnet build Engine/Source/Programs/BuildTool/BuildTool.sln -c Release`，20 个测试套件
- C++ 单元测试 (7 个测试项目):
  - `Tests/Core.Math.Tests/` — 284+ 个测试 / 11 套件 (FVector, FMatrix, FQuat, FRotator, FTransform, FRandomStream 等)
  - `Tests/Core.Delegates.Tests/` — 37 个测试 (TDelegate, TMulticastDelegate, FDelegateHandle)
  - `Tests/Core.Config.Tests/` — 配置系统测试 (FConfigCacheIni, ConfigDelegates)
  - `Tests/ApplicationCore.Tests/` — 20 个测试 (窗口创建、消息泵、消息处理器路由)
  - `Tests/RenderCore.Tests/` — RenderCore 模块测试
  - `Tests/Engine.Tests/` — Engine 模块测试 (SubsystemCollection 等)
  - `Tests/EnhancedInput.Tests/` — 增强输入系统测试 (动作、触发器、修饰器、映射上下文、管线)
  - `Tests/AsciiRenderer.Tests/` — AsciiRenderer 模块测试
- 引擎模块验证 (5 个集成测试):
  - `Tests/DllExportMacroTest/`, `Tests/ModuleInterfaceTest/`, `Tests/ModuleManagerTest/`
  - `Tests/LaunchModuleTest/`, `Tests/NlohmannJsonTest/`
- CI: `.github/workflows/build.yml`，4 个 job（buildtool / cpp-unit-tests / engine-module-validation / example-project-build）

## 第三方依赖
- googletest v1.15.2: git submodule (`Engine/Source/ThirdParty/googletest/source`)，wrapper CMakeLists.txt 用 `add_subdirectory(source)`
- nlohmann_json: 直接提交的 header-only 库 (`Engine/Source/ThirdParty/nlohmann_json/`)

## 参考项目和源代码
F:\\github\\EnigmaEngine\\Reference 包含了参考项目的源码和结构其中
- F:\\github\\EnigmaEngine\\Reference\\@example\_unreal\_project 是一个多模块多插件的Unreal游戏项目。
- F:\\github\\EnigmaEngine\\Reference\\@unreal\_engine\_source\_code 是Unreal Engine 5.6的源码。
- F:\github\EnigmaEngine\Reference\@example_unreal_shipped_project 是Unreal 的游戏项目的shipped打包后的文件结构。
在进行结构探索和源码查询时优先访问本机存在的内容其次进行互联网搜索。
