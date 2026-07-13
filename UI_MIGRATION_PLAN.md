# Zephyr-Watch UI 迁移计划

> 目标：提取 `zephyr-watch-main/` 的 LVGL UI 层，迁移到 ESP-IDF + LVGL9 工程
> 原则：优先保证 UI 完整性，用占位数据替代业务数据，确保 UI 可独立运行

---

## 第一部分：UI 整体架构图

```
main()
  │
  ├── [Zephyr] enable_watchdog_subsystem()
  ├── [纯C]    create_device_twin_instance(unix_time=0, utc_zone=+2)
  ├── [Zephyr] enable_display_subsystem()
  │
  ├── ★ user_interface_init()                    ← UI 入口
  │     ├── lv_theme_default_init(blue, red, dark=true)
  │     ├── home_screen_init()                   ← 创建 Home
  │     │     ├── create_screen()                ← ui_utils
  │     │     ├── create_column(100%, 100%)      ← ui_utils
  │     │     ├── create_row(100%, 20%) ×2       ← ui_utils
  │     │     ├── render_clock_label()           ← Label Montserrat46 "HH:mm"
  │     │     ├── render_date_label()            ← Label Montserrat18 "YYYY-MM-DD"
  │     │     ├── render_day_label()             ← Label Montserrat18 "SUN"~"SAT"
  │     │     └── lv_obj_add_event_cb(GESTURE)   ← 向上滑→Menu
  │     ├── lv_disp_load_scr(home_screen)
  │     ├── [Zephyr] k_work_queue_start()
  │     └── [Zephyr] k_timer_start(2s首次, 10s周期)
  │
  ├── [Zephyr] enable_datetime_subsystem()
  │     └── [纯算法] unix_to_localtime()         ← 可提取
  │
  └── [Zephyr] enable_bluetooth_subsystem()
        └── BLE CTS 写入回调
              └── trigger_ui_update()
                    └── home_screen_set_clock(h, m)

主循环:
  while(1) {
      user_interface_task_handler()  ← lv_task_handler()
      [Zephyr] k_sleep(20ms)
      [Zephyr] kick_watchdog()
  }
```

### UI 目录结构

```
zephyr-watch-main/src/
│
├── main.c                              ← 入口（Zephyr 重度依赖）
│
├── userinterface/                       ★ UI 核心目录
│   ├── userinterface.h                  ← UI 入口接口
│   ├── userinterface.c                  ← UI 初始化 + 时钟更新调度
│   ├── utils.h                          ← 屏幕/行/列创建工具接口
│   ├── utils.c                          ← 屏幕/行/列创建工具实现
│   │
│   ├── styles/
│   │   ├── widgetstyle.h               ← 共享样式接口
│   │   └── widgetstyle.c               ← 无边框样式实现
│   │
│   └── screens/
│       ├── home/
│       │   ├── home.h                  ← 首页接口
│       │   └── home.c                  ← 首页实现（时钟/日期/星期）
│       │
│       ├── menu/
│       │   ├── menu.h                  ← 菜单接口
│       │   └── menu.c                  ← 菜单实现（应用列表+注册系统）
│       │
│       └── blepairing/
│           ├── blepairing.h            ← BLE配对接口
│           └── blepairing.c            ← BLE配对实现（6位PIN显示）
│
├── devicetwin/                          ← 全局状态单例（纯C，可迁移）
│   ├── devicetwin.h
│   └── devicetwin.c
│
├── datetime/                            ← 时间算法（部分可迁移）
│   ├── datetime.h
│   └── datetime.c
│
├── display/                             ← Zephyr Display驱动（不迁移）
│   ├── display.h
│   └── display.c
│
├── bluetooth/                           ← Zephyr BLE（不迁移）
│   ├── infrastructure.h
│   ├── infrastructure.c
│   └── services/
│       ├── current_time_service.h
│       └── current_time_service.c
│
└── watchdog/                            ← Zephyr WDT（不迁移）
    ├── watchdog.h
    └── watchdog.c
```

---

## 第二部分：Screen 列表

### Screen A：Home（首页）

| 属性 | 值 |
|------|-----|
| 文件 | `screens/home/home.c` + `home.h` |
| 创建函数 | `home_screen_init()` |
| 全局对象 | `lv_obj_t *home_screen` |
| 生命周期 | 永不删除，启动时创建 |
| 是否独立 | 是（仅依赖 utils + menu.h 的 screen 指针） |
| Zephyr 依赖 | 无（纯 LVGL） |

**控件组成：**
```
home_screen (lv_obj_create(NULL))
  └── main_column (create_column 100%×100%)
        ├── clock_label_row (create_row 100%×20%)
        │     └── label_clock (Montserrat46, "HH:mm")
        └── date_day_row (create_row 100%×20%)
              ├── label_date (Montserrat18, "YYYY-MM-DD")
              └── label_day (Montserrat18, "SUN")
```

**事件：**
- `LV_EVENT_GESTURE` + `LV_DIR_TOP` → 加载 Menu

**数据更新接口：**
- `home_screen_set_clock(hour, minute)` → `lv_label_set_text_fmt()`
- `home_screen_set_date(year, month, day)` → `lv_label_set_text_fmt()`
- `home_screen_set_day(day_no)` → `lv_label_set_text()`

**已知问题：** 三个 set 函数末尾误用 `lv_disp_flush_ready()`，必须删除。

---

### Screen B：Menu（菜单）

| 属性 | 值 |
|------|-----|
| 文件 | `screens/menu/menu.c` + `menu.h` |
| 创建函数 | `menu_screen_init()` |
| 全局对象 | `lv_obj_t *menu_screen` |
| 生命周期 | 惰性初始化（首次滑入时创建），不删除 |
| 是否独立 | 是（依赖 utils + home.h 的 screen 指针） |
| Zephyr 依赖 | 仅 `LOG_MODULE_REGISTER` |

**控件组成：**
```
menu_screen (lv_obj_create(NULL))
  └── main_column (create_column 100%×100%)
        ├── title_row (create_row 100%×15%)
        │     └── title_label (Montserrat18, "Menu")
        └── menu_list (create_column 100%×80%, 可滚动)
              ├── btn_settings (Button, "Settings")
              ├── btn_stopwatch (Button, "Stopwatch")
              ├── btn_weather (Button, "Weather")
              └── btn_music (Button, "Music")
```

**注册系统：**
```c
typedef struct {
    lv_obj_t *screen;
    char *name;
    bool is_registered;
} application_t;

int register_application(lv_obj_t *screen, char *name);  // 最多10个App
```

**事件：**
- `title_row` 的 `LV_EVENT_DOUBLE_CLICKED` → 返回 Home
- 每个 Button 的 `LV_EVENT_CLICKED` → 切换到对应 App Screen

---

### Screen C：BLE Pairing（蓝牙配对）

| 属性 | 值 |
|------|-----|
| 文件 | `screens/blepairing/blepairing.c` + `blepairing.h` |
| 创建函数 | `blepairing_screen_init()` |
| 全局对象 | `lv_obj_t *blepairing_screen` |
| 生命周期 | 按需创建，用完删除（auto_del=true） |
| 是否独立 | 是（仅依赖 utils） |
| Zephyr 依赖 | 仅 `LOG_MODULE_REGISTER` |

**控件组成：**
```
blepairing_screen (lv_obj_create(NULL))
  └── main_column (create_column 100%×100%)
        ├── title_row (create_row 100%×15%)
        │     └── label_title (Montserrat18, "Pairing Request")
        ├── instruction_row (create_row 100%×15%)
        │     └── label_instruction (Montserrat14, "Enter PIN on your device.")
        ├── pin_row (create_row 100%×40%)
        │     └── pin_container (flex row)
        │           ├── digit_box[0] (25×45, radius=8) + pin_digits[0]
        │           ├── digit_box[1] + pin_digits[1]
        │           ├── digit_box[2] + pin_digits[2]
        │           ├── digit_box[3] + pin_digits[3]
        │           ├── digit_box[4] + pin_digits[4]
        │           └── digit_box[5] + pin_digits[5]
        └── footer_row (create_row 100%×15%)
              └── label_footer (Montserrat12, "Double tap here to cancel")
```

**事件：**
- `footer_row` 的 `LV_EVENT_DOUBLE_CLICKED` → 返回上一页

**特殊机制：**
- `blepairing_screen_load()` — 保存 `previous_screen = lv_scr_act()`，FADE_IN 加载
- `blepairing_screen_unload()` — FADE_OUT 返回 `previous_screen`，删除自身
- `blepairing_screen_set_pin(pin_code)` — 更新6位PIN显示

---

## 第三部分：Widget 列表

| Widget | 类型 | 所在 Screen | 可复用性 | 说明 |
|--------|------|------------|---------|------|
| `label_clock` | Label | Home | ★★★★★ | Montserrat46 时钟 |
| `label_date` | Label | Home | ★★★★★ | Montserrat18 日期 |
| `label_day` | Label | Home | ★★★★★ | Montserrat18 星期 |
| `title_label` | Label | Menu | ★★★★★ | 页面标题 |
| `btn` ×N | Button | Menu | ★★★★★ | 动态菜单按钮 |
| `label` ×N | Label | Menu | ★★★★★ | 按钮内文字 |
| `label_title` | Label | BLE | ★★★★☆ | 配对标题 |
| `label_instruction` | Label | BLE | ★★★★☆ | 提示文字 |
| `digit_box` ×6 | Obj | BLE | ★★★★☆ | PIN数字框容器 |
| `pin_digits` ×6 | Label | BLE | ★★★★☆ | PIN单个数字 |
| `label_footer` | Label | BLE | ★★★★☆ | 底部提示 |

**可复用 Widget 模式：**

| 模式 | 提取自 | 可用于 |
|------|--------|--------|
| `create_screen()` | utils | 所有页面 |
| `create_column()` | utils | 所有页面的垂直布局 |
| `create_row()` | utils | 所有页面的水平布局 |
| `style_no_border` | widgetstyle | 所有容器 |
| 菜单按钮样式 | menu.c 内联 | 设置页、列表页 |
| PIN数字框样式 | blepairing.c 内联 | 验证码、密码输入 |

---

## 第四部分：Theme

### 主题配置

```c
// userinterface.c
lv_theme_t *theme = lv_theme_default_init(
    display,
    lv_palette_main(LV_PALETTE_BLUE),   // 主色
    lv_palette_main(LV_PALETTE_RED),     // 次色
    true,                                 // 暗色模式
    LV_FONT_DEFAULT                       // 默认字体
);
lv_disp_set_theme(display, theme);
```

### 共享样式

```c
// widgetstyle.c
style_no_border:
  border_width = 0
  bg_opa       = TRANSP
  outline_width = 0
  shadow_width  = 0
  pad_all       = 0
```

### 内联样式统计

| 样式 | 位置 | 属性 |
|------|------|------|
| 按钮样式 | menu.c | radius=10, bg=0x2E2E2E, border=2/0x555555, pressed=0x404040 |
| PIN框样式 | blepairing.c | size=25×45, radius=8, bg=0x404040, border=2/0x555555/50% |
| 时钟Label | home.c | letter_space=5, font=Montserrat46 |
| 日期Label | home.c | font=Montserrat18 |
| 星期Label | home.c | font=Montserrat18 |

### 颜色 Palette

| 颜色 | Hex | 用途 |
|------|-----|------|
| 按钮背景 | `0x2E2E2E` | 菜单项默认 |
| 按钮按下 | `0x404040` | 菜单项按下 |
| 按钮边框 | `0x555555` | 菜单项/PIN框 |
| PIN框背景 | `0x404040` | PIN数字框 |
| 白色文字 | `lv_color_white()` | 所有Label |

### 迁移建议

| 内容 | 迁移方式 |
|------|---------|
| 暗色主题初始化 | 直接复制到 `ui_init()` |
| `style_no_border` | 直接复制到 `ui_style.c` |
| 按钮样式 | 提取为共享样式到 `ui_style.c` |
| PIN框样式 | 提取为共享样式到 `ui_style.c` |
| 颜色定义 | 提取为宏到 `ui_style.h` |

---

## 第五部分：Images

**无任何图片资源。**

该工程没有使用任何 PNG、JPG、BIN、C 数组图片或 `lv_image_dsc_t`。

所有视觉效果完全通过 LVGL 控件 + 样式 + 颜色实现。

迁移时无需处理任何图片资源。

---

## 第六部分：Fonts

| 字体 | 大小 | 来源 | LVGL配置 | 迁移方式 |
|------|------|------|----------|---------|
| Montserrat 46 | 46px | LVGL内置 | `LV_FONT_MONTSERRAT_46 1` | 启用配置即可 |
| Montserrat 18 | 18px | LVGL内置 | `LV_FONT_MONTSERRAT_18 1` | 默认已启用 |
| Montserrat 16 | 16px | LVGL内置 | `LV_FONT_MONTSERRAT_16 1` | 默认已启用 |
| Montserrat 14 | 14px | LVGL内置 | `LV_FONT_MONTSERRAT_14 1` | 默认已启用 |
| Montserrat 12 | 12px | LVGL内置 | `LV_FONT_MONTSERRAT_12 1` | 默认已启用 |

**无自定义字体。** 所有字体都是 LVGL 内置 Montserrat 系列。

**迁移注意：** 需要在 LVGL 配置中启用 `LV_FONT_MONTSERRAT_46`，该字体默认未启用。

---

## 第七部分：Animation

| 动画 | API | 时长 | 位置 | auto_del | 迁移方式 |
|------|-----|------|------|----------|---------|
| Home→Menu | `LV_SCR_LOAD_ANIM_MOVE_TOP` | 300ms | `home_screen_event()` | false | 直接用 |
| Menu→Home | `LV_SCR_LOAD_ANIM_MOVE_BOTTOM` | 300ms | `menu_screen_event()` | false | 直接用 |
| →BLE | `LV_SCR_LOAD_ANIM_FADE_IN` | 300ms | `blepairing_screen_load()` | false | 直接用 |
| BLE→上一页 | `LV_SCR_LOAD_ANIM_FADE_OUT` | 300ms | `blepairing_screen_unload()` | true | 直接用 |

**无自定义动画**（无 `lv_anim_t`、无 Timeline、无 Rotate/Scale/Opacity 动画）。

所有动画都是 `lv_screen_load_anim()` 内置页面切换动画，完全兼容 LVGL9，零修改即可使用。

---

## 第八部分：需要迁移的文件

### 直接复制（零修改或仅替换LOG宏）

| 源文件 | → 目标位置 | 修改量 | 说明 |
|--------|-----------|--------|------|
| `utils.c` | `ui/src/ui_utils.c` | 0行 | 纯LVGL API |
| `utils.h` | `ui/include/ui_utils.h` | 0行 | 纯LVGL API |
| `widgetstyle.c` | `ui/src/ui_style.c` | 0行 | 纯LVGL API |
| `widgetstyle.h` | `ui/include/ui_style.h` | 0行 | 纯LVGL API |
| `devicetwin.c` | `devicetwin/src/devicetwin.c` | 0行 | 纯C单例 |
| `devicetwin.h` | `devicetwin/include/devicetwin.h` | 0行 | 纯C |

### 简单修改（替换Zephyr API + 适配ui_manager）

| 源文件 | → 目标位置 | 修改内容 | 工作量 |
|--------|-----------|---------|--------|
| `home.c` | `ui/src/page_home.c` | ① 删除3处 `lv_disp_flush_ready()` ② 适配 `ui_manager` 的 create/destroy/update 模式 ③ 手势事件改用 `ui_switch()` ④ 替换 `LOG` → `ESP_LOG` | ★★☆ |
| `home.h` | `ui/include/page_home.h` | ① 改为 `page_home_create/destroy/update` 接口 | ★☆☆ |
| `menu.c` | `ui/src/page_menu.c` | ① 替换 `LOG` → `ESP_LOG` ② 适配 `ui_manager` ③ `home_screen` 引用改为通过 `ui_manager` 获取 ④ 保留 `register_application` 机制 | ★★★ |
| `menu.h` | `ui/include/page_menu.h` | ① 改为 `page_menu_create/destroy/update` 接口 | ★☆☆ |
| `blepairing.c` | `ui/src/page_ble_pairing.c` | ① 替换 `LOG` → `ESP_LOG` ② `load/unload` 改为 `create/destroy` ③ `previous_screen` 改为 `ui_manager` 管理 | ★★★ |
| `blepairing.h` | `ui/include/page_ble_pairing.h` | ① 改为 `page_ble_pairing_create/destroy/update` 接口 | ★☆☆ |
| `datetime.c` | `datetime/src/datetime.c` | ① 只保留 `unix_to_localtime()` + `is_leap_year()` + `calc_weekday()` ② 删除全部 Zephyr Counter/ISR 代码 ③ 替换 `LOG` → `ESP_LOG` | ★★☆ |
| `datetime.h` | `datetime/include/datetime.h` | ① 保留 `datetime_t` 结构体和转换函数声明 ② 删除 `enable/disable_datetime_subsystem()` | ★☆☆ |

### 需要重写（Zephyr内核原语→FreeRTOS）

| 源文件 | → 目标位置 | 重写内容 | 工作量 |
|--------|-----------|---------|--------|
| `userinterface.c` | `ui/src/ui_manager.c` | ① `k_work_queue` → FreeRTOS `xTask` ② `k_timer` → FreeRTOS `xTimer` ③ `k_work_submit` → FreeRTOS `xQueue` ④ 整合到现有 `ui_manager` 框架 ⑤ 时钟更新改为 `ui_update()` 中轮询 | ★★★★ |
| `userinterface.h` | `ui/include/ui_manager.h` | ① 适配新接口 | ★★☆ |

### 不能要（完全舍弃）

| 源文件 | 原因 | 替代方案 |
|--------|------|---------|
| `display.c` + `display.h` | Zephyr Display/PWM 驱动 | 我们已有 `lcd.c` + BSP |
| `infrastructure.c` + `infrastructure.h` | Zephyr BLE stack | 后续用 ESP-IDF BLE |
| `current_time_service.c` + `.h` | Zephyr BLE GATT | 后续用 ESP-IDF BLE GATT |
| `watchdog.c` + `watchdog.h` | Zephyr WDT 驱动 | 我们已有 Task Watchdog |
| `main.c` | Zephyr 入口 | 我们已有 `app_main()` |
| `prj.conf` | Zephyr Kconfig | 我们的 `sdkconfig` |
| `esp32.overlay` | Zephyr Device Tree | 我们的 `bsp_board.h` |
| `CMakeLists.txt` | Zephyr 构建系统 | 我们的 ESP-IDF CMake |

---

## 第九部分：预计工作量

| 模块 | 工作量 | 说明 |
|------|--------|------|
| **ui_utils** | ★☆☆☆☆ | 直接复制，5分钟 |
| **ui_style** | ★☆☆☆☆ | 直接复制，5分钟 |
| **devicetwin** | ★☆☆☆☆ | 直接复制，5分钟 |
| **datetime 算法** | ★★☆☆☆ | 删Zephyr代码，保留算法，30分钟 |
| **ui_data 桩函数** | ★★☆☆☆ | 新写占位接口，30分钟 |
| **Home 页面** | ★★☆☆☆ | 适配ui_manager + 删flush_ready，1小时 |
| **Menu 页面** | ★★★☆☆ | register_application适配，1.5小时 |
| **BLE Pairing 页面** | ★★★☆☆ | load/unload改造，1.5小时 |
| **ui_manager 重写** | ★★★★☆ | k_work/k_timer→FreeRTOS，2小时 |
| **LVGL 字体配置** | ★☆☆☆☆ | 启用Montserrat46，10分钟 |
| **整体联调** | ★★★☆☆ | 编译+烧录+验证，1小时 |
| **Theme** | ★☆☆☆☆ | 直接复制初始化代码，10分钟 |
| **Animation** | ★☆☆☆☆ | 已兼容LVGL9，0修改 |
| **Images** | × | 无图片资源 |
| **Drivers** | × | 不迁移 |
| **BLE** | × | 不迁移 |
| **RTC** | × | 不迁移 |
| **Zephyr** | × | 不迁移 |

**总计预估：约 8~10 小时**

---

## 附录A：Screen 切换关系图

```
         向上滑动
    ┌──────────────┐    LV_DIR_TOP      ┌──────────────┐
    │              │ ──────────────────→ │              │
    │  Home Screen │    MOVE_TOP 300ms   │ Menu Screen  │
    │  (时钟/日期)  │ ←────────────────── │  (应用列表)   │
    │              │    MOVE_BOTTOM 300ms │              │
    └──────────────┘    双击标题区        └──────┬───────┘
                                               │
                                               │ 点击菜单项
                                               │ lv_screen_load()
                                               ▼
                                        ┌──────────────┐
                                        │  App Screen  │
                                        │ (暂为NULL)   │
                                        └──────────────┘

    ┌──────────────┐   FADE_IN 300ms   ┌───────────────┐
    │  任意页面     │ ────────────────→ │ BLE Pairing   │
    │              │ ←──────────────── │ (6位PIN显示)   │
    │              │   FADE_OUT 300ms  │               │
    └──────────────┘   双击底部         └───────────────┘
```

## 附录B：Zephyr API → FreeRTOS 替换表

| Zephyr API | FreeRTOS 替代 | 说明 |
|-----------|--------------|------|
| `k_sleep(K_MSEC(n))` | `vTaskDelay(pdMS_TO_TICKS(n))` | 延时 |
| `k_timer_start()` | `xTimerStart()` | 定时器 |
| `K_TIMER_DEFINE()` | `xTimerCreate()` | 创建定时器 |
| `k_work_queue_start()` | `xTaskCreate()` | 工作队列→任务 |
| `K_THREAD_STACK_DEFINE()` | 任务栈参数 | 栈空间 |
| `k_work_init()` | 函数指针 | 工作项初始化 |
| `k_work_submit_to_queue()` | `xQueueSend()` | 提交工作 |
| `k_work_is_pending()` | `xQueueIsQueueFullFromISR()` | 检查状态 |
| `LOG_MODULE_REGISTER()` | `ESP_LOGI/E/W/D` | 日志 |
| `LOG_INF/DBG/ERR()` | `ESP_LOGI/D/E()` | 日志输出 |

## 附录C：LVGL9 兼容性检查

| API | LVGL8 | LVGL9 | 状态 |
|-----|-------|-------|------|
| `lv_obj_create(NULL)` | ✅ | ✅ | 兼容 |
| `lv_label_create()` | ✅ | ✅ | 兼容 |
| `lv_button_create()` | `lv_btn_create` | `lv_button_create` | ✅ 已用LVGL9名 |
| `lv_screen_load_anim()` | `lv_scr_load_anim` | `lv_screen_load_anim` | ✅ 已用LVGL9名 |
| `lv_scr_act()` | ✅ | ✅ | 兼容 |
| `lv_obj_remove_flag()` | `lv_obj_clear_flag` | 两者都支持 | ✅ 兼容 |
| `lv_disp_load_scr()` | ✅ | ❌ 改名 `lv_screen_load()` | ⚠️ 需修改1处 |
| `lv_disp_flush_ready()` | 仅flush_cb | 仅flush_cb | ❌ 误用，需删除3处 |
| `lv_obj_is_valid()` | ✅ | ✅ | 兼容 |
| `lv_indev_get_gesture_dir()` | ✅ | ✅ | 兼容 |
| `lv_theme_default_init()` | ✅ | ✅ | 兼容 |
| `LV_LAYOUT_FLEX` | ✅ | ✅ | 兼容 |
| `LV_FLEX_FLOW_COLUMN/ROW` | ✅ | ✅ | 兼容 |
| `LV_PCT()` | ✅ | ✅ | 兼容 |

**结论：该工程本身就是按 LVGL9 编写的，API 兼容性极好。仅需修改1处改名 + 删除3处误用。**

## 附录D：建议迁移顺序

| 步骤 | 模块 | 风险 | 验证方式 |
|------|------|------|---------|
| 1 | ui_utils + ui_style | ★☆☆☆☆ | 编译通过 |
| 2 | devicetwin + datetime算法 | ★☆☆☆☆ | 编译通过 |
| 3 | ui_data 桩函数 | ★★☆☆☆ | 编译通过 |
| 4 | Home 页面 | ★★☆☆☆ | 屏幕显示时钟+日期 |
| 5 | Menu 页面 | ★★★☆☆ | 滑动切换到菜单 |
| 6 | BLE Pairing 页面 | ★★★☆☆ | 代码触发配对页面 |
| 7 | ui_manager 时钟更新 | ★★★★☆ | 时钟实时走动 |
| 8 | LVGL Montserrat46 配置 | ★☆☆☆☆ | 时钟字体变大 |