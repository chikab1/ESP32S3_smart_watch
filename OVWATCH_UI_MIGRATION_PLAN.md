# OV-Watch UI 移植方案

## 第一阶段：OV-Watch UI 完整分析

---

### ① UI 目录结构

```
OV-Watch/Software/OV_Watch/User/
├── GUI_App/                    ← UI 层全部代码
│   ├── ui.h                    ← UI 入口头文件，字体声明
│   ├── ui.c                    ← ui_init()，主题初始化，Pages_init()
│   ├── ui_helpers.h            ← 工具函数声明
│   ├── ui_helpers.c            ← 工具函数（bar/slider/动画回调）
│   ├── Fonts/                  ← 字体资源（C 数组）
│   │   ├── ui_font_Cuyuan18.c
│   │   ├── ui_font_Cuyuan20.c
│   │   ├── ui_font_Cuyuan24.c
│   │   ├── ui_font_Cuyuan30.c
│   │   ├── ui_font_Cuyuan38.c
│   │   ├── ui_font_Cuyuan48.c
│   │   ├── ui_font_Cuyuan80.c
│   │   ├── ui_font_Cuyuan100.c
│   │   ├── ui_font_iconfont16.c
│   │   ├── ui_font_iconfont24.c
│   │   ├── ui_font_iconfont28.c
│   │   ├── ui_font_iconfont30.c
│   │   ├── ui_font_iconfont32.c
│   │   ├── ui_font_iconfont34.c
│   │   └── ui_font_iconfont45.c
│   ├── IMGs/                   ← 图片资源（C 数组）
│   │   └── ui_img_compass_needle_png.c
│   └── Screens/
│       ├── Inc/                ← 页面头文件
│       │   ├── ui_HomePage.h
│       │   ├── ui_MenuPage.h
│       │   ├── ui_SetPage.h
│       │   ├── ui_AboutPage.h
│       │   ├── ui_HRPage.h
│       │   ├── ui_SPO2Page.h
│       │   ├── ui_EnvPage.h
│       │   ├── ui_CompassPage.h
│       │   ├── ui_CalendarPage.h
│       │   ├── ui_ComputerPage.h
│       │   ├── ui_TimerPage.h
│       │   ├── ui_NFCCardPage.h
│       │   ├── ui_GameSelectPage.h
│       │   ├── ui_Game2048Page.h
│       │   ├── ui_GameMemPage.h
│       │   ├── ui_ChargPage.h
│       │   ├── ui_OffTimePage.h
│       │   ├── ui_DateTimeSetPage.h
│       │   └── ui_Megboxes.h
│       └── Src/                ← 页面实现
│           ├── ui_HomePage.c
│           ├── ui_MenuPage.c
│           ├── ui_SetPage.c
│           ├── ui_AboutPage.c
│           ├── ui_HRPage.c
│           ├── ui_SPO2Page.c
│           ├── ui_EnvPage.c
│           ├── ui_CompassPage.c
│           ├── ui_CalendarPage.c
│           ├── ui_ComputerPage.c
│           ├── ui_TimerPage.c
│           ├── ui_NFCCardPage.c
│           ├── ui_GameSelectPage.c
│           ├── ui_Game2048Page.c
│           ├── ui_GameMemPage.c
│           ├── ui_ChargPage.c
│           ├── ui_OffTimePage.c
│           ├── ui_DateTimeSetPage.c
│           └── ui_Megboxes.c
├── Func/                       ← 功能层
│   ├── Inc/
│   │   ├── PageManager.h       ← 页面栈管理器
│   │   ├── HWDataAccess.h      ← 硬件数据抽象层
│   │   ├── StrCalculate.h      ← 字符串计算器
│   │   └── pubsub.h            ← 发布订阅
│   └── Src/
│       ├── PageManager.c       ← 页面栈实现
│       ├── HWDataAccess.c      ← 硬件数据实现
│       ├── StrCalculate.c
│       └── pubsub.c
└── version.h                   ← 版本信息
```

**各目录职责：**

| 目录 | 职责 |
|------|------|
| `GUI_App/` | UI 层入口、字体声明、工具函数 |
| `GUI_App/Fonts/` | Cuyuan 中文字体 + iconfont 图标字体，C 数组形式 |
| `GUI_App/IMGs/` | 图片资源，C 数组形式（仅 compass_needle） |
| `GUI_App/Screens/Inc/` | 页面头文件，声明 Page_t、extern 变量、init/deinit |
| `GUI_App/Screens/Src/` | 页面实现，包含 UI 创建、事件回调、定时器 |
| `Func/Inc/PageManager.h` | 页面栈核心：Page_t 结构、Page_Load/Page_Back |
| `Func/Inc/HWDataAccess.h` | 硬件抽象层：RTC/BLE/Power/IMU/AHT21/SPL06/LSM303/EM7028 |

---

### ② UI 入口调用链

```
main()
  → MX_FREERTOS_Init()
    → xTaskCreate(vMainTask, ...)
      → user_TasksInit()           // 初始化所有 FreeRTOS 任务
        → ui_init()                // ★ UI 入口
          │
          ├─ lv_theme_default_init(dispp, ..., true, LV_FONT_DEFAULT)
          │   // true = 深色主题
          │
          ├─ Pages_init()          // ★ 页面管理器初始化
          │   ├─ page_stack_init(&PageStack)
          │   ├─ page_stack_push(&PageStack, &Page_Home)
          │   ├─ Page_Home.init()  // = ui_HomePage_screen_init()
          │   │   ├─ lv_obj_create(NULL) → ui_HomePage
          │   │   ├─ 创建时间、电池、步数、温湿度、心率等控件
          │   │   ├─ 注册手势事件 (LV_EVENT_GESTURE → Page_Load(&Page_Menu))
          │   │   └─ 创建定时器 (500ms 刷新)
          │   │
          │   └─ lv_disp_load_scr(ui_HomePage)  // 加载首页（无动画）
          │
          └─ lv_timer_create(main_timer, 1000, NULL)
              // 主定时器（当前为空）
```

**流程图：**

```
┌─────────┐     ┌─────────┐     ┌──────────────┐     ┌───────────────────┐
│  main() │────▶│ ui_init │────▶│ Pages_init() │────▶│ Page_Home.init()  │
└─────────┘     └────┬────┘     └──────┬───────┘     └────────┬──────────┘
                     │                  │                      │
                     ▼                  ▼                      ▼
              ┌─────────────┐   ┌──────────────┐     ┌───────────────────┐
              │ Theme Init  │   │ Stack Init   │     │ Create Screen     │
              │ Dark Mode   │   │ Push Home    │     │ Create Widgets    │
              └─────────────┘   └──────────────┘     │ Register Events   │
                                                       │ Create Timer      │
                                                       └───────────────────┘
```

---

### ③ 页面组织方式

OV-Watch 所有页面遵循统一的 `Page_t` 模式：

```c
typedef struct {
    void (*init)(void);       // 创建 screen + 所有 UI 控件
    void (*deinit)(void);     // 删除定时器、清理资源
    lv_obj_t **page_obj;      // 指向 screen 对象的指针
} Page_t;
```

**所有页面列表：**

| # | 页面 | Page_t 变量 | init() | deinit() | 定时器 | 导航方式 |
|---|------|-------------|--------|----------|--------|----------|
| 1 | **HomePage** | Page_Home | ✓ 创建时间/电池/步数/温湿度/心率 | ✓ 删除 HomePageTimer | 500ms | 手势右滑→Menu |
| 2 | **MenuPage** | Page_Menu | ✓ 创建可滚动菜单列表 | ✓ (空) | 无 | 手势右滑→Back |
| 3 | **SetPage** | Page_Set | ✓ 创建设置项列表 | ✓ (空) | 无 | 手势右滑→Back |
| 4 | **AboutPage** | Page_About | ✓ 创建设备信息列表 | ✓ (空) | 无 | 手势右滑→Back |
| 5 | **HRPage** | Page_HR | ✓ 创建心率显示 | ✓ 删除 HRPageTimer | 50ms | 手势右滑→Back |
| 6 | **SPO2Page** | Page_SPO2 | ✓ 创建血氧显示 | ✓ 删除 SPO2PageTimer | 500ms | 手势右滑→Back |
| 7 | **EnvPage** | Page_Env | ✓ 创建温湿度柱状图 | ✓ 删除 EnvPageTimer | 500ms | 手势右滑→Back |
| 8 | **CompassPage** | Page_Compass | ✓ 创建指南针表盘 | ✓ 删除 EcompassPageTimer | 500ms | 手势右滑→Back |
| 9 | **CalendarPage** | Page_Calender | ✓ 创建日历控件 | ✓ (空) | 无 | 手势右滑→Back |
| 10 | **ComputerPage** | Page_Computer | ✓ 创建计算器 | ✓ (空) | 无 | 手势右滑→Back |
| 11 | **TimerPage** | Page_Timer | ✓ 创建秒表 | ✓ (空) | 10ms | 手势右滑→Back |
| 12 | **NFCCardPage** | Page_NFCCard | ✓ 创建卡包列表 | ✓ (空) | 无 | 手势右滑→Back |
| 13 | **GameSelectPage** | Page_GameSelect | ✓ 创建游戏选择列表 | ✓ (空) | 无 | 手势右滑→Back |
| 14 | **Game2048Page** | Page_Game_2048 | ✓ | ✓ | 无 | 手势右滑→Back |
| 15 | **GameMemPage** | Page_GameMem | ✓ | ✓ | 无 | 手势右滑→Back |
| 16 | **ChargPage** | Page_Charg | ✓ 创建充电界面 | ✓ | 定时器 | 充电时自动进入 |
| 17 | **PowerPage** | Page_Power | ✓ 创建关机滑块 | ✓ (空) | 无 | HomePage下拉→Power |
| 18 | **OffTimeSetPage** | Page_LOffTimeSet / Page_TOffTimeSet | ✓ 创建 Roller 选择 | ✓ (空) | 无 | SetPage→子页面 |
| 19 | **DateTimeSetPage** | Page_DateTimeSet / Page_DateSet / Page_TimeSet | ✓ 创建日期时间设置 | ✓ (空) | 无 | SetPage→子页面 |

**注意：** Megboxes 不是页面，是工具函数，在当前屏幕上创建消息框覆盖层。

---

### ④ 页面切换

**所有页面切换调用点：**

#### Page_Load() 调用（前进导航）

| 调用位置 | 目标页面 | 触发方式 |
|----------|----------|----------|
| `ui_HomePage.c` → `ui_event_HomePage()` | Page_Menu | 手势右滑 (LV_DIR_RIGHT) |
| `ui_HomePage.c` → `ui_event_PowerButton()` | Page_Power | 点击电源按钮 |
| `ui_HomePage.c` → `ui_event_SetButton()` | Page_Set | 点击设置按钮 |
| `ui_MenuPage.c` → `ui_event_MenuCalPanel()` | Page_Calender | 点击日历面板 |
| `ui_MenuPage.c` → `ui_event_MenuComPanel()` | Page_Computer | 点击计算器面板 |
| `ui_MenuPage.c` → `ui_event_MenuTimPanel()` | Page_Timer | 点击秒表面板 |
| `ui_MenuPage.c` → `ui_event_MenuCardPanel()` | Page_NFCCard | 点击卡包面板 |
| `ui_MenuPage.c` → `ui_event_MenuHRPanel()` | Page_HR | 点击心率面板 |
| `ui_MenuPage.c` → `ui_event_MenuO2Panel()` | Page_SPO2 | 点击血氧面板 |
| `ui_MenuPage.c` → `ui_event_MenuEnvPanel()` | Page_Env | 点击环境面板 |
| `ui_MenuPage.c` → `ui_event_MenuCPPanel()` | Page_Compass | 点击指南针面板 |
| `ui_MenuPage.c` → `ui_event_MenuGamePanel()` | Page_GameSelect | 点击游戏面板 |
| `ui_MenuPage.c` → `ui_event_MenuSetPanel()` | Page_Set | 点击设置面板 |
| `ui_MenuPage.c` → `ui_event_MenuAbPanel()` | Page_About | 点击关于面板 |
| `ui_SetPage.c` → `ui_event_LightTimePanel()` | Page_LOffTimeSet | 点击常亮时间 |
| `ui_SetPage.c` → `ui_event_TOffTimePanel()` | Page_TOffTimeSet | 点击熄屏时间 |
| `ui_SetPage.c` → `ui_event_DateTimeSetPanel()` | Page_DateTimeSet | 点击时间日期 |
| `ui_GameSelectPage.c` → `ui_event_Game2048Panel()` | Page_Game_2048 | 点击2048 |
| `ui_GameSelectPage.c` → `ui_event_GameMemPanel()` | Page_GameMem | 点击记忆方块 |

#### Page_Back() 调用（返回导航）

| 调用位置 | 触发方式 |
|----------|----------|
| `ui_MenuPage.c` → `ui_event_MenuPage()` | 手势右滑 |
| `ui_SetPage.c` → `ui_event_SetPage()` | 手势右滑 |
| `ui_GameSelectPage.c` → `ui_event_GameSelectPage()` | 手势右滑 |

#### Page_Back_Bottom() 调用

当前代码中未使用。

#### 动画参数

所有页面切换使用统一动画：
```c
lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
// 100ms 滑动动画，自动删除旧 screen
```

---

### ⑤ UI 架构

**属于 B：进入才创建（Create on demand）**

理由：

1. **每次进入页面才调用 init()**：`Page_Load()` 先 deinit 当前页面，再 init 新页面
2. **任意时刻只有一个 screen 存在**：旧 screen 在动画完成后自动删除（`auto_del=true`）
3. **页面栈只保存 Page_t 指针**：不保存 UI 对象，只保存函数指针
4. **init() 每次都从零创建**：`lv_obj_create(NULL)` → 创建所有子控件 → 注册事件
5. **deinit() 负责清理**：删除定时器、释放资源（screen 本身由 LVGL 动画系统删除）

**不是 A（所有 Screen 同时创建）**：因为 deinit 会销毁旧页面
**不是 C（对象池）**：因为每次 init 都重新创建，没有复用

**关键设计优势：**
- 内存占用极低（只有一个 screen）
- 页面间无耦合
- 适合嵌入式设备（RAM 有限）

---

### ⑥ Widget 统计

| Widget | 使用 | 使用位置 |
|--------|------|----------|
| **Label** | ✓ 大量 | 所有页面：时间、日期、数值、图标文字 |
| **Button** | ✓ | Menu 面板内的图标按钮、下拉面板按钮 |
| **Arc** | ✓ | HomePage 电池/温度/湿度弧形指示器 |
| **Bar** | ✓ | HomePage 步数进度条、EnvPage 温湿度柱状图 |
| **Slider** | ✓ | HomePage 亮度滑块、PowerPage 关机滑块 |
| **Switch** | ✓ | SetPage 抬腕亮屏开关、HomePage NFC/BLE 开关 |
| **Calendar** | ✓ | CalendarPage 日历控件 |
| **BtnMatrix** | ✓ | ComputerPage 计算器按键矩阵 |
| **TextArea** | ✓ | ComputerPage 计算器显示区 |
| **Meter** | ✓ | CompassPage 指南针表盘、TimerPage 秒表表盘 |
| **Image** | ✓ | CompassPage 指南针指针图片 |
| **Roller** | ✓ | OffTimeSetPage/DateTimeSetPage 时间选择 |
| **Container (lv_obj)** | ✓ | 所有面板布局（MenuPanel、SetPanel 等） |

**未使用：** Canvas、Chart、List、Grid、Flex（OV-Watch 全部使用绝对定位）

---

### ⑦ Theme 分析

**主题初始化：**
```c
lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE),
                      lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
// true = 深色模式
```

**颜色体系（硬编码在各页面中，无集中定义）：**

| 用途 | 颜色 | 说明 |
|------|------|------|
| 电池 | `0x19C819` | 绿色 |
| 日期/星期 | `0xFF461E` | 橙红色 |
| 步数 | `0x3278FF` | 蓝色 |
| 温度 | `0xF5A73A` | 琥珀色 |
| 湿度 | `0x14C8E1` | 青色 |
| 心率 | `0xC80000` | 红色 |
| 血氧 | `0x0080FF` | 蓝色 |
| 关于标题 | `0x1980E1` | 蓝色 |
| 冒号 | `0x808080` | 灰色 |
| 菜单面板按下 | `0x808080` opa=100 | 灰色半透明 |
| 菜单图标 | 各异 | 每个菜单项不同颜色 |

**字体体系：**
- Cuyuan（粗圆）：中文文字，18/20/24/30/38/48/80/100
- iconfont：图标，16/24/28/30/32/34/45
- Montserrat：数字/英文，14/18/20/24

**样式特征：**
- 无阴影
- 面板圆角 radius=0
- 边框隐藏（border_opa=0 / border_width=0）
- 背景透明（bg_opa=0）
- 按下态：bg_opa=100, bg_color=0x808080

---

### ⑧ 图片资源

| 文件 | 格式 | 尺寸 | 用途 |
|------|------|------|------|
| `ui_img_compass_needle_png.c` | RGB565 C 数组 | 23×171 | 指南针指针 |

**组织方式：**
- 图片以 `const uint8_t array[]` 形式嵌入
- 使用 `LV_IMG_DECLARE()` 声明
- 使用 `lv_img_set_src()` 设置

**整个项目只有 1 张图片**，其余图标全部使用 iconfont 字体实现。

---

### ⑨ 字体

| 字体族 | 文件 | 大小(px) | 来源 | 用途 |
|--------|------|----------|------|------|
| **Cuyuan (粗圆)** | ui_font_Cuyuan18.c | 18 | 方正粗圆简体.TTF | 中文小字 |
| | ui_font_Cuyuan20.c | 20 | | 中文标签 |
| | ui_font_Cuyuan24.c | 24 | | 计算器 |
| | ui_font_Cuyuan30.c | 30 | | 环境数据 |
| | ui_font_Cuyuan38.c | 38 | | 秒表数字 |
| | ui_font_Cuyuan48.c | 48 | | 首页时间 |
| | ui_font_Cuyuan80.c | 80 | | 心率/血氧大数字 |
| | ui_font_Cuyuan100.c | 100 | | （备用） |
| **iconfont** | ui_font_iconfont16.c | 16 | iconfont.ttf | 电池图标 |
| | ui_font_iconfont24.c | 24 | | 步数图标 |
| | ui_font_iconfont28.c | 28 | | 湿度/计算器图标 |
| | ui_font_iconfont30.c | 30 | | 日历/运动/温度图标 |
| | ui_font_iconfont32.c | 32 | | 卡包/设置图标 |
| | ui_font_iconfont34.c | 34 | | 秒表/心率/密码图标 |
| | ui_font_iconfont45.c | 45 | | 熄屏/抬腕图标 |
| **Montserrat** | lv_font_montserrat_14 | 14 | LVGL 内置 | 电池百分比 |
| | lv_font_montserrat_18 | 18 | | 步数数值 |
| | lv_font_montserrat_20 | 20 | | 日期/血氧单位 |
| | lv_font_montserrat_24 | 24 | | 退格键 |

**注意：** Cuyuan 和 iconfont 字体文件是 LVGL8 格式，需要转换为 LVGL9 格式。

---

### ⑩ 动画

| 动画类型 | 使用位置 | 参数 |
|----------|----------|------|
| **Screen Transition** | Page_Load() / Page_Back() / Page_Back_Bottom() | `LV_SCR_LOAD_ANIM_MOVE_RIGHT`, 100ms, auto_del=true |
| **Image Rotation** | CompassPage | `lv_img_set_angle()` 由定时器驱动 |
| **Timer-driven Update** | HomePage/HRPage/SPO2Page/EnvPage/CompassPage/TimerPage | `lv_timer_create()` 10~500ms |

**没有使用：** fade、scale、scroll 动画。OV-Watch 的动画非常简洁，只有页面切换滑动。

---

## 第二阶段：迁移方案

---

### 可以 100% 直接迁移

| 类别 | 内容 | 说明 |
|------|------|------|
| **页面栈架构** | PageManager.h/c | 核心架构，仅需 LVGL8→9 API 适配 |
| **页面生命周期** | Page_t {init, deinit, page_obj} | 完美适配，无需修改 |
| **Menu 布局模式** | Panel + IconBtn + Label | 可直接复用布局代码 |
| **Set 布局模式** | Panel + IconBtn + Label + Switch | 可直接复用 |
| **About 布局模式** | 标题 Label + 内容 Label | 可直接复用 |
| **颜色方案** | 各页面的颜色定义 | 直接搬过来 |
| **iconfont 字体** | ui_font_iconfont*.c | 需要重新生成 LVGL9 格式，但字形数据不变 |
| **Cuyuan 字体** | ui_font_Cuyuan*.c | 需要重新生成 LVGL9 格式，但字形数据不变 |
| **图片资源** | ui_img_compass_needle_png.c | 需要转换为 LVGL9 格式 |
| **Megboxes** | ui_Megboxes.c | 消息框工具，与硬件无关 |
| **ui_helpers** | ui_helpers.c | 工具函数，仅需 LVGL8→9 API 适配 |

---

### 需要修改

| 类别 | 原始实现 | 修改内容 | 难度 |
|------|----------|----------|------|
| **PageManager** | `lv_scr_load_anim()` | → `lv_screen_load_anim()` | 低 |
| **PageManager** | `lv_disp_load_scr()` | → `lv_screen_load()` | 低 |
| **Screen 创建** | `lv_obj_create(NULL)` | LVGL9 中相同，无需改 | 无 |
| **手势检测** | `LV_EVENT_GESTURE` + `lv_indev_get_gesture_dir()` | LVGL9 中手势 API 变化，需改用 PRESSED/RELEASED 手动检测 | 中 |
| **Flag 操作** | `lv_obj_clear_flag()` / `lv_obj_add_flag()` | → `lv_obj_remove_flag()` / `lv_obj_add_flag()` | 低 |
| **字体声明** | `LV_FONT_DECLARE()` | → LVGL9 的 `LV_FONT_DECLARE()` 或 extern | 低 |
| **字体格式** | LVGL8 字体 C 数组 | → 需要用 LVGL9 的 font converter 重新生成 | 中 |
| **图片格式** | LVGL8 图片 C 数组 | → 需要转换为 LVGL9 格式 | 中 |
| **Arc API** | `lv_arc_set_bg_angles()` | → LVGL9 API 适配 | 低 |
| **Meter** | `lv_meter_create()` | LVGL9 中仍支持 | 低 |
| **Calendar** | `lv_calendar_header_arrow_create()` | → LVGL9 API 适配 | 低 |
| **HWDataAccess** | `HWInterface.XXX` | → 替换为我们自己的 `ui_data.c` 数据层 | 中 |
| **定时器** | `lv_timer_create()` / `lv_timer_del()` | → LVGL9 中 `lv_timer_create()` / `lv_timer_delete()` | 低 |
| **Menu 滚动位置** | `ui_MenuScrollY` 保存/恢复 | 需要适配 LVGL9 的 scroll API | 低 |
| **BtnMatrix** | `lv_btnmatrix_create()` + 事件 | LVGL9 API 微调 | 低 |

---

### 不能迁移

| 类别 | 内容 | 原因 |
|------|------|------|
| **STM32 HAL** | 所有 `stm32f4xx_hal_*` | 硬件平台不同 |
| **FreeRTOS 任务** | `user_*Task.c` | 我们用 ESP-IDF 的 FreeRTOS |
| **BLE** | `HWInterface.BLE.*` / KT6328 | 硬件不同 |
| **Sensor 驱动** | AHT21/EM7028/MPU6050/LSM303/SPL06 | 硬件不同 |
| **NFC** | `ICcard_Select()` | 硬件不同 |
| **Power** | `HWInterface.Power.Shutdown()` | 硬件不同 |
| **RTC** | `HWInterface.RealTimeClock.*` | 我们用 ESP-IDF SNTP/RTC |
| **LCD 驱动** | `lcd.c` / `lcd_init.c` | 我们有自己的 lcd 组件 |
| **Touch 驱动** | `CST816.c` | 我们有自己的 touch 组件 |
| **IIC 驱动** | `iic_hal.c` | 我们用 ESP-IDF I2C |
| **OTA/Ymodem** | IAP_F411 工程 | 不适用 |
| **Game2048/GameMem** | 游戏逻辑 | 暂不需要，后续可选 |
| **ChargPage** | 充电界面 | 硬件不同，后续可选 |
| **NFCCardPage** | NFC 卡包 | 硬件不同，后续可选 |

---

### 推荐迁移顺序

```
① PageManager（页面栈核心）
   ↓  确保架构正确，这是所有页面的基础
② Style + Font + Resource（样式/字体/资源）
   ↓  准备好所有 UI 元素
③ HomePage（首页）
   ↓  最核心的页面，验证页面栈 + 手势导航
④ MenuPage（菜单页）
   ↓  验证页面切换动画 + 点击导航
⑤ SetPage（设置页）
   ↓  验证子页面导航 + Switch 控件
⑥ AboutPage（关于页）
   ↓  简单的信息展示页
⑦ IMU Page（IMU 页面）
   ↓  替代 OV-Watch 的 HR/Compass，接入自己的传感器
⑧ 后续扩展
   ↓  Timer / Calendar / Environment / Compass 等
```

---

## 第三阶段：移植实施细节

---

### 目标目录结构

```
components/ui/
├── include/
│   ├── ui_manager.h          ← 页面栈管理器（替代旧 ui_manager）
│   ├── ui_style.h            ← 样式定义
│   ├── ui_utils.h            ← 工具函数
│   ├── ui_data.h             ← 数据抽象层（替代 HWDataAccess）
│   ├── ui_resource.h         ← 字体/图片资源声明
│   ├── page_home.h
│   ├── page_menu.h
│   ├── page_settings.h
│   ├── page_about.h
│   ├── page_imu.h
│   └── ... (后续扩展)
├── src/
│   ├── ui_manager.c          ← PageManager 移植
│   ├── ui_style.c            ← 样式实现
│   ├── ui_utils.c            ← 工具函数实现
│   ├── ui_data.c             ← 数据层实现
│   ├── page_home.c           ← OV-Watch HomePage 移植
│   ├── page_menu.c           ← OV-Watch MenuPage 移植
│   ├── page_settings.c       ← OV-Watch SetPage 移植
│   ├── page_about.c          ← OV-Watch AboutPage 移植
│   ├── page_imu.c            ← IMU 页面
│   └── ... (后续扩展)
├── resource/
│   ├── font/                 ← 字体 C 数组
│   │   ├── ui_font_cuyuan20.c
│   │   ├── ui_font_cuyuan48.c
│   │   ├── ui_font_iconfont30.c
│   │   └── ...
│   └── image/                ← 图片 C 数组
│       └── ui_img_compass_needle.c
└── CMakeLists.txt
```

---

### 关键 API 适配表（LVGL8 → LVGL9）

| LVGL 8 | LVGL 9 | 说明 |
|--------|--------|------|
| `lv_scr_load_anim()` | `lv_screen_load_anim()` | 页面切换动画 |
| `lv_disp_load_scr()` | `lv_screen_load()` | 直接加载页面 |
| `lv_obj_clear_flag()` | `lv_obj_remove_flag()` | 清除标志 |
| `lv_scr_act()` | `lv_screen_active()` | 获取当前活动屏幕 |
| `lv_timer_del()` | `lv_timer_delete()` | 删除定时器 |
| `lv_indev_get_gesture_dir()` | 手动实现 | LVGL9 移除了手势方向 API |
| `LV_EVENT_GESTURE` | 手动检测 | LVGL9 移除了手势事件 |
| `lv_obj_get_scroll_y()` | `lv_obj_get_scroll_y()` | 相同 |
| `lv_arc_set_bg_angles()` | `lv_arc_set_bg_angles()` | 相同 |
| `lv_meter_create()` | `lv_meter_create()` | 相同 |
| `lv_calendar_create()` | `lv_calendar_create()` | 相同 |

---

### 手势导航适配方案

OV-Watch 使用 `LV_EVENT_GESTURE` + `lv_indev_get_gesture_dir()`，但 LVGL9 已移除此 API。

**适配方案：** 在 screen 上监听 `LV_EVENT_PRESSED` / `LV_EVENT_RELEASED`，手动计算滑动方向：

```c
static int16_t s_press_y;
static bool s_press_valid;

static void screen_pressed_cb(lv_event_t *e) {
    lv_point_t pt;
    lv_indev_get_point(lv_indev_active(), &pt);
    s_press_y = pt.y;
    s_press_valid = true;
}

static void screen_released_cb(lv_event_t *e) {
    if (!s_press_valid) return;
    lv_point_t pt;
    lv_indev_get_point(lv_indev_active(), &pt);
    int16_t dy = s_press_y - pt.y;
    s_press_valid = false;

    if (dy > 40) {
        // 上滑 → 进入 Menu
        page_manager_push(PAGE_MENU);
    }
}
```

**返回导航同理：** 检测右滑（dx > 40）→ `page_manager_back()`

---

### 数据层适配方案

OV-Watch 使用 `HWInterface.XXX` 访问硬件数据。我们用 `ui_data.c` 抽象层替代：

```c
// ui_data.h - 数据抽象层
typedef struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t month;
    uint8_t day;
    uint8_t weekday;
    uint16_t year;
} ui_datetime_t;

typedef struct {
    uint8_t battery_percent;
    uint16_t step_count;
    int8_t temperature;
    uint8_t humidity;
    uint8_t heart_rate;
} ui_sensor_data_t;

ui_datetime_t ui_get_time(void);
ui_sensor_data_t ui_get_sensor_data(void);
```

当前 `ui_data.c` 返回硬编码数据，后续接入真实传感器时只需修改此文件。

---

### 移植原则重申

1. **禁止修改** `components/lcd`
2. **禁止修改** `components/touch`
3. **禁止修改** `components/lvgl_port`
4. **只允许修改** `components/ui/`
5. **目标是 UI 效果一致**，不是代码一模一样
6. **保留 ESP-IDF + LVGL9 + FreeRTOS 架构**