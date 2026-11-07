// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#include "Localization.h"

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <map>
#include <fstream>
#include "Log.h"
#include <vector>
#include <set>

namespace orbiter {

	// ========================================================================
	// Translation table
	// ========================================================================
	//
	// Each entry maps the English UI text (exactly as it appears in the dialog
	// resources or source code) to its Chinese translation.
	//
	// Notes:
	//  - Multi-line entries use "\n" for line breaks. At lookup time the current
	//    window text is normalized ("\r\n" -> "\n") before it is matched, so the
	//    table stays independent of the control's line-ending style.
	//  - The reverse mapping (Chinese -> English) is built from the same table,
	//    which allows switching back to English without restarting.
	//  - Strings are UTF-8; non-ASCII text is applied through the wide-character
	//    path in LocalizeWindowTree, so ANSI dialog resources are fine.
	//  - Accelerator keys are preserved: "&Launch Orbiter" -> "启动(&L)".
	//  - Non-translatable tokens (numbers, "4:3", "256 x 256", "v.123456", ...)
	//    are intentionally not listed.

	struct Translation {
		const char* en;
		const char* zh;
	};

	static const Translation g_translations[] = {
		// ---- Launchpad: main dialog -----------------------------------------
		{ "OpenOrbiter Launchpad", "OpenOrbiter 启动台" },
		{ "&Launch Orbiter", "启动 Orbiter(&L)" },
		{ "&Help", "帮助(&H)" },
		{ "E&xit", "退出(&X)" },
		{ "Scenarios ", "场景 " },
		{ "Options ", "选项 " },
		{ "Modules ", "模块 " },
		{ "Video ", "视频 " },
		{ "Extra ", "扩展 " },
		{ "About Orbiter ", "关于 Orbiter " },

		// ---- Launchpad: Scenario tab ----------------------------------------
		{ "Simulation scenarios", "模拟场景" },
		{ "Start paused", "启动时暂停" },
		{ "Save current ...", "保存当前..." },
		{ "Clear quicksaves", "清除快速存档" },

		// ---- Launchpad: Modules tab -----------------------------------------
		{ "Plugin modules", "插件模块" },
		{ "Expand all", "全部展开" },
		{ "Collapse all", "全部折叠" },
		{ "Deactivate all", "全部停用" },
		{ "Miscellaneous", "杂项" },
		{ "Tools and dialogs", "工具与对话框" },
		{ "Developer resources and samples", "开发者资源与示例" },
		{ "Input devices", "输入设备" },
		{ "MFD modes", "MFD 模式" },
		{ "Script tools and drivers", "脚本工具与驱动" },
		{ "SDK sample plugins", "SDK 示例插件" },
		{ "Sound module for Orbiter", "Orbiter 声音模块" },
		{ "Optional Orbiter plugin modules.\n\nDouble-click on a category to show or hide its entries.\n\nCheck or uncheck items to activate the corresponding modules.\n\nSelect an item to see a description of the module function.", "可选 Orbiter 插件模块。\n\n双击类别可显示或隐藏其条目。\n\n勾选或取消勾选可启用或停用对应模块。\n\n选择项目可查看模块功能说明。" },

		// ---- Launchpad: Video tab -------------------------------------------
		{ "Graphics engine:", "图形引擎：" },
		{ "3D device", "3D 设备" },
		{ "Always enumerate devices", "始终枚举设备" },
		{ "Try stencil buffer", "尝试模板缓冲" },
		{ "Full Screen", "全屏" },
		{ "Window", "窗口" },
		{ "Screen resolution", "屏幕分辨率" },
		{ "Colour depth [bpp]", "色深 [bpp]" },
		{ "Disable vertical sync.", "禁用垂直同步" },
		{ "Disable hardware pageflip", "禁用硬件页面翻转" },
		{ "Width", "宽度" },
		{ "Height", "高度" },
		{ "Fixed aspect ratio", "固定长宽比" },
		{ "Console mode (no engine loaded)", "控制台模式（未加载引擎）" },
		{ "No graphics engine has been selected. Orbiter will run in console mode.", "未选择图形引擎。Orbiter 将以控制台模式运行。" },

		// ---- Launchpad: Extra tab -------------------------------------------
		{ "Additional parameters", "附加参数" },
		{ "Edit", "编辑" },
		{ "Advanced and addon-specific configuration parameters.\n\nClick on an item to get a description.\n\nDouble-click to open or expand.", "高级及插件专用配置参数。\n\n单击项目查看说明。\n\n双击可打开或展开。" },

		// ---- Launchpad: About tab -------------------------------------------
		{ "Installed components", "已安装组件" },
		{ "On the web", "网络资源" },
		{ "Orbiter home:\nOrbiter web forum:\nYoutube channel:", "Orbiter 主页：\nOrbiter 官方论坛：\nYouTube 频道：" },
		{ "Orbiter is published under the MIT License.\nDetails can be found under 'License' below.", "Orbiter 以 MIT 许可证发布。\r\n详情请见下方“许可证”按钮。" },
		{ "License", "许可证" },
		{ "Credits and Contributions", "鸣谢与贡献者" },
		{ "Orbiter Homepage", "Orbiter 主页" },
		{ "D3D9Client module by Jarmo Nikkanen and Peter Schneider", "D3D9Client 模块：Jarmo Nikkanen 与 Peter Schneider" },
		{ "XRSound module Copyright (c) Doug Beachy", "XRSound 模块：版权所有 (c) Doug Beachy" },

		// ---- Launchpad: Video tab (controls added by graphics client) -------
		{ "Full Screen Mode", "全屏模式" },
		{ "Full Screen Window", "全屏窗口" },
		{ "Advanced", "高级" },
		{ "(unused)", "（未使用）" },
		{ "Force window size", "强制窗口尺寸" },
		{ "True Full Screen (no alt-tab)", "真全屏（不可 Alt-Tab）" },
		{ "Window with Taskbar", "带任务栏窗口" },
		{ "Multiple displays", "多显示器" },

		// ---- Extra page: startup module entries ------------------------------
		{ "Atlantis Configuration", "亚特兰蒂斯配置" },
		{ "Atmosphere Configuration", "大气配置" },
		{ "DG Configuration", "三角翼配置" },
		// ---- Common dialog buttons ------------------------------------------
		{ "OK", "确定" },
		{ "Cancel", "取消" },
		{ "Help", "帮助" },
		{ "Close", "关闭" },
		{ "Reset", "重置" },
		{ "Info", "信息" },
		{ "Apply", "应用" },

		// ---- Message / save scenario dialogs --------------------------------
		{ "Orbiter License", "Orbiter 许可证" },
		{ "Save current exit state", "保存当前退出状态" },
		{ "Scenario name", "场景名称" },
		{ "Description", "描述" },
		{ "Orbiter: Options", "Orbiter：选项" },
		{ "Please wait while Orbiter is closing down the simulation session . . .", "Orbiter 正在关闭模拟会话，请稍候 . . ." },

		// ---- Options page: Visual settings ----------------------------------
		{ "Visual settings", "视觉设置" },
		{ "Planetary effects", "行星效果" },
		{ "Cloud layers", "云层" },
		{ "Cloud shadows", "云影" },
		{ "Horizon haze", "地平线雾霭" },
		{ "Distance fog", "距离雾" },
		{ "Specular water reflections", "水面镜面反射" },
		{ "Specular ripples", "镜面波纹" },
		{ "Planet night lights, at level (0-1):", "行星夜间灯光，级别 (0-1)：" },
		{ "Surface elevation, using", "地表高度采样：" },
		{ "Max. resolution level (1-21)", "最高细节级别 (1-21)" },
		{ "General effects", "通用效果" },
		{ "Vessel shadows", "载具阴影" },
		{ "Reentry flames", "再入等离子火焰" },
		{ "Object shadows", "物体阴影" },
		{ "Particle streams", "粒子流" },
		{ "Specular object reflections", "物体镜面反射" },
		{ "Local light sources", "局部光源" },
		{ "Ambient light level (0-255)", "环境光级别 (0-255)" },
		{ "Some graphics clients may ignore, override or extend some of these settings.", "某些图形客户端可能忽略、覆盖或扩展其中部分设置。" },
		{ "linear interpolation", "线性插值" },
		{ "cubic interpolation", "三次插值" },

		// ---- Options page: Physics ------------------------------------------
		{ "Physics settings", "物理设置" },
		{ "Perturbations", "摄动" },
		{ "Nonspherical gravity sources", "非球形引力源" },
		{ "Radiation pressure", "辐射压" },
		{ "Gravity-gradient torque", "重力梯度力矩" },
		{ "Atmospheric wind effects", "大气风效应" },
		{ "Collisions", "碰撞" },
		{ "Vessel-to-Base", "载具与基地" },
		{ "Vessel-to-Vessel", "载具之间" },

		// ---- Options page: Instruments & panels -----------------------------
		{ "Instruments & panels", "仪表与面板" },
		{ "Multi-functional displays", "多功能显示器 (MFD)" },
		{ "MFD refresh interval [s]:", "MFD 刷新间隔 [秒]：" },
		{ "Glass cockpit MFD size:", "玻璃座舱 MFD 尺寸：" },
		{ "Transparent glass cockpit MFD", "玻璃座舱 MFD 透明" },
		{ "Virtual cockpit MFD texture size:", "虚拟座舱 MFD 纹理尺寸：" },
		{ "Instrument panels", "仪表面板" },
		{ "Panel scroll speed:", "面板滚动速度：" },
		{ "2D panel scale:", "2D 面板缩放：" },

		// ---- Options page: Vessel settings ----------------------------------
		{ "Vessel settings", "载具设置" },
		{ "Limited fuel", "有限燃料" },
		{ "Auto-refuel on pad", "发射台上自动加注" },
		{ "Complex flight model", "复杂飞行模型" },
		{ "Damage and failure simulation", "损伤与故障模拟" },

		// ---- Options page: User interface -----------------------------------
		{ "User interface", "用户界面" },
		{ "Mouse focus behaviour", "鼠标焦点行为" },
		{ "Language", "语言" },
		{ "Focus requires click", "需要点击才获得焦点" },
		{ "Hybrid: Click required only for child windows", "混合：仅子窗口需要点击" },
		{ "Focus follows mouse", "焦点跟随鼠标" },

		// ---- Options page: Joystick -----------------------------------------
		{ "Joystick", "摇杆" },
		{ "Joystick device", "摇杆设备" },
		{ "Main engine control", "主发动机控制" },
		{ "Ignore throttle setting on launch", "启动时忽略油门设置" },
		{ "Calibration", "校准" },
		{ "Throttle saturation", "油门饱和值" },
		{ "Deadzone", "死区" },
		{ "<Disabled>", "<已禁用>" },
		{ "<Keyboard only>", "<仅键盘>" },
		{ "Z-axis", "Z 轴" },
		{ "Slider 0", "滑块 0" },
		{ "Slider 1", "滑块 1" },

		// ---- Options page: Celestial sphere ---------------------------------
		{ "Celestial sphere", "天球" },
		{ "Background stars: show as pixels", "背景恒星：以像素显示" },
		{ "Map app. magnitude", "视星等映射" },
		{ "to max. brightness 1", "至最大亮度 1" },
		{ "to min. brightness", "至最小亮度" },
		{ "Magnitude-brightness mapping:", "星等-亮度映射：" },
		{ "linear", "线性" },
		{ "exponential", "指数" },
		{ "Background stars: show as image", "背景恒星：以图像显示" },
		{ "Background map", "背景星图" },
		{ "Brightness:", "亮度：" },

		// ---- Options page: Visual helpers -----------------------------------
		{ "Visual helpers", "视觉辅助" },
		{ "Options for displaying markers and gridlines on the celestial sphere, labels for planetary surface features, as well as coordinate axes and force vectors.", "用于显示天球标记与网格线、行星表面特征标签，以及坐标轴与力矢量的选项。" },
		{ "Planetarium mode (F9)", "天象仪模式 (F9)" },
		{ "Planetarium options", "天象仪选项" },
		{ "Surface/object labels (Alt-F9)", "表面/物体标签 (Alt-F9)" },
		{ "Label options", "标签选项" },
		{ "Body force vectors", "天体受力矢量" },
		{ "Body force options", "天体受力选项" },
		{ "Object frame axes", "物体坐标系" },
		{ "Frame axis options", "坐标系选项" },

		// ---- Options page: Planetarium --------------------------------------
		{ "Planetarium", "天象仪" },
		{ "Grids and circles", "网格与圆圈" },
		{ "Local horizon grid", "本地地平网格" },
		{ "Celestial grid", "天球网格" },
		{ "Ecliptic grid", "黄道网格" },
		{ "Galactic grid", "银河网格" },
		{ "Target Equator", "目标赤道" },
		{ "Constellations", "星座" },
		{ "Labels", "标签" },
		{ "Full", "完整" },
		{ "3-letter", "三字母" },
		{ "Boundaries", "边界" },
		{ "Patterns", "图案" },
		{ "Celestial markers", "天体标记" },
		{ "Show markers", "显示标记" },

		// ---- Options page: Labels -------------------------------------------
		{ "Surface and object labels", "表面与物体标签" },
		{ "Object markers", "物体标记" },
		{ "Vessels", "载具" },
		{ "Celestial bodies", "天体" },
		{ "Surface bases", "地面基地" },
		{ "VOR transmitters", "VOR 导航台" },
		{ "Surface features", "表面特征" },
		{ "Features", "特征" },

		// ---- Options page: Body forces --------------------------------------
		{ "Body forces", "天体受力" },
		{ "Linear forces", "线性力" },
		{ "Weight", "重力" },
		{ "Thrust", "推力" },
		{ "Lift", "升力" },
		{ "Drag", "阻力" },
		{ "Side", "侧向力" },
		{ "Total", "合力" },
		{ "Angular moments", "角动量" },
		{ "Torque", "力矩" },
		{ "Display", "显示" },
		{ "Scale:", "比例：" },
		{ "Log", "对数" },
		{ "Opacity:", "不透明度：" },

		// ---- Options page: Frame axes ---------------------------------------
		{ "Object axes", "物体坐标轴" },
		{ "Objects", "物体" },
		{ "Show negative axes", "显示负轴" },

		// ---- Extra page: time propagation -----------------------------------
		{ "Time propagation", "时间推进" },
		{ "Select and configure the time propagation methods Orbiter uses to update vessel positions and velocities from one time frame to the next.", "选择并配置 Orbiter 用于在各个时间帧之间更新飞船位置与速度的时间推进方法。" },
		{ "Dynamic state propagators", "动态状态推进器" },
		{ "Select the numerical integration methods used for dynamic state updates.\n\nState propagators affect the accuracy and stability of spacecraft orbits and trajectory calculations.", "选择用于动态状态更新的数值积分方法。\n\n状态推进器影响航天器轨道与轨迹计算的精度和稳定性。" },
		{ "Orbiter: Dynamic state propagators", "Orbiter：动态状态推进器" },
		{ "Integration stages", "积分阶段" },
		{ "Active:", "启用：" },
		{ "Propagator:", "推进器：" },
		{ "Step targets:", "步长目标：" },
		{ "time [s]", "时间 [秒]" },
		{ "rotation [°]", "旋转 [°]" },
		{ "Step limits:", "步长限制：" },
		{ "Parameters", "参数" },
		{ "Max. subsamples:", "最大子采样数：" },
		{ "Angular state propagators", "角状态推进器" },
		{ "Select the numerical integration method for dynamic angular state updates.\n\nAngular propagators affect the simulation accuracy and stability of rotating vessels.", "选择用于动态角状态更新的数值积分方法。\n\n角推进器影响旋转飞船的模拟精度与稳定性。" },
		{ "Orbit stabilisation", "轨道稳定化" },
		{ "Select the parameters that determine the conditions when Orbiter switches between dynamic and stabilised state updates.", "配置 Orbiter 在动态更新与稳定化更新之间切换的条件参数。" },
		{ "Orbiter: Orbit stabilisation parameters", "Orbiter：轨道稳定化参数" },
		{ "Enable orbit stabilisation", "启用轨道稳定化" },
		{ "Orbit step limit", "轨道步长限制" },
		{ "Enable stabilisation for orbit steps larger than", "当轨道步长大于以下值时启用稳定化：" },
		{ "% of full orbit", "整轨道的百分比" },
		{ "Gravitational field perturbation limit", "引力场摄动限制" },
		{ "Suppress stabilisation if gravity perturbations are greater than", "当引力摄动大于以下值时禁用稳定化：" },
		{ "% of total field", "总场强的百分比" },
		{ "Subsampling of perturbation integration", "摄动积分子采样" },
		{ "Target orbit step", "目标轨道步长" },
		{ "Max. steps", "最大步数" },
		{ "Perturbation model", "摄动模型" },
		{ "Suppress nonspherical gravity perturbations for orbit steps greater than", "当轨道步长大于以下值时抑制非球形引力摄动：" },
		{ "Orbiter: Angular dynamic state propagators", "Orbiter：角动态状态推进器" },
		{ "Active", "启用" },
		{ "Step limits", "步长限制" },
		{ "Time [s]", "时间 [秒]" },
		{ "Angle [deg]", "角度 [度]" },
		{ "Subsampling", "子采样" },
		{ "Target angle step [deg]:", "目标角度步长 [度]：" },
		{ "Rotation model", "旋转模型" },
		{ "Suppress cross-axis coupling for angle steps greater than [deg]:", "当角度步长大于以下值时抑制交叉轴耦合 [度]：" },
		{ "Suppress torque terms for angle steps greater than [deg]:", "当角度步长大于以下值时抑制力矩项 [度]：" },

		// ---- Extra page: other settings -------------------------------------
		{ "Instruments and panels", "仪表与面板" },
		{ "Select general configuration parameters for spacecraft instruments, MFD displays and instrument panels.", "配置航天器仪表、MFD 显示器与仪表面板的通用参数。" },
		{ "MFD parameter configuration", "MFD 参数配置" },
		{ "Select display parameters for multifunctional displays (MFD).", "设置多功能显示器 (MFD) 的显示参数。" },
		{ "Vessel configuration", "载具配置" },
		{ "Configure spacecraft parameters", "配置载具参数" },
		{ "Celestial body configuration", "天体配置" },
		{ "Configure options for celestial objects", "配置天体的相关选项" },
		{ "Debugging options", "调试选项" },
		{ "Various options that are useful for debugging and special tasks. Not generally used for standard simulation sessions.", "用于调试与特殊任务的各种选项，通常不用于常规模拟会话。" },
		{ "Orbiter shutdown options", "Orbiter 关闭选项" },
		{ "Set the behaviour of Orbiter after closing the simulation window: return to Launchpad, respawn or terminate.", "设置关闭模拟窗口后 Orbiter 的行为：返回启动台、重新启动进程或退出。" },
		{ "Orbiter: Shutdown options", "Orbiter：关闭选项" },
		{ "After closing simulation window:", "关闭模拟窗口后：" },
		{ "De-allocate memory and display launchpad dialog", "释放内存并显示启动台对话框" },
		{ "Respawn Orbiter process", "重新启动 Orbiter 进程" },
		{ "Terminate Orbiter process", "终止 Orbiter 进程" },
		{ "Fixed time steps", "固定时间步长" },
		{ "This option assigns a fixed simulation time interval to each frame. Useful for debugging, and when numerical accuracy and stability of the dynamic propagators are important (for example, to generate trajectory data or when recording high-fidelity playbacks).\n\nWarning: Selecting this option leads to nonlinear time flow and a simulation that is no longer real-time.", "此选项为每帧指定固定的模拟时间间隔。适用于调试，以及需要动态推进器数值精度与稳定性的场合（例如生成轨迹数据或录制高保真回放）。\n\n警告：启用此选项会导致时间非线性流动，模拟不再实时。" },
		{ "Orbiter: Fixed time steps", "Orbiter：固定时间步长" },
		{ "Enable fixed time step length", "启用固定时间步长" },
		{ "Fixed time step length per frame [s]:", "每帧固定时间步长 [秒]：" },
		{ "Rendering options", "渲染选项" },
		{ "Some rendering options that can be used for debugging problems.", "用于调试问题的渲染选项。" },
		{ "Performance options", "性能选项" },
		{ "This option can be used to modify Windows environment parameters that can improve the simulator performance.", "此选项可修改 Windows 环境参数以改善模拟器性能。" },
		{ "Orbiter: Performance settings", "Orbiter：性能设置" },
		{ "Font smoothing", "字体平滑" },
		{ "Disable font smoothing while Orbiter is running", "运行 Orbiter 时禁用字体平滑" },
		{ "Font smoothing provides better text display in MFD instruments, but can seriously degrade performance on some computers. Disable font smoothing if you are experiencing performance issues, such as periodic stutters.", "字体平滑可改善 MFD 仪表中的文字显示，但在某些计算机上会严重降低性能。若遇到周期性卡顿等性能问题，请禁用字体平滑。" },
		{ "Enforce font smoothing on exit", "退出时强制恢复字体平滑" },
		{ "Orbiter usually restores the original front smoothing setting on exit. Tick this option to enforce font smoothing on exiting Orbiter even if it was not active at the start.", "Orbiter 通常会在退出时恢复原有的字体平滑设置。勾选此项后，即使启动时未启用字体平滑，退出时也会强制启用。" },
		{ "Launchpad options", "启动台选项" },
		{ "Configure the behaviour of the Orbiter Launchpad dialog.", "配置 Orbiter 启动台对话框的行为。" },
		{ "Logfile options", "日志文件选项" },
		{ "Configure options for log file output.", "配置日志文件输出选项。" },

		// ---- Extra page: dialogues ------------------------------------------
		{ "Orbiter: Timer settings", "Orbiter：计时器设置" },
		{ "Timer preference:", "计时器首选：" },
		{ "Planetary body rendering options", "行星体渲染选项" },
		{ "Surface texture load options", "表面纹理加载选项" },
		{ "Load on demand (recommended)", "按需加载（推荐）" },
		{ "Pre-load at session start", "会话开始时预加载" },
		{ "Max. load frequency [Hz]:", "最大加载频率 [Hz]：" },
		{ "Mipmap interpolation", "Mipmap 插值" },
		{ "Do not load mipmaps (low quality)", "不加载 mipmap（低质量）" },
		{ "Point sampling (medium quality)", "点采样（中等质量）" },
		{ "Linear interpolation (high quality)", "线性插值（高质量）" },
		{ "Mipmap bias:", "Mipmap 偏移：" },
		{ "sharper", "更锐利" },
		{ "smoother", "更平滑" },
		{ "Anisotropic filtering", "各向异性过滤" },
		{ "Filter level:", "过滤级别：" },
		{ "Tile cache size:", "瓦片缓存大小：" },
		{ "Load tiles in separate thread", "在独立线程中加载瓦片" },
		{ "Tile resolution bias", "瓦片分辨率偏移" },
		{ "performance", "性能" },
		{ "quality", "质量" },
		{ "Tile patch mesh resolution", "瓦片网格分辨率" },
		{ "Tile sources", "瓦片来源" },
		{ "Orbiter: MFD parameters", "Orbiter：MFD 参数" },
		{ "MFD texture size: 2D panels", "MFD 纹理尺寸：2D 面板" },
		{ "Enforce pow2 MFD display texture sizes:", "强制 MFD 纹理尺寸为 2 的幂：" },
		{ "No", "否" },
		{ "Yes", "是" },
		{ "Auto", "自动" },
		{ "Switch to hires texture at MFD size:", "MFD 尺寸达到以下值时切换高分辨率纹理：" },
		{ "MFD texture size: virtual cockpits", "MFD 纹理尺寸：虚拟座舱" },
		{ "Orbiter: Launchpad options", "Orbiter：启动台选项" },
		{ "Scenario description format", "场景说明格式" },
		{ "Inline HTML", "内嵌 HTML" },
		{ "Simple text display with external HTML viewer", "简易文本显示（外部 HTML 查看器）" },
		{ "Auto detect", "自动检测" },
		{ "Scenario screenshot", "场景截图" },
		{ "Save a screenshot on closing a simulation session", "关闭模拟会话时保存截图" },
		{ "If activated, the screenshot is displayed in the description of the ", "启用后，截图将显示在以下场景的说明中：" },
		{ "Orbiter: Debug Rendering Options", "Orbiter：调试渲染选项" },
		{ "Render objects as wire frames", "以线框渲染物体" },
		{ "Force normalisation of normals", "强制法线归一化" },
		{ "Orbiter: Logfile options", "Orbiter：日志文件选项" },
		{ "Output format", "输出格式" },
		{ "Verbose output", "详细输出" },
		{ "Verbose output logs many events during the simulation session, such as texture loads. This can help identifying problems, but it may also slightly affect performance, and will create very large log files.", "详细输出会记录模拟会话中的大量事件（如纹理加载），有助于定位问题，但可能轻微影响性能，并产生非常大的日志文件。" },

		// ---- Loading screen (splash) ----------------------------------------
		{ "ORBITER Space Flight Simulator", "ORBITER 太空飞行模拟器" },
		{ "Build", "构建" },
		{ " (Debug Build)", "（调试版）" },
		{ " (Release Build)", "（正式版）" },
		{ " via D3D9on12 emulator", "（通过 D3D9on12 兼容层）" },
		{ "Building Shader Programs...", "正在构建着色器程序…" },
		{ "Loading Celestial Sphere...", "正在加载天球…" },

		// ---- In-simulation dialogs: Flight recorder / player -----------------
		{ "A flight record under this name already exists!", "该名称的飞行记录已存在！" },
		{ "Attitude Data", "姿态数据" },
		{ "Editor", "编辑器" },
		{ "Equatorial Frame", "赤道坐标系" },
		{ "Local Horizon Frame", "本地地平坐标系" },
		{ "Overwrite", "覆盖" },
		{ "Play at Recording Speed", "按录制速度播放" },
		{ "Position / Velocity Data", "位置/速度数据" },
		{ "REC", "录制" },
		{ "Record Focus Events", "记录焦点事件" },
		{ "Record Time Acceleration", "记录时间加速" },
		{ "Replay Options", "回放选项" },
		{ "Sampling in System Time Steps", "按系统时间步长采样" },
		{ "Show Inflight Notes", "显示飞行中注释" },
		{ "STOP", "停止" },
		{ "Use Recorded Camera Settings", "使用录制的相机设置" },
		{ "Use Recorded Focus Events", "使用录制的焦点事件" },
		{ "Resume", "继续" },

		// ---- In-simulation dialogs: Map --------------------------------------
		{ "Reset map", "重置地图" },
		{ "All", "全部" },
		{ "Focus Only", "仅焦点" },
		{ "None", "无" },
		{ "Orbit Display", "轨道显示" },
		{ "Focus Vessel", "焦点载具" },
		{ "Target", "目标" },
		{ "Orbit Plane", "轨道面" },
		{ "Ground Track", "地面轨迹" },
		{ "Horizon Line", "地平线" },
		{ "Terminator", "晨昏线" },
		{ "Line", "线" },
		{ "Shaded", "阴影" },
		{ "Surface Markers", "地表标记" },
		{ "Grid Line", "网格线" },
		{ "Surface Bases", "地表基地" },
		{ "Coastlines", "海岸线" },
		{ "VOR Transmitters", "VOR 发射台" },
		{ "Contour Lines", "等高线" },
		{ "Landmarks", "地标" },
		{ "Other", "其他" },
		{ "Natural Satellites", "天然卫星" },
		{ "Moons", "卫星" },
		{ "Search filter", "搜索过滤" },
		{ "Search object", "搜索物体" },
		{ "Open info dialog for object", "打开物体信息对话框" },
		{ "Bases", "基地" },

		// ---- Surface MFD: speed mode labels --------------------------------

		// ---- Surface MFD: tape labels --------------------------------------

		{ "(step 0.05)", "0.05 步进" },
		{ "(step 1.0)", "1.0 步进" },
		{ "IDS", "信标" },
		// ---- Comms MFD -------------------------------------------------------
		{ "XPDR", "应答机" },
		{ "XPDR Transmitter", "应答机发射机" },
		{ "NAV Receiver Stack", "接收机列表" },
		{ "Select receiver", "选择接收机" },
		{ "Dec frequency", "减小频率" },
		{ "Inc frequency", "增大频率" },
		{ "Scan down", "向下扫描" },
		{ "Scan up", "向上扫描" },

		// ---- Transfer MFD ----------------------------------------------------
		{ "Select reference (Shift-R)", "选择参考（Shift-R）" },
		{ "Src", "源" },
		{ "[self]", "[自身]" },
		{ "TLi", "交点经度" },
		{ "DTi", "交会时间" },
		{ "HTO", "假设轨道" },
		{ "TLe", "切出经度" },
		{ "DTe", "切出时间" },
		{ "Dv", "Δv" },
		{ "Tgt ", "目标 " },
		{ "Num orbit", "数值轨迹" },
		{ "Stp", "时步数" },
		{ "Enter orbit", "输入轨道参考" },
		{ "Enter source", "输入源" },
		{ "Enter target", "输入目标" },
		{ "Toggle transfer", "切换转移轨道" },
		{ "Toggle numerical", "切换数值轨迹" },
		{ "Update numerical", "更新数值轨迹" },
		{ "Adjust time steps", "调整时间步长" },
		{ "Decrease dV", "减小 Δv" },
		{ "Increase dV", "增大 Δv" },
		{ "Decrease step", "减小步长比例" },
		{ "Increase step", "增大步长比例" },

		// ---- Sync orbit MFD -------------------------------------------------
		{ "Sync Orbit: ", "轨道同步：" },
		{ "no target", "无目标" },
		{ "Cannot determine orbit", "无法确定轨道" },
		{ "Select target (Shift-T)", "选择目标（Shift-T）" },
		{ "Invalid target", "目标无效" },
		{ "Cannot determine target orbit", "无法确定目标轨道" },
		{ "Ref: ", "参考：" },
		{ "Intersect 1", "交点1" },
		{ "Intersect 2", "交点2" },
		{ "Sh periapsis", "飞船近点" },
		{ "Sh apoapsis", "船远点" },
		{ "Tg periapsis", "目标近点" },
		{ "Tg apoapsis", "目标远点" },
		{ "Manual axis", "手动轴" },
		{ "No intersection", "无交点" },
		{ "RAnm", "轴近角" },
		{ "DLng", "经度差" },
		{ "RVel", "相对速度" },
		{ "DTmin", "时间差" },
		{ "Ob", "序" },
		{ "Sh-ToR", "船-参考" },
		{ "Tg-ToR", "目标-参考" },
		{ "Toggle intersect", "切换相交模式" },
		{ "Select # transit", "设置绕行圈数" },
		{ "Rot. manual axis", "旋转手动轴" },

		// ---- Align planes MFD -----------------------------------------------
		{ "Current", "当前" },
		{ "Relative", "相对" },
		{ "dA[°]", "交点角[°]" },
		{ "TtN[s]", "到交点[s]" },
		{ "dV[m/s]", "ΔV[m/s]" },
		{ "BT[s]", "燃烧[s]" },
		{ "TtB[s]", "点火倒计时[s]" },
		{ "TtL[s]", "发射倒计时[s]" },
		{ "NML-", "法向-" },
		{ "NML+", "法向+" },

		{ "Align plane", "轨道面对齐" },
		{ "Orbit  ", "轨道" },
		{ "Ballist", "弹道" },
		{ "[Custom]", "[自定义]" },
		{ "No reference body selected.", "未选择参考天体。" },
		{ "No target orbit selected.", "未选择目标轨道。" },
		{ "Use Shift-T to select a target", "按 Shift-T 选择目标" },
		{ "or Shift-E to define a custom", "或按 Shift-E 定义自定义" },
		{ "orbit.", "轨道。" },
		{ "Current     Target      Relative", "当前          目标          相对" },
		{ "RInc", "相对倾角" },
		{ "R", "变化率" },
		{ "Node encounter", "交点相遇" },
		{ "dA[°] TtN[s]", "交点角[°] 到交点[s]" },
		{ "Dir   dV[m/s]  BT[s]   TtB[s]", "方向   ΔV[m/s]  燃烧[s]  点火倒计时[s]" },
		{ "[LAUNCH]", "[发射]" },
		{ " [BURN] ", " [点火] " },
		{ "No intersection with target", "与目标轨道面无交点：" },
		{ "plane at launch latitude.", "发射纬度处。" },
		{ "Trajectory intersects surface", "轨迹与地面相交" },
		{ "Data may be inaccurate.", "数据可能不准确。" },
		{ "AN", "升" },
		{ "DN", "降" },
		{ "Select elements", "输入参数" },
		{ "Cycle modes", "循环模式" },

		// ---- Docking MFD ----------------------------------------------------
		{ "Docking: ", "对接：" },
		{ "APPR/DOCK", "交会/对接" },
		{ "FINE", "精细" },
		{ "NO DATA", "无数据" },
		{ "Device not available", "设备不可用" },
		{ "NO SIGNAL", "无信号" },
		{ " DOCKED ", " 已对接 " },
		{ " IDS ", " 信标 " },
		{ " XPDR ", " 应答机 " },
		{ " VIS ", " 目视 " },
		{ "IDS ", "信标 " },
		{ "XPDR ", "应答机 " },
		{ "hD", "横距" },
		{ "vD", "纵距" },
		{ "hV", "横移率" },
		{ "vV", "纵移率" },
		{ "CVEL", "接近率" },
		{ "Y", "偏航" },
		{ "P", "俯仰" },
		{ "B", "滚转" },
		{ "Select docking port", "选择对接端口" },
		{ "Visual acquisition", "目视捕获" },
		{ "Toggle coarse/fine", "切换粗/精刻度" },
		{ "dock", "接口" },

		// ---- VOR/VTOL MFD --------------------------------------------------
		{ "DIR", "方向" },
		{ "HSPD", "平移率" },
		{ "VSPD", "升降率" },
		{ "DIST", "距离" },

		// ---- HSI MFD --------------------------------------------------------
		{ "ILS Rwy", "ILS 跑道" },
		{ "[no signal]", "[无信号]" },
		{ "TO", "向台" },
		{ "FROM", "背台" },
		{ "NAV", "接收机" },
		{ "VOR", "信标" },
		{ "DEV", "偏差" },
		{ "Switch HSI focus", "切换 HSI 焦点" },
		{ "NAV receiver", "选择 NAV 接收机" },
		{ "OBS indicator left", "OBS 指示器 左" },
		{ "OBS indicator right", "OBS 指示器 右" },

		// ---- Map marker list names ------------------------------------------
		{ "Airport", "机场" },
		{ "City", "城市" },
		{ "Mountain", "山" },
		{ "Forest", "森林" },
		{ "Water body", "水体" },
		{ "City.mkr", "城市" },
		{ "Airport.mkr", "机场" },
		{ "Mountain.mkr", "山" },
		{ "Forest.mkr", "森林" },
		{ "Water body.mkr", "水体" },

		// ---- Selection menus ------------------------------------------------
		{ "By name ...", "按名称…" },
		{ "Spacecraft", "航天器" },
		{ "[none]", "[无]" },

		// ---- Map MFD --------------------------------------------------------
		{ "Map: ", "地图：" },
		{ "Map: Display parameters", "地图：显示参数" },
		{ "TRK", "轨迹" },
		{ "ZM %d", "缩放 %d" },
		{ "TGT:", "目标：" },
		{ "BSE:", "基地：" },
		{ "SHP:", "飞船：" },
		{ "Brg", "方位" },
		{ "Crs", "航向" },
		{ "Orbit lines", "轨道线" },
		{ "Groundtrack", "地面轨迹" },
		{ "Horizon lines", "地平线" },
		{ "Shading", "阴影" },
		{ "Line+shading", "线+阴影" },
		{ "Grid lines", "网格线" },
		{ "Base markers", "基地标记" },
		{ "Navaid markers", "导航台标记" },
		{ "ON ", "开" },
		{ "OFF", "关" },
		{ "Map reference", "地图参考天体" },
		{ "Zoom out", "缩小" },
		{ "Zoom in", "放大" },
		{ "Track mode on/off", "跟踪模式开关" },
		{ "Display parameters", "显示参数" },
		{ "Scroll up", "向上滚动" },
		{ "Scroll down", "向下滚动" },
		{ "Scroll left", "向左滚动" },
		{ "Scroll right", "向右滚动" },
		{ "Move selection up", "选择上移" },
		{ "Move selection down", "选择下移" },
		{ "Modify selected", "修改选中" },
		{ "Return to map", "返回地图" },

		// ---- MFD menu value hints --------------------------------------------
		{ "ECL/EQU", "黄道/赤道" },
		{ "Radius/Altitude", "半径/高度" },

		// ---- MFD mode selection page ---------------------------------------
		{ "MODE SELECT", "模式选择" },
		{ "MODE SELECT [Page %d]", "模式选择[第 %d 页]" },
		{ "MFD MENU", "菜单" },
		{ "Orbit", "轨道" },
		{ "Surface", "地表飞行" },
		{ "HSI", "导航方位仪" },
		{ "VOR/VTOL", "垂直起降" },
		{ "Docking", "对接" },
		{ "Align Planes", "轨道面对齐" },
		{ "Sync Orbit", "轨道同步" },
		{ "Transfer", "轨道转移" },
		{ "COM/NAV", "通讯/导航" },

		// ---- Surface MFD (dedicated keys) ----------------------------------
		{ "SURF_Title", "地表：" },
		{ "SURF_ATM", "大气数据" },
		{ "SURF_OAT", "外温" },
		{ "SURF_M", "马赫" },
		{ "SURF_DNS", "密度" },
		{ "SURF_STP", "静压" },
		{ "SURF_DNP", "动压" },
		{ "SURF_EQP", "位置" },
		{ "SURF_RATE", "变化率" },
		{ "SURF_BNK", "滚转" },
		{ "SURF_PTCH", "俯仰" },
		{ "SURF_L", "左" },
		{ "SURF_R", "右" },
		{ "SURF_ALTR", "对地高" },
		{ "SURF_ALT", "海拔高" },
		{ "SURF_VS", "垂速" },
		{ "SURF_VACC", "垂直加速" },
		{ "SURF_AOA", "攻角" },
		{ "SURF_ACC", "加速度" },
		{ "SURF_GS", "地速" },
		{ "SURF_TAS", "真速" },
		{ "SURF_IAS", "表速" },
		{ "SURF_OS", "轨速" },

		// ---- Surface MFD ---------------------------------------------------
		{ "Surface: ", "地表：" },
		{ "ATM DATA", "大气数据" },
		{ "Indicated airspeed", "表显空速" },
		{ "Ground-relative", "地面速度" },
		{ "Orbital speed", "轨道速度" },

		// ---- Orbit MFD (final scheme) -------------------------------------
		{ "Ship plane", "自身" },
		{ "Target plane", "目标" },

		// ---- Orbit MFD -----------------------------------------------------
		{ "Orbit: ", "轨道：" },
		{ "Proj: ", "投影：" },
		{ "Frm     Prj", "参考  投影" },
		{ "Frm", "参考" },
		{ "Prj", "投影" },
		{ "ECL", "黄道" },
		{ "TGT", "目标" },
		{ "EQU", "赤道" },
		{ "SHP", "自身" },
		{ "SMa", "半长轴" },
		{ "SMi", "半短轴" },
		{ "PeR", "近点径" },
		{ "ApR", "远点径" },
		{ "Rad", "半径　" },
		{ "PeA", "近点高" },
		{ "ApA", "远点高" },
		{ "ALT", "高度" },
		{ "Alt", "高度　" },
		{ "Ecc", "偏心率" },
		{ "T", "周期　" },
		{ "PeT", "近点时" },
		{ "ApT", "远点时" },
		{ "Vel", "速度　" },
		{ "Inc", "倾角　" },
		{ "LAN", "升交经" },
		{ "LPe", "近点经" },
		{ "AgP", "近点幅" },
		{ "TrA", "真近角" },
		{ "TrL", "真经度" },
		{ "MnA", "平近角" },
		{ "MnL", "平经度" },
		{ "--OSC.EL.--", "--密切根数--" },
		{ "Orbital elements", "轨道根数" },
		{ "not available", "不可用" },
		{ "not", "不" },
		{ "available", "可用" },
		{ "Orbit reference", "参考天体" },
		{ "Auto reference", "自动参考" },
		{ "Select target", "选择目标" },
		{ "Unselect target", "取消目标" },
		{ "Display mode", "显示模式" },
		{ "Reference frame", "选择坐标系" },
		{ "Projection plane", "投影面" },
		{ "Distance display", "半径/高度切换" },
		{ "Copy data to HUD", "复制数据到 HUD" },
		{ "REF", "参考" },
		{ "AR", "自动" },
		{ "NT", "删目标" },
		{ "MOD", "模式" },
		{ "PRJ", "投影面" },
		{ "Ecliptic", "黄道面" },

		// ---- State propagators / pad status / directions ---------------------
		{ "free", "空闲" },
		{ "Docked to", "已对接至" },
		{ "Main", "主发动机" },
		{ "Retro", "制动发动机" },
		{ "Hover", "悬停发动机" },
		{ "Aux", "辅助发动机" },
		{ "reserved", "已预留" },
		{ "stabilised", "稳定" },
		{ "Runge-Kutta, 2nd order (RK2)", "龙格-库塔，二阶（RK2）" },
		{ "Runge-Kutta, 4th order (RK4)", "龙格-库塔，四阶（RK4）" },
		{ "Runge-Kutta, 1st order (RK1)", "龙格-库塔，一阶（RK1）" },
		{ "Euler", "欧拉" },
		{ "Symplectic", "辛积分" },

		// ---- Case-difference / misc entries ---------------------------------
		{ "Select...", "请选择…" },
		{ "Scenario", "场景" },
		{ "X", "X" },

		// ---- Vessel class names -------------------------------------------
		{ "DeltaGlider", "三角翼" },
		{ "DG-S", "三角翼-S" },
		{ "ShuttleA", "航天飞机 A" },
		{ "ShuttlePB", "航天飞机 PB" },
		{ "ProjectAlpha_ISS", "阿尔法计划空间站" },
		{ "MaterialTestSphere", "材质测试球" },
		{ "Module1", "模块 1" },
		{ "Module2", "模块 2" },
		{ "mplm", "多用途后勤舱" },
		{ "Wheel", "轮" },
		{ "Atlantis", "亚特兰蒂斯" },
		{ "ScriptPB", "脚本航天飞机" },

		// ---- Options: force vectors / labels / axes ---------------------------
		{ "2D panel scale", "2D 面板比例" },
		{ "X-axis", "X 轴" },
		{ "Y-axis", "Y 轴" },

		// ---- Options: planetarium / sliders ------------------------------------
		{ "Apollo AGC navigation stars", "阿波罗 AGC 导航星" },
		{ "Common star names", "常用星名" },
		{ "Galactic", "银河系" },
		{ "Messier objects", "梅西耶天体" },
		{ "Bright Stars (ESA Hipparcos + Tycho)", "亮星（ESA 依巴谷 + 第谷）" },
		{ "Starmap 2020 (ESA Gaia)", "星图 2020（ESA 盖亚）" },
		{ "Saturation", "饱和度" },
		{ "Max magnitude threshold", "最大星等阈值" },
		{ "Min magnitude threshold", "最小星等阈值" },
		{ "Min brightness", "最小亮度" },
		{ "Brightness", "亮度" },
		{ "Scale", "缩放" },
		{ "Rotate", "旋转" },

		// ---- Misc UI strings ------------------------------------------------
		{ "Simple", "简单" },
		{ "Realistic", "真实" },
		{ "No damage", "无损伤" },
		{ "Allow damage", "允许损伤" },
		{ "Field of view [%s]", "视场角 [%s]" },
		{ "External", "外部" },
		{ "Tilt", "倾斜" },
		{ "External view", "外部视角" },

		// ---- Celestial bodies and surface bases ------------------------------
		{ "Vesta", "灶神星" },
		{ "Ceres", "谷神星" },
		{ "Hyperion", "土卫七（海伯利安）" },
		{ "Phoebe", "土卫九（福柏）" },
		{ "Miranda", "天卫五（米兰达）" },
		{ "Ariel", "天卫一（艾瑞尔）" },
		{ "Umbriel", "天卫二（乌姆柏里厄尔）" },
		{ "Titania", "天卫三（泰坦妮亚）" },
		{ "Oberon", "天卫四（奥伯龙）" },
		{ "Triton", "海卫一（特里同）" },
		{ "Proteus", "海卫八（普罗透斯）" },
		{ "Nereid", "海卫二（涅瑞伊得）" },
		{ "Nix", "冥卫二（尼克斯）" },
		{ "Hydra", "冥卫三（许德拉）" },
		{ "Luna-OB1", "玉盘1" },
		{ "Al Anbar", "安巴尔" },
		{ "Alcantara", "阿尔坎塔拉" },
		{ "Baikonur", "拜科努尔" },
		{ "Barent Sea", "巴伦支海" },
		{ "China Lake", "中国湖" },
		{ "Doberai", "多贝拉伊" },
		{ "Edwards", "爱德华兹" },
		{ "Gran Canaria", "大加那利" },
		{ "Habana", "哈瓦那" },
		{ "Hammaguira", "哈马基拉" },
		{ "Inyokern", "因约肯" },
		{ "Jingyu", "靖宇" },
		{ "Jiuquan", "酒泉" },
		{ "Kagoshima", "鹿儿岛" },
		{ "Kapustin Yar", "卡普斯京亚尔" },
		{ "Kourou", "库鲁" },
		{ "Matagorda", "马塔戈达" },
		{ "Mojave Spaceport", "莫哈韦航天港" },
		{ "Musudan", "舞水端" },
		{ "Olympus", "奥林帕斯" },
		{ "Overberg", "奥弗贝格" },
		{ "Palmachim", "帕尔马希姆" },
		{ "Palmdale AF Plant 42", "帕姆代尔空军42厂" },
		{ "Peenemünde", "佩内明德" },
		{ "Plesetsk", "普列谢茨克" },
		{ "Salto di Quirra", "萨尔托迪基拉" },
		{ "San Marco", "圣马可" },
		{ "Sriharikota", "斯里哈里科塔" },
		{ "Svobodniy", "斯沃博德内" },
		{ "Taiyuan", "太原" },
		{ "Tanegashima", "种子岛" },
		{ "Vandenberg", "范登堡" },
		{ "Wallops Island", "沃洛普斯岛" },
		{ "White Sands", "白沙" },
		{ "Woomera", "伍默拉" },
		{ "Wuzhai", "五寨" },
		{ "Xichang", "西昌" },

		// ---- In-simulation dialogs / plugin UI / names ----------------------
		{ "Control", "控制" },
		{ "Preset", "预设" },
		{ "Glass cockpit", "玻璃座舱" },
		{ "Panel view", "面板视图" },
		{ "Virtual cockpit", "虚拟座舱" },
		{ "Nearby", "附近" },
		{ "Available targets", "可用目标" },
		{ "Selected target", "已选目标" },
		{ "Select", "选择" },
		{ "Previous", "上一个" },
		{ "Save snapshots", "保存快照" },
		{ "To the clipboard", "到剪贴板" },
		{ "To file", "到文件" },
		{ "Take snapshot", "拍摄快照" },
		{ "Save frame sequence", "保存帧序列" },
		{ "To directory", "到目录" },
		{ "Counter start", "起始计数器" },
		{ "Skip frames", "跳过帧" },
		{ "Stop recording", "停止录制" },
		{ "Start recording", "开始录制" },
		{ "File settings", "文件设置" },
		{ "File format", "文件格式" },
		{ "Image quality", "图像质量" },
		{ "Appearance", "外观" },
		{ "Menu bar", "菜单栏" },
		{ "Show", "显示" },
		{ "Hide", "隐藏" },
		{ "Auto-hide", "自动隐藏" },
		{ "Always show labels", "始终显示标签" },
		{ "Opacity", "不透明度" },
		{ "Animation speed", "动画速度" },
		{ "Button size", "按钮尺寸" },
		{ "Hovered size", "悬停尺寸" },
		{ "Spacing", "间距" },
		{ "Info bars", "信息栏" },
		{ "Always show warp factor", "始终显示加速倍率" },
		{ "FPS info bar", "FPS 信息栏" },
		{ "Menu items", "菜单项" },
		{ "from module", "来自模块" },
		{ "2-body", "二体" },
		{ "Spaceports", "航天港" },
		{ "D3D9 Debug Controls", "D3D9 调试控制" },
		{ "This dialog allows to control various debug and development features", "此对话框用于控制各种调试与开发功能" },
		{ "D3D9 Graphics Controls", "D3D9 图形控制" },
		{ "This dialog allows to control various graphics options", "此对话框用于控制各种图形选项" },
		{ "D3D9 Collider Visuals", "D3D9 碰撞体可视化" },
		{ "This dialog allows toggling physics collider visualizations", "此对话框用于切换物理碰撞体的可视化" },
		{ "D3D9 Atmospheric Controls", "D3D9 大气控制" },
		{ "This dialog allows to control various atmospheric parameters and effects", "此对话框用于控制各种大气参数与效果" },
		{ "Mercury", "水星" },
		{ "Venus", "金星" },
		{ "Earth", "地球" },
		{ "Moon", "月球" },
		{ "Mars", "火星" },
		{ "Jupiter", "木星" },
		{ "Saturn", "土星" },
		{ "Uranus", "天王星" },
		{ "Neptune", "海王星" },
		{ "Pluto", "冥王星" },
		{ "Sol", "太阳系" },
		{ "Io", "木卫一（伊奥）" },
		{ "Europa", "木卫二（欧罗巴）" },
		{ "Ganymede", "木卫三（盖尼米德）" },
		{ "Callisto", "木卫四（卡利斯托）" },
		{ "Amalthea", "木卫五（阿马尔忒亚）" },
		{ "Himalia", "木卫六（希马利亚）" },
		{ "Mimas", "土卫一（弥玛斯）" },
		{ "Enceladus", "土卫二（恩克拉多斯）" },
		{ "Tethys", "土卫三（忒堤斯）" },
		{ "Dione", "土卫四（狄俄涅）" },
		{ "Rhea", "土卫五（瑞亚）" },
		{ "Titan", "土卫六（泰坦）" },
		{ "Iapetus", "土卫八（伊阿珀托斯）" },
		{ "Phobos", "火卫一（福波斯）" },
		{ "Deimos", "火卫二（得摩斯）" },
		{ "Charon", "冥卫一（卡戎）" },
		{ "ISS", "国际空间站" },
		{ "Mir", "和平号空间站" },
		{ "LUNA-OB", "玉盘" },
		{ "Cape Canaveral", "卡纳维拉尔角" },
		{ "Brighton Beach", "布莱顿海滩" },

		// ---- In-simulation dialogs: content --------------------------------
		{ "Add", "添加" },
		{ "ACTIVE (dynamic)", "活动（动态）" },
		{ "Aerodynamic Parameters", "气动参数" },
		{ "Altitude", "高度" },
		{ "Altitude [m]", "高度 [m]" },
		{ "Analytic (%s)", "解析（%s）" },
		{ "Angle of Attack", "攻角" },
		{ "Atmosphere", "大气" },
		{ "Atmosphere Model", "大气模型" },
		{ "Atmospheric Parameters", "大气参数" },
		{ "axis and force vectors", "轴与力矢量" },
		{ "Bank", "滚转角" },
		{ "Camera mode", "相机模式" },
		{ "Camera reference frame", "相机参考系" },
		{ "Camera Target", "相机目标" },
		{ "Celestical markers", "天体标记" },
		{ "Class", "类别" },
		{ "Clear", "清除" },
		{ "Cockpit", "座舱" },
		{ "Cockpit view", "座舱视角" },
		{ "Damage model", "损伤模型" },
		{ "Declination (Dec)", "赤纬（Dec）" },
		{ "Default", "默认" },
		{ "Delete", "删除" },
		{ "Density", "密度" },
		{ "Designation", "标识" },
		{ "Docking Ports", "对接端口" },
		{ "Down", "下" },
		{ "Dry mass", "干质量" },
		{ "Dynamic Pressure", "动压" },
		{ "Eccentricity (e)", "偏心率（e）" },
		{ "Ecliptic position from primary", "相对主天体的黄道位置" },
		{ "Excentricity (e)", "偏心率（e）" },
		{ "Fixed:", "固定：" },
		{ "Flight model", "飞行模型" },
		{ "Forward", "前进" },
		{ "Geocentric Celestial Position", "地心天球位置" },
		{ "Glass cockpit MFD size", "玻璃座舱 MFD 尺寸" },
		{ "Gravitational Moments", "引力矩" },
		{ "Gravity Sources", "引力源" },
		{ "Ground", "地面" },
		{ "Ground Speed", "地速" },
		{ "Heading", "航向" },
		{ "IDLE (landed)", "空闲（已着陆）" },
		{ "Inclination (i)", "倾角（i）" },
		{ "Inclination (igg)", "倾角（igg）" },
		{ "Landing Pads", "着陆平台" },
		{ "Latitude", "纬度" },
		{ "Latitude [°]", "纬度 [°]" },
		{ "Left", "左" },
		{ "Lift/Drag Ratio", "升阻比" },
		{ "Located on", "位于" },
		{ "Location", "位置" },
		{ "Lock view on target", "锁定视角到目标" },
		{ "Long labels", "长标签" },
		{ "Longitude", "经度" },
		{ "Longitude [°]", "经度 [°]" },
		{ "Longitude of Ascending Node", "升交点经度" },
		{ "Longitude of Periapsis", "近点经度" },
		{ "MFD refresh interval [s]", "MFD 刷新间隔 [s]" },
		{ "Mach Number", "马赫数" },
		{ "Mass", "质量" },
		{ "Mean longitude", "平经度" },
		{ "Mean radius", "平均半径" },
		{ "Mode", "模式" },
		{ "Mouse focus behavior", "鼠标焦点行为" },
		{ "Movable:", "可移动：" },
		{ "Move", "移动" },
		{ "N/A", "不适用" },
		{ "G", "相对引力" },
		{ "Name", "名称" },
		{ "Numerical", "数值" },
		{ "Object frame axis", "物体坐标系轴" },
		{ "Object: %s", "物体：%s" },
		{ "Obliquity of Ecliptic", "黄赤交角" },
		{ "Orbit Period", "轨道周期" },
		{ "Options for displaying markers and gridlines on the celestial", "在天球上显示标记与网格线、" },
		{ "Osculating Elements (Ecliptic Frame)", "密切轨道根数（黄道坐标系）" },
		{ "P. moments of inertia", "主惯性矩" },
		{ "PASSIVE (attached)", "被动（已连接）" },
		{ "Pad %d", "停机坪 %d" },
		{ "Panel scroll speed", "面板滚动速度" },
		{ "Physical Parameters", "物理参数" },
		{ "Pitch", "俯仰角" },
		{ "Port %d", "接口 %d" },
		{ "Position", "位置" },
		{ "Posture", "姿态" },
		{ "Pressure", "压力" },
		{ "Primary", "主天体" },
		{ "Propellant mass", "推进剂质量" },
		{ "Radial Distance", "径向距离" },
		{ "Reference", "参考" },
		{ "Reference body", "参考天体" },
		{ "Right", "右" },
		{ "Right Ascension (RA)", "赤经（RA）" },
		{ "Runway %02d/%02d", "跑道 %02d/%02d" },
		{ "Runways", "跑道" },
		{ "Scroll panel", "滚动面板" },
		{ "Select an object on the left panel", "在左侧面板选择一个物体" },
		{ "Semi-major Axis (a)", "半长轴（a）" },
		{ "Show negative axis", "显示负轴" },
		{ "Siderial day", "恒星日" },
		{ "Solar System", "太阳系" },
		{ "Specific Gas Constant", "气体常数" },
		{ "Specific Heat Ratio", "比热比" },
		{ "sphere, labels for planetary surface features, as weel as coordinate", "行星表面特征标签，以及坐标" },
		{ "State Propagation", "状态推进" },
		{ "State Propagator", "状态推进器" },
		{ "Subsamples", "子采样" },
		{ "Surface and object labels (Alt-F9)", "地表与物体标签（Alt-F9）" },
		{ "Surface density", "表面密度" },
		{ "Surface Pressure", "表面压力" },
		{ "Surface-relative Parameters", "相对地表参数" },
		{ "Switch panel", "开关面板" },
		{ "Target distance [m]:", "目标距离 [m]：" },
		{ "Temperature", "温度" },
		{ "Thrusters Group Ratings (vacuum)", "推进器组推力（真空）" },
		{ "Total mass", "总质量" },
		{ "Track", "跟踪" },
		{ "Tracking target", "跟踪目标" },
		{ "Transponder Frequency", "应答机频率" },
		{ "True Airspeed", "真空速" },
		{ "Up", "上" },
		{ "Update mode", "更新模式" },
		{ "Vertical Speed", "垂直速度" },
		{ "View target", "视角目标" },
		{ "Virtual cockpit MFD texture size", "虚拟座舱 MFD 纹理尺寸" },

		// ---- In-simulation dialog titles -------------------------------------
		{ "Orbiter: Time acceleration", "Orbiter：时间加速" },
		{ "Orbiter: Camera", "Orbiter：相机" },
		{ "Orbiter: Object info", "Orbiter：物体信息" },
		{ "Orbiter: Help", "Orbiter：帮助" },
		{ "Orbiter: Configure menu bars", "Orbiter：配置菜单栏" },
		{ "Orbiter: Custom functions", "Orbiter：自定义功能" },
		{ "Orbiter: Capture frames", "Orbiter：捕获帧" },
		{ "Orbiter: Map", "Orbiter：地图" },
		{ "Orbiter: Select spacecraft", "Orbiter：选择载具" },
		// ---- In-simulation menu bar -----------------------------------------
		{ "Ship", "载具" },
		{ "Remove from menu", "从菜单移除" },
		{ "Camera", "相机" },
		{ "Speed", "速度" },
		{ "Pause", "暂停" },
		{ "Function", "功能" },
		{ "Info", "信息" },
		{ "Options", "选项" },
		{ "Map", "地图" },
		{ "Record", "录制" },
		{ "Help", "帮助" },
		{ "Save", "保存" },
		{ "Exit", "退出" },
		// ---- In-simulation info bars ----------------------------------------
		{ "Tgt", "目标" },
		{ "Cam Cockpit", "座舱视角" },
		{ "FoV", "视场" },
		{ "Dst", "距离" },
		{ "Sim", "模拟时间" },
		{ "Pause", "暂停" },
		{ "Cam track (rel-pos)", "相机跟踪（相对位置）" },
		{ "Cam track (abs-dir)", "相机跟踪（绝对方向）" },
		{ "Cam track (global frame)", "相机跟踪（全局坐标系）" },
		{ "Cam target to", "相机目标至" },
		{ "Cam target from", "相机目标自" },
		{ "Cam ground", "相机地面" },
		{ "(tgt-lock)", "（目标锁定）" },
		{ "(free)", "（自由）" },

		// ---- Weekday and month names (date display) -------------------------
		{ "Sun", "太阳" },
		{ "Mon", "周一" },
		{ "Tue", "周二" },
		{ "Wed", "周三" },
		{ "Thu", "周四" },
		{ "Fri", "周五" },
		{ "Sat", "周六" },
		{ "Jan", "1月" },
		{ "Feb", "2月" },
		{ "Mar", "3月" },
		{ "Apr", "4月" },
		{ "May", "5月" },
		{ "Jun", "6月" },
		{ "Jul", "7月" },
		{ "Aug", "8月" },
		{ "Sep", "9月" },
		{ "Oct", "10月" },
		{ "Nov", "11月" },
		{ "Dec", "12月" },
	};

	static const size_t kNumTranslations = sizeof(g_translations) / sizeof(g_translations[0]);

	// ========================================================================
	// State
	// ========================================================================

	static Language g_language = Language::English;
	static bool g_tablesBuilt = false;
	static std::map<std::string, std::string> g_enToZh;
	static std::map<std::string, std::string> g_zhToEn;
	static std::map<std::string, std::string> g_enToZhLower;

	static std::string NormalizeLineEndings(const std::string &s);

static void BuildTables()
	{
		if (g_tablesBuilt) return;
		g_tablesBuilt = true;
		for (size_t i = 0; i < kNumTranslations; i++) {
			const Translation &t = g_translations[i];
			if (!t.en || !t.zh) continue;
			if (strcmp(t.en, t.zh) == 0) continue;  // placeholder
			std::string en = NormalizeLineEndings(t.en);
			std::string lower = en;
			for (char &c : lower) c = (char)tolower((unsigned char)c);
			g_enToZh[en] = t.zh;
			g_enToZhLower[lower] = t.zh;
			g_zhToEn[t.zh] = en;
		}
	}

	// Normalize line endings: "\r\n" -> "\n", lone "\r" -> "\n".
	// The translation table stores line breaks as "\n".
	static std::string NormalizeLineEndings(const std::string &s)
	{
		std::string r;
		r.reserve(s.size());
		for (size_t i = 0; i < s.size(); i++) {
			if (s[i] == '\r') {
				if (i + 1 < s.size() && s[i + 1] == '\n') continue;
				r += '\n';
			} else {
				r += s[i];
			}
		}
		return r;
	}

	// ========================================================================
	// UTF-8 <-> UTF-16 conversion
	// ========================================================================

	static std::wstring Utf8ToWide(const char* utf8)
	{
		if (!utf8 || !*utf8) return std::wstring();
		int len = int(strlen(utf8));
		int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, len, nullptr, 0);
		if (wlen <= 0) return std::wstring();
		std::wstring w(size_t(wlen), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, utf8, len, &w[0], wlen);
		return w;
	}

	static std::string WideToUtf8(const wchar_t* w)
	{
		if (!w || !*w) return std::string();
		int len = int(wcslen(w));
		int n = WideCharToMultiByte(CP_UTF8, 0, w, len, nullptr, 0, nullptr, nullptr);
		if (n <= 0) return std::string();
		std::string s(size_t(n), '\0');
		WideCharToMultiByte(CP_UTF8, 0, w, len, &s[0], n, nullptr, nullptr);
		return s;
	}

	// ========================================================================
	// Configuration
	// ========================================================================

	static const char* kCfgFile = "Localization.cfg";

	const char* LocalizationConfigFile()
	{
		return kCfgFile;
	}

	void InitLocalization()
	{
		BuildTables();
		g_language = Language::English;

		std::ifstream ifs(kCfgFile);
		if (ifs) {
			std::string line;
			while (std::getline(ifs, line)) {
				while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
					line.pop_back();
				size_t pos = line.find_first_not_of(" \t");
				if (pos != std::string::npos && pos > 0)
					line = line.substr(pos);
				if (line.empty() || line[0] == ';' || line[0] == '#') continue;
				if (line.compare(0, 9, "Language=") == 0) {
					std::string v = line.substr(9);
					if (v == "zh" || v == "ZH" || v == "Chinese")
						g_language = Language::Chinese;
					else
						g_language = Language::English;
				}
			}
		}
	}

	Language GetLanguage()
	{
		return g_language;
	}

	bool IsChinese()
	{
		return g_language == Language::Chinese;
	}

	void SetLanguage(Language lang, bool save)
	{
		g_language = lang;
		BuildTables();
		if (!save) return;

		std::ofstream ofs(kCfgFile);
		if (ofs) {
			ofs << "; Orbiter localization settings\n";
			ofs << "; Language: en = English, zh = Chinese\n";
			ofs << "Language=" << (lang == Language::Chinese ? "zh" : "en") << "\n";
		}
	}


	// ------------------------------------------------------------------------
	// Alias keys: allow dedicated keys (e.g. "SURF_GS") to be translated
	// without clashing with other UI texts. The alias maps to the original
	// English text, which is returned in English mode.
	// ------------------------------------------------------------------------
	struct AliasEntry { const char* alias; const char* english; };
	static const AliasEntry g_aliases[] = {
		{ "SURF_Title", "Surface: " },
		{ "SURF_ATM",  "ATM DATA" },
		{ "SURF_OAT",  "OAT" },
		{ "SURF_M",    "M" },
		{ "SURF_DNS",  "DNS" },
		{ "SURF_STP",  "STP" },
		{ "SURF_DNP",  "DNP" },
		{ "SURF_EQP",  "EQU POS" },
		{ "SURF_RATE", "RATE" },
		{ "SURF_BNK",  "BNK" },
		{ "SURF_PTCH", "PTCH" },
		{ "SURF_L",    "L" },
		{ "SURF_R",    "R" },
		{ "SURF_ALTR", "ALT-R" },
		{ "SURF_ALT",  "ALT" },
		{ "SURF_VS",   "VS" },
		{ "SURF_VACC", "VACC" },
		{ "SURF_AOA",  "AOA" },
		{ "SURF_ACC",  "ACC" },
		{ "SURF_GS",   "GS" },
		{ "SURF_TAS",  "TAS" },
		{ "SURF_IAS",  "IAS" },
		{ "SURF_OS",   "OS" },
	};
	static const size_t kNumAliases = sizeof(g_aliases) / sizeof(g_aliases[0]);

	const char* Tr2(const char* aliasKey)
	{
		if (!aliasKey || !*aliasKey) return aliasKey;
		if (g_language == Language::English) {
			for (size_t i = 0; i < kNumAliases; i++)
				if (strcmp(g_aliases[i].alias, aliasKey) == 0) return g_aliases[i].english;
			return aliasKey;
		}
		return Tr(aliasKey);
	}
	const char* Tr(const char* english)
	{
		if (!english || !*english) return english;
		if (g_language == Language::English) return english;
		BuildTables();
		std::string key = NormalizeLineEndings(english);
		auto it = g_enToZh.find(key);
		if (it != g_enToZh.end()) return it->second.c_str();
		{
			std::string lower = key;
			for (char &c : lower) c = (char)tolower((unsigned char)c);
			auto it2 = g_enToZhLower.find(lower);
			if (it2 != g_enToZhLower.end()) return it2->second.c_str();
		}
		static std::set<std::string> missSet;
		if (missSet.size() < 300 && missSet.insert(key).second) {
			// Only log ASCII keys (dynamic object names are noise)
			bool ascii = true;
			for (char c : key) if ((unsigned char)c >= 0x80) { ascii = false; break; }
			if (ascii) LOGOUT("LOCALIZATION miss: [%s]", english);
		}
		return english;
	}

	std::wstring Utf8ToWideString(const char* utf8)
	{
		return Utf8ToWide(utf8);
	}

	void ComboAddStringUtf8(HWND hCombo, const char* utf8)
	{
		if (!hCombo || !utf8) return;
		std::wstring w = Utf8ToWide(utf8);
		SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)w.c_str());
	}

	void ListBoxAddStringUtf8(HWND hListBox, const char* utf8)
	{
		if (!hListBox || !utf8) return;
		std::wstring w = Utf8ToWide(utf8);
		SendMessageW(hListBox, LB_ADDSTRING, 0, (LPARAM)w.c_str());
	}

	void SetWindowTextUtf8(HWND hWnd, const char* utf8)
	{
		if (!hWnd) return;
		if (!utf8) { SetWindowTextW(hWnd, L""); return; }
		std::wstring w = Utf8ToWide(utf8);
		SetWindowTextW(hWnd, w.c_str());
	}

	// ========================================================================
	// Window tree localization
	// ========================================================================

	// Rebuild the item list of a combobox in the active language. Item data and
	// the current selection are preserved. Comboboxes whose items have no
	// translation are left untouched.
	// Read one combobox item as UTF-16. Combobox messages have no ANSI/Unicode
	// variants: the string encoding is determined by the window's Unicode flag.
	static std::wstring ComboGetItemText(HWND hCombo, int index)
	{
		if (IsWindowUnicode(hCombo)) {
			wchar_t wbuf[512];
			LRESULT len = SendMessageW(hCombo, CB_GETLBTEXT, (WPARAM)index, (LPARAM)wbuf);
			if (len == CB_ERR || len < 0) return std::wstring();
			return std::wstring(wbuf, size_t(len));
		}
		char abuf[1024];
		LRESULT len = SendMessageW(hCombo, CB_GETLBTEXT, (WPARAM)index, (LPARAM)abuf);
		if (len == CB_ERR || len < 0) return std::wstring();
		int n = MultiByteToWideChar(CP_ACP, 0, abuf, int(len), nullptr, 0);
		if (n <= 0) return std::wstring();
		std::wstring w(size_t(n), L'\0');
		MultiByteToWideChar(CP_ACP, 0, abuf, int(len), &w[0], n);
		return w;
	}

	static void ComboAddItemText(HWND hCombo, const std::wstring& text)
	{
		if (IsWindowUnicode(hCombo)) {
			SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)text.c_str());
			return;
		}
		int n = WideCharToMultiByte(CP_ACP, 0, text.c_str(), int(text.length()), nullptr, 0, nullptr, nullptr);
		if (n <= 0) { SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)""); return; }
		std::string s(size_t(n), '\0');
		WideCharToMultiByte(CP_ACP, 0, text.c_str(), int(text.length()), &s[0], n, nullptr, nullptr);
		SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)s.c_str());
	}

	// Rebuild the item list of a combobox in the active language. Item data and
	// the current selection are preserved. Comboboxes whose items have no
	// translation are left untouched.
	static void LocalizeComboBox(HWND hCombo)
	{
		int count = (int)SendMessageW(hCombo, CB_GETCOUNT, 0, 0);
		if (count <= 0) return;

		struct Item {
			std::wstring text;
			LRESULT data;
			bool hasData;
		};
		std::vector<Item> items;
		items.reserve(size_t(count));
		bool changed = false;

		for (int i = 0; i < count; i++) {
			std::wstring wtext = ComboGetItemText(hCombo, i);
			std::string current = WideToUtf8(wtext.c_str());
			std::string target = RetranslateCurrent(current.c_str());
			if (target != current) changed = true;
			Item it;
			it.text = Utf8ToWide(target.c_str());
			it.data = SendMessageW(hCombo, CB_GETITEMDATA, (WPARAM)i, 0);
			it.hasData = (it.data != CB_ERR);
			items.push_back(it);
		}
		if (!changed) return;   // leave dynamic lists (devices, modes, ...) alone

		LRESULT sel = SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
		// Clear the selection first to avoid CBN_SELCHANGE notifications while
		// the list is rebuilt (they could re-enter the language switch).
		SendMessageW(hCombo, CB_SETCURSEL, (WPARAM)-1, 0);
		SendMessageW(hCombo, CB_RESETCONTENT, 0, 0);
		for (size_t i = 0; i < items.size(); i++) {
			ComboAddItemText(hCombo, items[i].text);
			if (items[i].hasData)
				SendMessageW(hCombo, CB_SETITEMDATA, (WPARAM)i, (LPARAM)items[i].data);
		}
		if (sel != CB_ERR)
			SendMessageW(hCombo, CB_SETCURSEL, (WPARAM)sel, 0);
	}
	static void LocalizeWindow(HWND hWnd)
	{
		wchar_t wbuf[2048];
		int len = GetWindowTextW(hWnd, wbuf, 2048);
		if (len <= 0) return;

		std::string current = WideToUtf8(wbuf);
		if (current.empty()) return;

		BuildTables();
		std::string key = NormalizeLineEndings(current);
		std::string target = current;
		if (g_language == Language::Chinese) {
			auto it = g_enToZh.find(key);
			if (it != g_enToZh.end()) target = it->second;
		} else {
			auto it = g_zhToEn.find(key);
			if (it != g_zhToEn.end()) target = it->second;
		}
		if (target == current) return;

		std::wstring wtarget = Utf8ToWide(target.c_str());
		SetWindowTextW(hWnd, wtarget.c_str());
	}

		static BOOL CALLBACK LocalizeChildProc(HWND hWnd, LPARAM lParam)
	{
		wchar_t cls[64];
		GetClassNameW(hWnd, cls, 64);
		if (wcscmp(cls, L"ComboBox") == 0)
			LocalizeComboBox(hWnd);
		else
			LocalizeWindow(hWnd);
		return TRUE;
	}

	static bool g_relocalizing = false;
	bool IsRelocalizing() { return g_relocalizing; }
	void SetRelocalizing(bool active) { g_relocalizing = active; }

	void LocalizeWindowTree(HWND root)
	{
		if (!root) return;
		LocalizeWindow(root);
		EnumChildWindows(root, LocalizeChildProc, 0);
	}

	// ========================================================================
	// Language selector combobox (Launchpad)
	// ========================================================================

	static const wchar_t* kLangNames[2] = { L"English", L"中文（简体）" };

	void InitLanguageCombo(HWND hCombo)
	{
		if (!hCombo) return;
		SendMessageW(hCombo, CB_RESETCONTENT, 0, 0);
		for (int i = 0; i < 2; i++)
			SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)kLangNames[i]);
		SyncLanguageCombo(hCombo);
	}

	void SyncLanguageCombo(HWND hCombo)
	{
		if (!hCombo) return;
		SendMessageW(hCombo, CB_SETCURSEL, (WPARAM)(g_language == Language::Chinese ? 1 : 0), 0);
	}

	void ApplyLanguageFromCombo(HWND hCombo)
	{
		if (!hCombo) return;
		LRESULT idx = SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
		if (idx == CB_ERR) return;
		SetLanguage(idx == 1 ? Language::Chinese : Language::English, true);
	}

	std::string WideToUtf8String(const wchar_t* w)
	{
		return WideToUtf8(w);
	}

	std::string RetranslateCurrent(const char* currentUtf8)
	{
		if (!currentUtf8) return std::string();
		BuildTables();
		std::string key = NormalizeLineEndings(currentUtf8);
		std::string english = key;
		auto rev = g_zhToEn.find(key);
		if (rev != g_zhToEn.end()) english = rev->second;
		if (g_language == Language::English) return english;
		auto fwd = g_enToZh.find(english);
		return (fwd != g_enToZh.end()) ? fwd->second : english;
	}
} // namespace orbiter
