## Vision of Nightreign

**Vision of Nightreign** 是一款为《艾尔登法环：黑夜君临》设计的实时状态监控工具。在左上角添加了一个叠加层，显示当前目标敌人的额外信息，包括生命值、耐力、韧性、异常效果的积累条。

#### 目前的功能

- **属性条**：显示针对目标敌人的当前以及最大生命值，韧性。
- **异常积蓄**：显示当前效果的积累情况，如毒、猩红腐烂、出血、死亡枯萎、冻伤、睡眠和疯狂。仅在异常条有积累时显示，目标免疫或者未积累的异常条不显示。
- **自定义透明度**：支持调整透明度，按下 **~** 键会在屏幕左下角显示一个可拖动的滑块，以调整叠加层透明度。

#### 兼容性
Vision of Nightreign 基于《艾尔登法环：黑夜君临》**1.03.5** 版本开发，只要游戏内部结构保持不变，预计它能兼容之前和未来的版本。

- 由于非官方版本可能存在不可预测的一些问题，所以这些版本的工作稳定性可能无法保证。无论如何，如果出现问题请告诉我。
- 此外对于一些大型 mod，如果 mod 内部的结构与游戏结构保持相同，预计也能够兼容。如果在游玩大型 mod 的过程中遇见不兼容的 mod 也可以联系我。

#### 未来的计划
由于是我制作的第一个 mod，因此 mod 的各项功能都不是很完善。未来更新的点会很多：

- **自定义 UI**：当前只支持透明度的修改，预计在未来加入更多的功能，例如位置拖拽，自定义颜色，以及自定义显示的项目等。
- **界面美化**：目前只制作了一个简单的界面，未来预计对界面进行美化，例如异常图标的加入。
- **更多信息**：未来会加入更多的信息显示，例如目标对不同伤害类型的抗性（魔法，火焰，雷电等），对目标免疫的效果的额外显示。此外对褪色者类型的敌人也许存在 FP，耐力等数值，未来也会加入显示。
- 有更多的想法可以随时联系我。

#### 安装说明
推荐使用 [Mod Engine 3](https://github.com/soulsmods/ModEngine3/releases) 进行安装。

- 1. 下载模组压缩包并解压出 *VisionOfNightreign.dll* 文件。

- 2. 将 **dll 文件直接拖拽入 Mod Engine 3 界面中**，或者通过其 DLL 导入功能将文件导入。

- 3. 启动游戏。

  

**旧版的 Mod Engine 的安装方法理论上也是可行的，但我没有进行过测试。*

*注意：*
**Vision of Nightreign** 需要实时读取游戏内存。在连接官方服务器或启用反作弊（EAC）的情况下使用本模组有可能会导致**账号永久封禁**等一系列问题。本工具建议在离线模式下运行。

#### 鸣谢

- 感谢 **Souls Vision** 的作者。感谢其提供的启发，这个 Mod 在我游玩《艾尔登法环》时提供了巨大的帮助。

---

## Vision of Nightreign

**Vision of Nightreign** is a real-time status monitoring tool designed for *Elden Ring: Night's Embrace*. It adds an overlay in the upper left corner, displaying additional information about the current target enemy, including health, stamina, resilience, and accumulation bars for abnormal effects. 
Current Functions 

- **Attribute Bar**: Displays the current and maximum health, and resilience of the targeted enemy.
- **Abnormal Accumulation**: Shows the accumulation status of current effects, such as poison, crimson corruption, bleeding, withering, frostbite, sleep, and madness. It only appears when there is accumulation in the abnormal bar; immune or non-accumulated abnormal bars are not displayed.
- **Custom Transparency**: Allows for adjusting transparency. Pressing the **~** key will display a draggable slider in the lower left corner of the screen to adjust the transparency of the overlay. 
#### Compatibility
Vision of Nightreign is developed based on the **1.03.5** version of Elden Ring: Nightfall. As long as the internal structure of the game remains unchanged, it is expected to be compatible with previous and future versions. 
- Since unofficial versions may have some unpredictable issues, the stability of these versions cannot be guaranteed. Anyway, please let me know if any problems occur.
- Additionally, for some large mods, if the internal structure of the mod remains the same as the game structure, compatibility is expected. If you encounter incompatible mods while playing large mods, you can also contact me. 
#### Future Plans
Since this is my first mod, the various functions of the mod are not very complete. There will be many points for future updates: 
- **Custom UI**: Currently, only the modification of transparency is supported. It is expected that more features will be added in the future, such as position dragging, custom colors, and the ability to customize displayed items.
- **Interface Beautification**: At present, only a simple interface has been created. It is planned to beautify the interface in the future, for example, by adding abnormal icons.
- **More Information**: In the future, more information will be displayed, such as the target's resistance to different types of damage (magic, fire, lightning, etc.), and additional display of effects that the target is immune to. Additionally, for enemies of the Withered type, there may be values such as FP and endurance, which will also be displayed in the future.
- If you have any more ideas, feel free to contact me at any time. 
#### Installation Instructions
It is recommended to use [Mod Engine 3](https://github.com/soulsmods/ModEngine3/releases) for installation. 
1. Download the mod zip file and extract the *VisionOfNightreign.dll* file. 
2. Drag the **dll file directly into the Mod Engine 3 interface**, or import the file through its DLL import function. 
3. Start the game. 


The installation method of the old version of Mod Engine is theoretically feasible as well, but I haven't tested it. *

*Note:*
**Vision of Nightreign** requires real-time reading of game memory. Using this mod while connected to the official server or with anti-cheat (EAC) enabled may result in a series of issues, including permanent account bans. This tool is recommended to be run in offline mode. 

#### Acknowledgements
Thank you to the author of **Souls Vision**. I'm grateful for the inspiration it provided, as this mod was of great assistance to me during my playthrough of Elden Ring.

---

##  更新日志 Changelog

- **[v1.0.2] - 06-05-2026**
  - 修复了 steam 的叠加层冲突以及切屏后的任务栏遮挡问题
- **[v1.0.1] - 25-04-2026**
  - 新增了叠加层窗口缩放功能，允许拖拽窗口位置，并记忆上次窗口状态
    - 在按 `~` 键解锁设置界面后，现在可以自由拖拽血条 UI 的位置。
    - 新增了缩放滑动条，支持 0.5x 到 2.5x 的等比例缩放
    - UI 调整后的位置将自动保存，下次启动游戏后将沿用上一次结束游戏时的设置
