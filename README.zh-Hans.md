# TrickLogic2

《诡计×逻辑 第二季》（*Trick × Logic Season 2*，PSP，UCJS-10105）的原生移植，支持 macOS 和 Linux（包括 Steam Deck）。基于 [PortableKit](https://github.com/TeamGDB/PortableKit)：它把游戏的 PSP 代码预先编译成 C++，并提供游戏所需的 PSP 系统环境。

[English](README.md)

**本仓库不含任何游戏内容。** 你需要自己的光盘镜像。移植版会读取镜像、准备好游戏的可执行文件；玩家这边不需要编译任何东西。

## 支持的光盘

| 光盘 | 版本 | 用途 |
| --- | --- | --- |
| 《诡计×逻辑 第二季》（UCJS-10105） | 日文原版或其简体中文汉化版 | 游戏本体 |
| 《诡计×逻辑 第一季》（UCJS-10097） | 日文原版或其汉化版 | 附加内容：一代的四个案件加入二代的书架 |

光盘镜像可以是 `.iso`，也可以是用 `chdman createdvd` 转换的 CHD。CHD 直接读取，不会解压到磁盘。

两个二代版本共用同一套编译结果：汉化只改了文字和图片，没有改游戏代码。一代的案件在安装时从你自己的一代光盘生成，并使用一代自己的字体。

## 编译

```bash
git clone --recursive <本仓库地址>
cd TrickLogic2
cmake -S . -B out -G Ninja
cmake --build out -j2
```

第一次编译时还没有预编译的游戏代码，游戏会在 PortableKit 的解释器里运行，速度很慢。要编译游戏代码，先从光盘准备好可执行文件，再生成代码并重新编译：

```bash
out/bin/TrickLogic2Native --install /path/to/season2.iso
scripts/generate.sh
cmake --build out -j2
```

游戏代码大约 60 个编译单元，在 Apple Silicon Mac 上用 `-j2` 不到十分钟就能编完。请保持 `-j2`：单元很大，编译时很占内存。详细步骤（包括为 Steam Deck 交叉编译）见 [docs/BUILDING.md](docs/BUILDING.md)。

## 游玩

第一次启动时，初始设置会要求选择二代的光盘镜像，检查后准备好游戏，然后询问是否添加一代（可以跳过）。也可以在终端里操作：

```bash
TrickLogic2Native --install season2.chd           # 加 --in-place 则原地使用镜像，不复制
TrickLogic2Native --add-disc season1.chd          # 添加一代的案件，之后随时可以加
```

| 键盘 | 按键 |
| --- | --- |
| Z、空格 | ○（确定） |
| X、退格 | ×（取消） |
| C | △ |
| V | □ |
| Q、Page Up / E、Page Down | L / R |
| 回车 | START（跳过动画） |
| Tab | SELECT |
| 方向键、WASD | 十字键 |
| Esc | 移植版菜单 |

手柄按 PSP 的布局使用，L3+R3 打开菜单。

游戏数据、存档和设置在 macOS 上保存在 `~/Library/Application Support/TrickLogic2/UCJS10105`，在 Linux 上保存在 `~/.local/share/TrickLogic2/UCJS10105`。可以用 `TXL2_DATA_DIR` 指定其他目录。PortableKit 的各项环境变量都使用 `TXL2_` 前缀，见其 [README](https://github.com/TeamGDB/PortableKit)。

## 法律声明

本项目与索尼互动娱乐及游戏的汉化者均无关联。*Trick × Logic* 是其权利人的商标。本仓库不含游戏的任何代码或数据，请使用你自己拥有的光盘。

移植版以 [MIT 许可证](LICENSE) 发布。PortableKit 及其附带的组件保留各自的许可证。
