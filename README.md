# Nightreign Enemy Status Overlay

Provides real-time monitoring of enemy health, poise (stagger), and status ailment buildups for **ELDEN RING: Nightreign**.

## Features
- **Stat Tracking**: Real-time display of current HP, Max HP, and Stagger bars.
- **Status Effects**: Support for Poison, Rot, Bleed, Blight, Frost, Sleep, and Madness buildup bars.
- **Adjustable UI**: Built-in settings panel for real-time opacity adjustment.

## Controls
- `~` : Toggle the settings panel. No `Shift` key required.

## Installation
### Mod Engine 3 (Recommended)
1. Download the latest `NightreignStatusMod.dll` from the Releases page.
2. Drag and drop the DLL into the loader, or use the DLL import feature.
3. Launch the game.

## Building from Source
### Prerequisites
- Windows 10/11
- Visual Studio 2022 (MSVC v143)
- CMake 3.15+
- Windows SDK 10.0.22621.0+

### Dependencies
Ensure the `third_party/` directory contains:
- `imgui/`
- `minhook/`

### Compilation
```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## Disclaimer
This project is an open-source learning project and has no connection with FromSoftware or Bandai Namco. Users must assume the risks associated with using the Mod by themselves.

---

# 《艾尔登法环：黑夜君临》敌人状态显示

本插件为 **《艾尔登法环：黑夜君临》** 提供实时的敌人血量、韧性（失衡）及异常状态累积监控。

## 功能
- **数值监控**：实时显示敌人当前的 HP、最大 HP 以及韧性条。
- **异常状态**：支持中毒、腐败、出血、咒死、冰冻、睡眠、发狂的累积槽显示。
- **UI 调节**：内置设置面板，支持在游戏内实时调节界面透明度。

## 按键控制
- `~` ：切换透明度设置面板。无需按住 `shift`

## 安装说明
### 使用 Mod Engine 3 (推荐)
1. 从 Releases 页面下载最新的 `NightreignStatusMod.dll`。
2. 将该 DLL 文件拖放进其中，或使用 DLL 文件导入功能。
3. 正常启动游戏。

## 源码编译
### 环境要求
- Windows 10/11
- Visual Studio 2022 (MSVC v143)
- CMake 3.15+
- Windows SDK 10.0.22621.0+

### 依赖项
确保 `third_party/` 目录下包含以下库：
- `imgui/`
- `minhook/`

### 编译指令
```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## 免责声明
本项目为开源学习项目，与 FromSoftware 或 Bandai Namco 无任何关联。用户需自行承担使用 Mod 带来的风险。