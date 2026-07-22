# ESP32-S3 Smart Watch

基于 ESP32-S3 的智能手表项目，采用工业级开发流程，掌握 FreeRTOS、驱动开发、LVGL、双核调度、低功耗管理。

## 硬件平台

- **MCU**: ESP32-S3 (Dual-core Xtensa LX7, 240MHz)
- **LCD**: 1.28寸 240×240 圆屏 GC9A01 (SPI)
- **Touch**: CST816S (I2C, 地址 0x15)
- **IMU**: QMI8658 6轴惯性传感器 (I2C, 地址 0x6B)

## 项目架构

```
ESP32_S3_Watch/
├── main/                    # 应用入口
├── components/
│   ├── app/                 # 应用层 (IMU任务、页面逻辑)
│   ├── bsp/                 # 板级支持包 (I2C总线、GPIO)
│   ├── battery/             # 电池管理
│   ├── cst816s/             # 触摸驱动 (I2C)
│   ├── lcd/                 # GC9A01 LCD驱动 (SPI)
│   ├── lvgl_port/           # LVGL移植层 (显示+触摸)
│   ├── qmi8658/             # IMU驱动 (I2C)
│   ├── services/            # 系统服务 (电源管理、WiFi、设置)
│   └── ui/                  # UI页面 (首页、菜单、设置、IMU)
└── ESP32-S3-Touch-LCD-1.28-Demo/  # Waveshare官方参考例程
```

## 双核任务调度

| 核心 | 任务 | 优先级 | 频率 |
|------|------|--------|------|
| Core 0 | imu_task | 3 | 100Hz |
| Core 0 | touch_task | 3 | 50Hz |
| Core 1 | lv_task | 2 | 200Hz |
| Core 1 | lv_tick_task | 1 | 1000Hz |

## 开发进度

### 2026.7.22 - v0.2.0 低功耗闭环 + I2C资源管理

- ✅ Light Sleep 进入与唤醒完整流程
- ✅ GPIO 触摸中断唤醒 (TP_INT, GPIO5)
- ✅ LCD 唤醒恢复 (GC9A01 重新初始化)
- ✅ CST816S 唤醒恢复 (I2C 设备重新注册)
- ✅ QMI8658 唤醒恢复 (I2C 设备重新注册)
- ✅ LVGL 唤醒恢复 (显示+触摸)
- ✅ I2C 总线互斥锁 (防止 IMU/LVGL 并发访问冲突)
- ✅ IMU 任务同步机制 (suspend/resume 防止 I2C 竞争)
- ✅ 多 I2C 设备生命周期管理 (deinit → bus delete → bus create → reinit)
- ✅ 自动息屏 + 自动进入 Light Sleep
- ✅ WiFi 电源管理 (睡眠时 modem sleep，唤醒时恢复)
- ✅ 背光 PWM 调光 (Dim → Off → 唤醒恢复)
- ✅ 页面手势导航 (Home ↔ Menu ↔ Settings ↔ IMU)
- ✅ 内存稳定无泄漏 (heap 保持 127KB+)

### 2026.7.7 - v0.1.0 LVGL显示成功

- ✅ 搭建 ESP-IDF 工程架构，BSP/HAL分层
- ✅ I2C Bus-Device-Driver 模型，QMI8658 + CST816S 共享总线
- ✅ QMI8658 6轴IMU驱动 (WHO_AM_I验证、加速度计/陀螺仪数据读取)
- ✅ CST816S 触摸驱动 (I2C中断模式、FreeRTOS Queue数据传递)
- ✅ GC9A01 LCD驱动 (SPI+DMA、Waveshare官方完整初始化序列)
- ✅ LVGL 9.5 移植 (RGB5655显示+触摸输入、PARTIAL渲染模式)
- ✅ 双核任务调度 (Core0: IMU/Touch, Core1: LVGL)
- ✅ 修复GC9A01花屏问题 (根因: 初始化序列不完整，对照Waveshare官方例程补全60+条命令)
- ✅ 修复Task Watchdog超时 (lv_task中polling SPI占用CPU，添加vTaskDelay让出时间片)

## 技术文档

### I2C 总线架构

```
I2C_NUM_0 (400kHz)
├── CST816S (0x15) - 触摸控制器
└── QMI8658 (0x6B) - 6轴IMU
```

所有 I2C 设备共享同一总线，通过 `bsp_i2c_get_mutex()` 获取互斥锁，防止多任务并发访问导致总线冲突。

### 低功耗唤醒流程

```
Light Sleep
    ↓
GPIO5 (TP_INT) 低电平唤醒
    ↓
1. imu_suspend()          - 暂停IMU任务访问I2C
2. cst816s_deinit()       - 从旧总线移除触摸设备
3. imu_deinit()           - 从旧总线移除IMU设备
4. bsp_i2c_reinit()       - 删除并重建I2C总线
5. cst816s_resume()       - 重新注册触摸设备
6. imu_init_after_wakeup() - 重新注册IMU设备
7. imu_resume()           - 恢复IMU任务
8. lcd_reinit()           - 重新初始化LCD
9. lvgl_port_resume()     - 恢复LVGL
10. 恢复背光 + WiFi
```

### 唤醒日志示例

```
I (52011) power_svc: POWER ENTER LIGHT SLEEP
I (52031) power_svc: POWER WAKEUP
I (52041) cst816s: device removed
I (52041) bsp_i2c: I2C bus reinitialized
I (52261) cst816s: resume OK, chip ID=0xB5
I (52261) power_svc: CST816S RESUME OK
I (52261) imu: reinit OK after wakeup
I (52541) power_svc: LCD RESUME OK
I (52541) lvgl_port: LVGL resumed
I (52551) power_svc: wakeup, screen ON
```

## 编译与烧录

```bash
idf.py build
idf.py flash monitor
```

## 依赖

- ESP-IDF v5.5.4
- LVGL 9.5.0 (通过 idf_component.yml 管理)