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
		// ---- addendum ----------------------------------------------------------
		// ---- surface bases (Config/<planet>/Base/*.cfg) ----
		{ "Al Anbar", "安巴尔" },
		{ "Alcantara", "阿尔坎塔拉" },
		{ "Baikonur", "拜科努尔" },
		{ "Barent Sea", "巴伦支海" },
		{ "Brighton Beach", "布莱顿海滩" },
		{ "Cape Canaveral", "卡纳维拉尔角" },
		{ "China Lake", "中国湖" },
		{ "Doberai", "多贝莱" },
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
		{ "Mojave Spaceport", "莫哈维航天港" },
		{ "Musudan", "舞水端" },
		{ "Olympus", "奥林匹斯" },
		{ "Overberg", "奥弗贝格" },
		{ "Palmachim", "帕尔马希姆" },
		{ "Palmdale AF Plant 42", "帕姆代尔空军42厂" },
		{ "Peenemuende", "佩内明德" },
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
		{ "radius", "半径" },
		{ "longitude", "经度" },
		{ "latitude", "纬度" },
		{ "d radius / dt", "d 半径 / dt" },
		{ "d longitude / dt", "d 经度 / dt" },
		{ "d latitude / dt", "d 纬度 / dt" },
		{ "Inactive (Landed)", "未激活 (已着陆)" },
		{ "Active (Flight)", "激活 (飞行中)" },
		{ "Animations", "动画" },
		{ "Passengers", "乘客" },
		{ "polar", "极坐标" },
		// ---- MFD mode names (SEL menu) ----
		{ "TransX", "转移X" },
		{ "Ascent profile", "上升剖面" },
		{ "Attitude RCS", "姿态控制" },
		{ "Terminal MFD", "终端" },
		{ "MFD Template", "MFD模板" },
		{ "Script MFD API-Test", "脚本API测试" },
		{ "Generic Camera", "通用相机" },
		// ---- Terminal MFD (LuaMFD) ----
		{ "Input command", "输入命令" },
		// ---- Generic Camera MFD ----
		{ "Next attachment", "下一个附件" },
		// ---- Ascent profile MFD ----
		{ "Time: s", "时间: s" },
		// ---- Attitude RCS MFD (script) button menus ----
		{ "Set attitude mode", "设置姿态模式" },
		// ---- TransX MFD ----
		{ "Pe Distance", "近点距离" },
		{ "Ej Orientation", "弹射方向" },
		{ "Equatorial view", "赤道视图" },
		{ "Draw Base", "绘制基地" },
		{ "View Orbit", "查看轨道" },
		{ "Velocity.", "速度" },
		{ "Outward angle", "外推角" },
		{ "Inc. angle", "倾角" },
		{ "Inherit Vel.", "继承速度" },
		{ "Eject date", "出发日期" },
		{ "Prograde vel.", "顺行速增量" },
		{ "Outward vel.", "离心速增量" },
		{ "Ch. plane vel.", "法向速增量" },
		{ "Stage %i:%i", "阶段 %i:%i" },
		{ "Vars Stage %i", "变量阶段 %i" },
		{ "General Setup", "通用设置" },
		{ "Toggle Help", "切换帮助" },
		{ "Step Forward", "前进一阶段" },
		{ "Step Back", "后退一阶段" },
		{ "Select View", "选择视图" },
		{ "Next Variable", "下一个变量" },
		{ "Prev. Variable", "上一个变量" },
		{ "Set adjustment", "设置调整" },
		{ "method (+)", "方法 (+)" },
		{ "method (-)", "方法 (-)" },
		{ "Increment", "增加" },
		{ "variable", "变量" },
		{ "Decrement", "减少" },
		{ "Execute", "执行" },
		{ "Command", "命令" },
		{ "Enter value", "输入数值" },
		{ "Executed", "已执行" },
		{ "TransX MFD", "转移X MFD" },
		{ "Plan:Escape", "计划:逃逸" },
		// ---- TransX help system ----
		{ "Select where you're going.", "选择目标天体。" },
		{ "Choose object, or escape", "选择天体，或逃逸" },
		{ "Advanced:Select type of plan", "高级：选择计划类型" },
		{ "you want to follow.", "你想执行的计划。" },
		{ "None for simple trips.", "无：简单行程。" },
		{ "Escape when leaving SOI", "逃逸：离开影响球" },
		{ "Choose what to do at this", "选择在此天体的操作" },
		{ "planet", "天体" },
		{ "None for simple trips,", "无：简单行程，" },
		{ "Eject for leaving an SOI", "出发：离开影响球" },
		{ "Advanced:Set minor body", "高级：设置次天体" },
		{ "Enable manoeuvre settings", "启用机动设置" },
		{ "Enable advanced settings", "启用高级设置" },
		{ "Set to 'Target' to obtain better", "设为'目标'以获得更好的" },
		{ "view when target is small", "目标较小时的视图" },
		{ "Orbit on which manoeuvres", "机动所基于的轨道" },
		{ "are based. ++ sets to focus orbit", "++ 可设为焦点轨道" },
		{ "Positive numbers to outer planets.", "正数：飞向外部行星。" },
		{ "Negative numbers to inner planets.", "负数：飞向内部行星。" },
		{ "Your planned launch date/time", "计划的发射日期/时间" },
		{ "Alter to seek good launch windows", "调整以寻找良好发射窗口" },
		{ "Add velocity outwards/inwards.", "向外/向内增加速度。" },
		{ "Allows launch date flexibility.", "提供发射日期灵活性。" },
		{ "Use to swing grey plane intersection", "用于摆动灰色平面交线" },
		{ "line towards intercept point.", "朝向交会点。" },
		{ "Adjusts the way MFD displays orbit", "调整 MFD 显示轨道的方式" },
		{ "Adjust to taste", "按喜好调整" },
		{ "Intercept info shown for whatever", "交会信息显示取决于" },
		{ "you select here.", "交会信息。" },
		{ "The number of orbits ahead to", "向前搜索交会的" },
		{ "look for an intercept with target", "与目标交会的轨道圈数" },
		{ "This plan (None) is used largely for", "此计划(无)主要用于" },
		{ "simple cruise to a target.", "简单巡航到目标。" },
		{ "Select an alternative plan for more", "选择其他计划以进行更" },
		{ "complex manoeuvres. Use FWD to", "复杂机动。用 FWD 创建" },
		{ "create next stage.", "下一阶段。" },
		{ "Periapsis of planned orbit", "计划轨道的近点" },
		{ "(Distance from planet centre)", "(距行星中心距离)" },
		{ "Use to rotate planned orbit around", "用于绕出发方向轴" },
		{ "axis of departure direction.", "旋转计划轨道。" },
		{ "Used to leave SOI of this planet/moon", "用于离开此行星/卫星的影响球" },
		{ "for another. eg Earth to", "前往另一个，例如地球到" },
		{ "Mars. Best to plan before you take", "火星。最好在起飞前规划。" },
		{ "off. Most efficient way to plan", "最有效的规划方式" },
		{ "Choose to not draw the base", "选择不绘制基地" },
		{ "Used to show information on the", "用于显示到达行星的信息。" },
		{ "arrival planet. No function other than", "除了显示行星及地面基地的" },
		{ "to show extra information on a planet", "额外信息外，无其他功能" },
		{ "including a surface base", "（包括地面基地）" },
		{ "Choose to view approach", "选择查看接近轨道" },
		{ "or departure orbit", "或离开轨道" },
		{ "Total velocity of escape", "逃逸总速度" },
		{ "Angle away from prograde direction", "偏离顺行方向的角度" },
		{ "(Direction of Minor body)", "(次天体方向)" },
		{ "Rotates around prograde vector", "绕顺行矢量旋转" },
		{ "Inherit velocity and date from", "从上一阶段计划继承" },
		{ "previous stage's plan.", "速度和日期。" },
		{ "Date of periapsis of minor body.", "次天体近点日期。" },
		{ "Normally leave as inherited.", "通常保持继承值。" },
		{ "for another. Designed for use in", "前往另一个。设计用于与" },
		{ "combination with the slingshot plan", "上一阶段的弹弓计划" },
		{ "in the previous stage", "组合使用" },
		{ "Positive to move outward from MAJ", "正数：远离主天体" },
		{ "Negative to move inward toward MAJ", "负数：靠近主天体" },
		{ "Your planned launch date. Adjust to", "计划发射日期。调整以" },
		{ "find efficient launch window.", "寻找高效发射窗口。" },
		{ "Adjust to give you launch date", "调整以提供发射日期" },
		{ "flexibility or fast flight.", "灵活性或快速飞行。" },
		{ "Adjust grey target plane intersect", "调整灰色目标平面交线" },
		{ "line to coincide with intercept.", "使其与交会点重合。" },
		{ "Used to set direction at exit of", "用于设置离开影响球时的" },
		{ "SOI to aim for another planet eg", "方向以瞄准另一行星，例如" },
		{ "Earth to Mars. MAJ is body all", "地球到火星。MAJ 为轨道" },
		{ "orbit. Best to plan before you", "中心。最好在起飞前规划。" },
		{ "take off.", "起飞。" },
		{ "Select the ship from which", "选择要继承设置的飞船" },
		{ "you wish to inherit settings.", "（选择载具）。" },
		{ "View:Escape Plan", "视图:逃逸计划" },
		{ "Plan:Eject", "计划:出发" },
		{ "View:Eject Plan", "视图:出发计划" },
		{ "Plan:Sling Direct", "计划:直接抛射" },
		{ "View:Sling Direct", "视图:直接抛射" },
		{ "Plan:Encounter", "计划:遭遇" },
		{ "View:Encounter", "视图:遭遇" },
		{ "Plan:Slingshot", "计划:弹弓" },
		{ "View:Slingshot", "视图:弹弓" },
		{ "Plan type", "计划类型" },
		{ "Plan", "计划" },
		{ "Intercept with", "交会对象" },
		{ "Orbits to Icept", "交会轨道圈数" },
		{ "Scale to view", "缩放视图" },
		{ "Autoplan", "自动规划" },
		{ "Manoeuvre mode", "机动模式" },
		{ "Base Orbit", "基准轨道" },
		{ "Select Minor", "选择次天体" },
		{ "Updating", "更新中" },
		{ "++ Updates", "++ 更新" },
		{ "On", "开" },
		{ "Off", "关" },
		{ "Coarse", "粗略" },
		{ "Inc:", "倾角:" },
		{ "Enc. MJD %.4f", "交会 MJD %.4f" },
		{ "LAN    :%.4g°", "升交点经度:%.4g°" },
		{ "Pe Lat/Long: %.4g, %.4g", "近点纬/经度: %.4g, %.4g" },
		{ "Medium", "中等" },
		{ "Rough", "粗糙" },
		{ "Super", "超精细" },
		{ "Ultra", "极精细" },
		{ "Hyper", "超高精" },
		{ "Micro", "微调" },
		{ "Auto-Min", "自动最小" },
		{ "Reset", "重置" },
		{ "Min Alt:", "最低高度:" },
		{ "Pe Vel:", "近点速度:" },
		{ "Capture Delta:", "捕获增量:" },
		{ "Circ. Delta:", "圆化增量:" },
		{ "Inclination:", "倾角:" },
		{ "Incl.  :%.4g°", "倾角:%.4g°" },
		{ "Heading:%.4g°", "航向:%.4g°" },
		{ "No solution", "无解" },
		{ "not available", "不存在" },
		{ "Heading:", "航向:" },
		{ "Heading: N/A", "航向:不可用" },
		{ "Delta V:", "速度增量:" },
		{ "T to Pe:", "至近点时间:" },
		{ "Begin Burn:", "开始点火:" },
		{ "Begin Burn", "开始点火" },
		{ "Ang. to Pe:", "至近点角度:" },
		{ "S.maj diff:", "半长轴差:" },
		{ "LAN ", "升交点经度 " },
		{ "TOF (days):", "飞行时间(天):" },
		{ "Total DeltaV:", "总速度增量:" },
		{ "Rel Inc:", "相对倾角:" },
		{ "R. Inc:", "相对倾角:" },
		{ "New Plan", "新计划" },
		{ "Inherit from", "继承自" },
		{ "Man. date", "机动日期" },
		{ "Fine", "精细" },
		{ "All", "全部" },
		{ "Target", "目标" },
		{ "Craft", "飞船" },
		{ "Graph projection", "图形投影" },
		{ "Initial", "初始" },
		{ "Through point", "通过点" },
		{ "TX_Through point", "飞掠" },
		{ "Cruise plan", "巡航" },
		{ "None", "无" },
		{ "Slingshot", "弹弓" },
		{ "Encounter", "遭遇" },
		{ "Eject", "出发" },
		{ "Sling Direct", "直接抛射" },
		{ "Auto", "自动" },
		{ "Manoeuvre", "机动" },
		{ "Focus", "焦点" },
		{ "Ecliptic", "黄道" },
		{ "Edge On", "侧视" },
		{ "Pe MJD:", "近点 MJD:" },
		{ "H. Pe MJD", "双曲近点 MJD" },
		{ "On/Off", "开/关" },
		{ "/ Add Step", "/ 添加阶段" },
		{ "MAJ:", "主天体:" },
		{ "MIN:", "次天体:" },
		{ "TGT:", "目标:" },
		{ "View:Target", "视图:目标" },
		{ "View:Setup", "视图:设置" },
		{ "View:Manoeuvre", "视图:机动" },
		{ "Delta V: ", "速度增量: " },
		{ "T to Mnvre: ", "至机动: " },
		{ "Begin Burn: ", "开始点火: " },
		{ "Cl. App.: ", "最近接近: " },
		{ "Hohmann dv: ", "霍曼增量: " },
		{ "Enc. V:", "交会速度:" },
		{ "Maj. Rad: ", "主天体半径: " },
		{ "Focus PeD:", "主天体近点距:" },
		{ "Focus ApD:", "主天体远点距:" },
		{ "Hyp PeD ", "双曲近点距 " },
		{ "Planets/Moons", "行星/卫星" },
		{ "Ships", "飞船" },
		{ "Pe MJD:   %.4f", "近点 MJD:   %.4f" },
		{ "H. Pe MJD %.2f", "双曲近点 MJD %.2f" },
		{ "Escape", "逃逸" },
		{ "No Command", "无命令" },
		{ "Dock sync mode", "对接同步模式" },
		{ "Suspend active mode", "挂起活动模式" },
		{ "Cancel active mode", "取消活动模式" },
		{ "Configure vessel", "配置载具" },
		{ "Add rotation", "添加旋转" },
		{ "Delete rotation", "删除旋转" },
		{ "Change rotation", "更改旋转" },
		{ "axis", "轴" },
		{ "Increase rotation", "增大旋转" },
		{ "angle", "角度" },
		{ "Decrease rotation", "减小旋转" },
		{ "Selection up", "向上选择" },
		{ "Selection down", "向下选择" },
		{ "Activate configuration", "激活配置" },
		{ "Return to", "返回" },
		{ "main page", "主页面" },
		{ "Next base mode", "下一个基准模式" },
		{ "Invert base mode", "反转基准模式" },
		{ "Keep current", "保持当前" },
		{ "attitude", "姿态" },
		{ "Activate mode", "激活模式" },
		{ "NAV receiver", "NAV 接收机" },
		{ "select", "选择" },
		{ "Activate dock", "激活对接" },
		{ "alignment mode", "对准模式" },
		{ "Alt: km", "高度: km" },
		{ "Pitch: deg", "俯仰: 度" },
		{ "Vel: km/s", "速度: km/s" },
		{ "V rad", "径向速度" },
		{ "V tan", "切向速度" },
		{ "Select page", "选择页面" },
		{ "Altitude range", "高度范围" },
		{ "Radial", "径向" },
		{ "Tangential", "切向" },
		{ "velocity range", "速度范围" },
		{ "Altitude range (km) [min max, or 'a' for auto]:", "高度范围 (km) [最小值 最大值，或 'a' 自动]：" },
		{ "Vrad range (km/s) [min max, or 'a' for auto]:", "径向速度范围 (km/s) [最小值 最大值，或 'a' 自动]：" },
		{ "Vtan range (km/s) [min max, or 'a' for auto]:", "切向速度范围 (km/s) [最小值 最大值，或 'a' 自动]：" },
		{ "Prev attachment", "上一个附件" },
		{ "Next dockport", "下一个对接口" },
		{ "Prev dockport", "上一个对接口" },
		{ "Move Forward", "向前移动" },
		{ "Move Backwards", "向后移动" },
		{ "Select Vessel", "选择载具" },
		{ "Night Vision", "夜视" },
		{ "Zoom In", "放大" },
		{ "Zoom Out", "缩小" },
		{ "Parent Mode", "父级模式" },
		{ "Crosshairs", "十字准线" },
		{ "No Graphics API", "无图形 API" },
		{ "Custom Cameras Disabled", "自定义相机已禁用" },
		{ "No Dock/Attachment points", "无对接/挂载点" },
		{ "Attach(", "挂载(" },
		{ "Dock(", "对接(" },
		{ "Child", "子级" },
		{ "Parent", "父级" },
		{ "Viewing %s %s%d)%s", "查看 %s %s%d)%s" },
		{ "[%s] FOV=%0.0f° Ofs=%2.2f[m]", "[%s] 视场=%0.0f° 偏移=%2.2f[m]" },
		{ "Keyboard Input:", "键盘输入：" },
		{ "Create new terminal", "新建终端" },
		{ "Delete terminal page", "删除终端页" },
		{ "Next page", "下一页" },
		{ "Previous page", "上一页" },
		{ "Input script command:", "输入脚本命令：" },
		{ "Term %d/%d", "终端 %d/%d" },
		{ "busy", "忙" },
		{ "OK", "确定" },
		{ "Cancel", "取消" },
		{ "Execution", "执行" },
		{ "%06.2f, range %sm", "%06.2f，范围 %sm" },
		{ "Execute the command buffer", "执行命令缓冲" },
		{ "Single line", "单行" },
		{ "History", "历史" },
		{ "Previous command", "上一条命令" },
		{ "Next command", "下一条命令" },
		{ "Clear history", "清空历史" },
		{ "Maintain fixed state vectors", "保持固定状态向量" },
		{ "Maintain fixed surface position", "保持固定表面位置" },
		{ "Propagate along osculating elements", "沿密切根数传播" },
		{ "Destroy vessel", "销毁载具" },
		{ "ref. equator (fixed)", "参考赤道（固定）" },
		{ "ref. equator (rotating)", "参考赤道（旋转）" },
		{ "This dialog allows to visualize active sound instances", "此对话框用于显示当前激活的声音实例" },
		{ "rad", "弧度" },
		{ "Linear", "线性" },

		{ "Global configuration for the default Space Shuttle Atlantis.", "默认航天飞机亚特兰蒂斯的全局配置。" },
		{ "Global configuration for the default Delta-glider.", "默认三角翼的全局配置。" },
		{ "Configure atmospheric parameters for celestial bodies.", "配置天体的大气参数。" },
		{ "Visible (DSS2/Wikisky)", "可见光（DSS2/Wikisky）" },
		{ "Far IR (IRAS)", "远红外（IRAS）" },
		{ "Microwave (Planck)", "微波（普朗克）" },
		{ "Radio", "射电" },
		{ "X-ray (RASS)", "X 射线（RASS）" },
		{ "Gamma (Fermi)", "伽马射线（费米）" },
		{ "Hydrogen Alpha", "氢阿尔法" },

		{ "Runge-Kutta, 5th order (RK5)", "龙格-库塔，五阶（RK5）" },
		{ "Runge-Kutta, 6th order (RK6)", "龙格-库塔，六阶（RK6）" },
		{ "Runge-Kutta, 7th order (RK7)", "龙格-库塔，七阶（RK7）" },
		{ "Runge-Kutta, 8th order (RK8)", "龙格-库塔，八阶（RK8）" },
		{ "Symplectic, 2nd order (SY2)", "辛方法，二阶（SY2）" },
		{ "Symplectic, 4th order (SY4)", "辛方法，四阶（SY4）" },
		{ "Symplectic, 6th order (SY6)", "辛方法，六阶（SY6）" },
		{ "Symplectic, 8th order (SY8)", "辛方法，八阶（SY8）" },
		{ "Select \"Simple text display\" if the inline HTML viewer causes problems. \"Auto-detect\" disables the inline viewer when running Orbiter in a Linux/WINE environment.", "如果内嵌 HTML 查看器出现问题，请选择\"简易文本显示\"。\"自动检测\"会在 Linux/WINE 环境下运行 Orbiter 时禁用内嵌查看器。" },
		{ "If activated, the screenshot is displayed in the description of the \"Current state\" scenario.", "如果启用，截图将显示在\"当前状态\"场景的说明中。" },
		{ "Simple text display (external HTML viewer)", "简易文本显示（外部 HTML 查看器）" },
		{ "Scenario screenshots", "场景截图" },
		{ "Save screenshot when closing simulation session", "关闭模拟会话时保存截图" },

		// ---- Launchpad: main dialog -----------------------------------------
		{ "OpenOrbiter Launchpad", "OpenOrbiter 启动器" },
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
		// ---- Constellations (IAU 88) ----
		{ "Andromeda", "仙女座" },
		{ "Antlia", "唧筒座" },
		{ "Apus", "天燕座" },
		{ "Aquarius", "宝瓶座" },
		{ "Aquila", "天鹰座" },
		{ "Ara", "天坛座" },
		{ "Aries", "白羊座" },
		{ "Auriga", "御夫座" },
		{ "Bootes", "牧夫座" },
		{ "Boötes", "牧夫座" },
		{ "Caelum", "雕具座" },
		{ "Camelopardalis", "鹿豹座" },
		{ "Cancer", "巨蟹座" },
		{ "Canes Venatici", "猎犬座" },
		{ "Canis Major", "大犬座" },
		{ "Canis Minor", "小犬座" },
		{ "Capricornus", "摩羯座" },
		{ "Carina", "船底座" },
		{ "Cassiopeia", "仙后座" },
		{ "Centaurus", "半人马座" },
		{ "Cepheus", "仙王座" },
		{ "Cetus", "鲸鱼座" },
		{ "Chamaeleon", "蝘蜓座" },
		{ "Circinus", "圆规座" },
		{ "Columba", "天鸽座" },
		{ "Coma Berenices", "后发座" },
		{ "Corona Australis", "南冕座" },
		{ "Corona Borealis", "北冕座" },
		{ "Corvus", "乌鸦座" },
		{ "Crater", "巨爵座" },
		{ "Crux", "南十字座" },
		{ "Cygnus", "天鹅座" },
		{ "Delphinus", "海豚座" },
		{ "Dorado", "剑鱼座" },
		{ "Draco", "天龙座" },
		{ "Equuleus", "小马座" },
		{ "Eridanus", "波江座" },
		{ "Fornax", "天炉座" },
		{ "Gemini", "双子座" },
		{ "Grus", "天鹤座" },
		{ "Hercules", "武仙座" },
		{ "Horologium", "时钟座" },
		{ "Hydra", "长蛇座" },
		{ "Hydrus", "水蛇座" },
		{ "Indus", "印第安座" },
		{ "Lacerta", "蝎虎座" },
		{ "Leo", "狮子座" },
		{ "Leo Minor", "小狮座" },
		{ "Lepus", "天兔座" },
		{ "Libra", "天秤座" },
		{ "Lupus", "豺狼座" },
		{ "Lynx", "天猫座" },
		{ "Lyra", "天琴座" },
		{ "Mensa", "山案座" },
		{ "Microscopium", "显微镜座" },
		{ "Monoceros", "麒麟座" },
		{ "Musca", "苍蝇座" },
		{ "Norma", "矩尺座" },
		{ "Octans", "南极座" },
		{ "Ophiuchus", "蛇夫座" },
		{ "Orion", "猎户座" },
		{ "Pavo", "孔雀座" },
		{ "Pegasus", "飞马座" },
		{ "Perseus", "英仙座" },
		{ "Phoenix", "凤凰座" },
		{ "Pictor", "绘架座" },
		{ "Pisces", "双鱼座" },
		{ "Piscis Austrinus", "南鱼座" },
		{ "Puppis", "船尾座" },
		{ "Pyxis", "罗盘座" },
		{ "Reticulum", "网罟座" },
		{ "Sagitta", "天箭座" },
		{ "Sagittarius", "人马座" },
		{ "Scorpius", "天蝎座" },
		{ "Sculptor", "玉夫座" },
		{ "Scutum", "盾牌座" },
		{ "Serpens", "巨蛇座" },
		{ "Sextans", "六分仪座" },
		{ "Taurus", "金牛座" },
		{ "Telescopium", "望远镜座" },
		{ "Triangulum", "三角座" },
		{ "Triangulum Australe", "南三角座" },
		{ "Tucana", "杜鹃座" },
		{ "Ursa Major", "大熊座" },
		{ "Ursa Minor", "小熊座" },
		{ "Vela", "船帆座" },
		{ "Virgo", "室女座" },
		{ "Volans", "飞鱼座" },
		{ "Vulpecula", "狐狸座" },
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
		{ "Propagator:", "推进方法：" },
		{ "Step targets:", "步长目标：" },
		{ "time [s]", "时间[秒]" },
		{ "rotation [°]", "角度[度]" },
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
		{ "Enable stabilisation for orbit steps larger than", "当轨道步长大于整条轨道的百分之" },
		{ "% of full orbit", "整轨道的百分比" },
		{ "Gravitational field perturbation limit", "引力场摄动限制" },
		{ "Suppress stabilisation if gravity perturbations are greater than", "当引力摄动大于以下值时禁用稳定化：" },
		{ "% of total field", "总场强的百分比" },
		{ "Subsampling of perturbation integration", "摄动积分子采样" },
		{ "Target orbit step", "目标轨道步长为整条轨道的百分之" },
		{ "Max. steps", "最大步数" },
		{ "Perturbation model", "摄动模型" },
		{ "Suppress nonspherical gravity perturbations for orbit steps greater than", "当轨道步长大于整条轨道的百分之" },
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
		{ "TLe", "出发经度" },
		{ "DTe", "出发时间" },
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
		{ "ORBIT_PRJ_ECL", "黄道" },
		{ "ORBIT_PRJ_SHIP", "自身" },
		{ "ORBIT_PRJ_TGT", "目标" },

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
		{ "Alt", "高度" },
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
		{ "Peenemuende", "佩内明德" },
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
		{ "Graphs", "图表" },
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
				{ "Orbiter Remote Vessel Control", "Orbiter 远程载具控制" },
				{ "Save to file", "保存到文件" },
						{ "Audio debug", "音频调试" },
				{ "of %d", "/ %d" },
		{ "planet rad.", "行星半径" },
		{ "current", "当前" },
		{ "ecliptic", "黄道" },
		{ "ref. equator", "参考赤道" },
		{ "cartesian", "直角坐标" },
		{ "Orbital", "轨道" },
		{ "Suborbital", "亚轨道" },
		{ "Now", "现在" },
		{ "<none>", "<无>" },
		{ "Mass (0-%0.2f kg)", "质量 (0-%0.2f kg)" },
		{ "Establish docking connection with", "建立对接连接对象" },
				{ "Atmosphere", "大气" },
		{ "Graphs", "图表" },
		{ "Temperature (K)", "温度 (K)" },
		{ "Pressure (kPa)", "压强 (kPa)" },
		{ "Temperature", "温度" },
		{ "Static", "静压" },
		{ "Dynamic", "动压" },
		{ " ___TEMP", " ___温度" },
		{ " _______STP", " _______静压" },
		{ " _______DNP", " _______动压" },
		{ "Lift & Drag", "升力与阻力" },
		{ "Force (kN)", "力 (kN)" },
		{ "L/D", "升阻比" },
		{ "Lift", "升力" },
		{ "Drag", "阻力" },
		{ " _____LIFT", " _____升力" },
		{ " _____DRAG", " _____阻力" },
		{ " _____L/D", " _____升阻比" },
		{ "Mass", "质量" },
		{ "Mass (Ton)", "质量 (吨)" },
		{ "Total", "总质量" },
		{ "Propellant", "推进剂" },
		{ " _TOTMASS", " _总质量" },
		{ " _PRPMASS", " _推进剂" },
		{ "AoA", "攻角" },
		{ "Angle (°)", "角度 (°)" },
		{ "AOA", "攻角" },
		{ "Slip", "侧滑" },
		{ " ____AOA", " ____攻角" },
		{ " ___SLIP", " ___侧滑" },
		{ "Vertical speed", "垂直速度" },
		{ "Airspeed", "空速" },
		{ " ___VSPEED", " ___垂直速度" },
		{ " _AIRSPEED", " _空速" },
		{ "This dialog allows to visualize active sounds", "此对话框用于显示当前激活的声音" },
		{ "PLAYING", "播放中" },
		{ "PAUSED", "已暂停" },
		{ "STOPPED", "已停止" },
		{ "Looping", "循环" },
		{ "One shot", "单次" },
		{ "Orbiter Performance Meter", "Orbiter 性能计量器" },
		{ "Lua Console", "Lua 控制台" },
		{ "Mesh Debugger", "网格调试器" },
		{ "Visual:", "视觉体:" },
		{ "Mesh:", "网格:" },
		{ "Group:", "组:" },
		{ "(0 to %d)", "(0 到 %d)" },
		{ "Altitude", "高度" },
		{ "Altitude (km)", "高度 (km)" },
		{ "Speed", "速度" },
		{ "Speed (km/s)", "速度 (km/s)" },
		{ "Mach number", "马赫数" },
		{ " _______ALT", " _______高度" },
		{ " __MACH", " __马赫" },
		{ "Sampling period", "采样周期" },
		{ "Reset", "重置" },
		{ "Start", "开始" },
		{ "Stop", "停止" },
		{ "Mach", "马赫" },
		{ "Vessel", "载具" },
		{ "Engines", "引擎" },
		{ "RCS Rotation", "RCS 旋转" },
		{ "RCS Translation", "RCS 平移" },
		{ "Nav mode", "导航模式" },
		{ "Set focus", "设置焦点" },
		{ "Killrot", "消除自转" },
		{ "Prograde", "顺行" },
		{ "Normal", "法向" },
		{ "HLevel", "水平" },
		{ "Retrograde", "逆行" },
		{ "Anti Normal", "反法向" },
		{ "Retro", "后向" },
		{ "Main", "主引擎" },
		{ "Hover", "悬停" },
		{ "RCS level", "RCS 等级" },
		// --- D3D9 Graphics Controls ---
				{ "Post Processing Configuration", "后期处理配置" },
						{ "Glow threshold", "光晕阈值" },
		{ "Graphics Controls", "图形控制" },
				{ "Diffuse", "漫反射" },
		{ "Mesh: %s", "网格: %s" },
		{ "Emission", "自发光" },
		{ "Smoothness", "平滑度" },
		{ "Emissive2", "自发光2" },
		{ "Metalness", "金属度" },
		{ "SpecialFX", "特殊效果" },
		{ "Selected Visual", "所选视觉体" },
		{ "Selected Mesh", "所选网格" },
		{ "Selected Group", "所选组" },
		{ "Wheel Fly/Pan Cam", "滚轮飞行/平移相机" },
		{ "PBR (Old)", "PBR（旧）" },
		{ "None", "无" },
		{ "Normals Global", "法线全局" },
		{ "Normals Tangent", "法线切线" },
		{ "Height", "高度" },
		{ "Height Mk2", "高度 Mk2" },
		{ "Tile Level", "瓦片层级" },
		{ "Convert to DXT5", "转换为 DXT5" },
		{ "Convert to RGB8", "转换为 RGB8" },
		{ "Convert to RGB4", "转换为 RGB4" },
		{ "Save", "保存" },
		{ "Assign to slot 0", "分配到槽 0" },
		{ "Assign to slot 1", "分配到槽 1" },
		{ "Assign to slot 2", "分配到槽 2" },
		{ "Mirror", "镜面" },
		{ "Blur 1", "模糊 1" },
		{ "Blur 2", "模糊 2" },
		{ "Blur 3", "模糊 3" },
		{ "Blur 4", "模糊 4" },
		{ "Irrad.Probe", "辐照探针" },
		{ "IrdPreItg", "预积分辐照" },
		{ "ShadowMap", "阴影贴图" },
		{ "Irradiance", "辐照度" },
		{ "GlowMask", "光晕遮罩" },
		{ "ScreenDepth", "屏幕深度" },
		{ "Normals", "法线" },
		{ "LightVisbil.", "光照可见性" },
		{ "EclipseTbl", "日食表" },
		{ "Gamma", "伽马" },
		{ "Everything", "全部" },
		{ "Center on visual", "以视觉体为中心" },
		{ "Metalness PBR", "金属度 PBR" },
		{ "Opens a test dialog box which doesn't do much.", "打开一个测试对话框，主要用于演示。" },
		{ "Visual: ", "视觉体: " },
				{ "Material %u:", "材质 %u:" },
		{ "Material %u: [Skin %s]", "材质 %u: [皮肤 %s]" },
		{ "Texture: None", "纹理：无" },
		{ "Texture: %s [%u]", "纹理: %s [%u]" },
		{ "Visual: %s", "视觉体: %s" },
		{ "Mesh: None", "网格：无" },
		{ "Texture: ", "纹理: " },
		{ "Mesh: ", "网格: " },
		{ "Group Status:", "组状态：" },
		{ "Light glow intensity", "光晕强度" },
		{ "Light glow distance", "光晕距离" },
		{ "Light Configuration", "光照配置" },
		{ "Sunlight Intensity", "阳光强度" },
		{ "Indirect Lighting", "间接光照" },
		{ "Local Lights Max", "局部光源上限" },
		{ "Sun Glare Intensity", "太阳眩光强度" },
		{ "Recrete Sun/Glares", "重建太阳/眩光" },
		{ "Collider Visualization", "碰撞体可视化" },
		{ "Physics Colliders", "物理碰撞体" },
		{ "Show Base Colliders", "显示基地碰撞体" },
		{ "Show Vessel Colliders", "显示载具碰撞体" },
		// --- D3D9 Atmospheric Controls ---
		{ "D3D9 Atmospheric Controls", "D3D9 大气控制" },
		{ "This dialog allows to control various atmospheric parameters and effects", "此对话框用于控制各种大气参数与效果" },
		{ "Atmospheric Controls", "大气控制" },
		{ "Water and Sun-glare", "水面与眩光" },
		{ "Auto", "自动" },
		{ "Surface", "表面" },
		{ "Low Orbit", "低轨道" },
		{ "High Orbit", "高轨道" },
		{ "Twilight and Ambient settings", "暮光与环境设置" },
		{ "Wavelenght and Scale height settings", "波长与标高设置" },
		{ "Rayleigh settings", "瑞利设置" },
		{ "Mie settings", "米氏设置" },
		{ "Custom settings", "自定义设置" },
		{ "Water Rendering", "水面渲染" },
		{ "Distance", "距离" },
		{ "Terrain", "地形" },
		{ "Building", "建筑" },
		{ "Green", "绿色" },
		{ "R-Pow", "R 幂" },
		{ "M-Pow", "M 幂" },
		{ "R-Height", "R 高度" },
		{ "M-Height", "M 高度" },
		{ "Brightness", "亮度" },
		{ "Gamma", "伽马" },
		{ "Light/Shad", "光照/阴影" },
		{ "Density", "密度" },
		{ "Ratio", "比例" },
		{ "Phase-A", "相位 A" },
		{ "Phase-B", "相位 B" },
		{ "CloudAlt", "云高" },
		{ "HDR", "HDR" },
		{ "Intensity", "强度" },
		{ "Normals", "法线" },
		{ "Spec", "高光" },
		{ "Color", "颜色" },
		{ "Light travel distance behind terminator", "晨昏线后的光线传播距离" },
		{ "Green wave lenght. (Green balance)", "绿色波长（绿色平衡）" },
		{ "Terrain brightness during twilight", "暮光期间的地形亮度" },
		{ "Main control for atmospheric rayleigh color composition (4.0 for the Earth)", "大气瑞利颜色组成主控（地球为 4.0）" },
		{ "Main control for atmospheric mie color composition", "大气米氏颜色组成主控" },
		{ "Atmosphere Ray scale height (7km - 9km for the Earth)", "大气瑞利标高（地球 7-9km）" },
		{ "Atmosphere Mie scale height (0.6km - 2km for the Earth)", "大气米氏标高（地球 0.6-2km）" },
		{ "Terrain/Ocean brightness control (default 1.0)", "地形/海洋亮度控制（默认 1.0）" },
		{ "Terrain/Ocean gamma control value (default 1.0)", "地形/海洋伽马控制值（默认 1.0）" },
		{ "Terrain light and shadow boost", "地形光影增强" },
		{ "Overall control for rayleigh scattering (i.e. Haze stickness, atmosphere transparency, optical depth", "瑞利散射总体控制（雾霾浓度、大气透明度、光学厚度）" },
		{ "Rayleigh in-scatter out-scatter ratio (1.0 nominal)", "瑞利内外散射比（标称 1.0）" },
		{ "Building lighting level", "建筑照明等级" },
		{ "Overall scale factor for mie scattering. (Mie-particle density)", "米氏散射总缩放系数（米氏粒子密度）" },
		{ "Directional strength of Henyey-Greenstein phase function", "Henyey-Greenstein 相位函数方向强度" },
		{ "Mie in-scatter out-scatter ratio (1.0 nominal)", "米氏内外散射比（标称 1.0）" },
		{ "Altitude for cloud lighting calculations", "云层光照计算高度" },
		{ "'HDR' Exposure factor", "HDR 曝光系数" },
		{ "Omnidirectional mie scattering scale factor", "全向米氏散射缩放系数" },
		{ "[Dual purpose] Clouds intensity [on surface]. Multiscatter light level [on orbit]", "[双用途] 云层强度[表面]，多次散射光强[轨道]" },
		{ "Water normal map strength", "水面法线贴图强度" },
		{ "Water reflectivity", "水面反射率" },
		{ "Water color / transparency", "水面颜色/透明度" },
		{ "Unused", "未使用" },
		{ "External MFD", "外部 MFD" },
		{ "Flight Data Monitor", "飞行数据监视器" },
		{ "Performance Meter", "性能计量器" },
		{ "Lua console window", "Lua 控制台窗口" },
		{ "Mesh debugger", "网格调试器" },
		{ "Notes", "笔记" },
		{ "Remote Vessel Control", "远程载具控制" },
		{ "Scenario Editor", "场景编辑器" },
		{ "TerrainToolKit", "地形工具包" },
		{ "DX9 External MFD", "DX9 外部 MFD" },
		{ "My dialog", "我的对话框" },
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
	// Case-insensitive keys that map to different translations (e.g. "rad"/"Rad").
	// Strict lookup is still exact; these keys are excluded from the case-insensitive
	// fallback so that e.g. "rad" can never resolve to the translation of "Rad".
	static std::set<std::string> g_caseAmbiguous;

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
			// Build the case-insensitive fallback table. If two entries differ only by
			// letter case but have different translations (e.g. "rad" vs "Rad"), the
			// fallback is ambiguous: mark the key so Tr() uses strict matching only.
			auto itL = g_enToZhLower.find(lower);
			if (itL == g_enToZhLower.end()) {
				g_enToZhLower[lower] = t.zh;
			} else if (itL->second != t.zh) {
				g_caseAmbiguous.insert(lower);
			}
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
		{ "ORBIT_PRJ_ECL",  "Ecliptic" },
		{ "ORBIT_PRJ_SHIP", "Ship" },
		{ "ORBIT_PRJ_TGT",  "Target" },
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
			// Case-insensitive fallback only for keys without case-variant ambiguity
			if (!g_caseAmbiguous.count(lower)) {
				auto it2 = g_enToZhLower.find(lower);
				if (it2 != g_enToZhLower.end()) return it2->second.c_str();
			}
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
