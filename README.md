# ESP32-S3 Smart Watch

基于 ESP32-S3 的智能手表项目，采用工业级开发流程，掌握 FreeRTOS、驱动开发、LVGL、双核调度。

## 硬件平台

- **MCU**: ESP32-S3 (Dual-core Xtensa LX7, 240MHz)
- **LCD**: 1.28寸 240×240 圆屏 GC9A01 (SPI)
- **Touch**: CST816S (I2C)
- **IMU**: QMI8658 6轴惯性传感器 (I2C)

## 项目架构

```
ESP32_S3_Watch/
├── main/                    # 应用入口
├── components/
│   ├── bsp/                 # 板级支持包 (I2C总线、GPIO)
│   ├── lcd/                 # GC9A01 LCD驱动 (SPI)
│   ├── cst816s/             # 触摸驱动 (I2C)
│   ├── qmi8658/             # IMU驱动 (I2C)
│   └── lvgl_port/           # LVGL移植层 (显示+触摸)
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

### 2026.7.7 - v0.1.0 LVGL显示成功

- ✅ 搭建 ESP-IDF 工程架构，BSP/HAL分层
- ✅ I2C Bus-Device-Driver 模型，QMI8658 + CST816S 共享总线
- ✅ QMI8658 6轴IMU驱动 (WHO_AM_I验证、加速度计/陀螺仪数据读取)
- ✅ CST816S 触摸驱动 (I2C中断模式、FreeRTOS Queue数据传递)
- ✅ GC9A01 LCD驱动 (SPI+DMA、Waveshare官方完整初始化序列)
- ✅ LVGL 9.5 移植 (RGB565显示+触摸输入、PARTIAL渲染模式)
- ✅ 双核任务调度 (Core0: IMU/Touch, Core1: LVGL)
- ✅ 修复GC9A01花屏问题 (根因: 初始化序列不完整，对照Waveshare官方例程补全60+条命令)
- ✅ 修复Task Watchdog超时 (lv_task中polling SPI占用CPU，添加vTaskDelay让出时间片)

## 编译与烧录

```bash
idf.py build
idf.py flash monitor
```

## 依赖

- ESP-IDF v5.5.4
- LVGL 9.5.0 (通过 idf_component.yml 管理)