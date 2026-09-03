# 基于 STM32G474 的 Modbus RTU 工业通信系统

双节点（Master / Slave）RS485 主从式工业通信系统，自研 Modbus RTU 协议栈，配套 PC Python 测试工具。

- MCU：STM32G474VET6 ×2（Genbotter 开发板，两块板硬件一致，外设齐全）
- 链路：RS485 半双工总线（板上自动方向电路 / DE 可选，见 `BSP/bsp_board_cfg.h`）
- 工具链：Keil MDK-ARM（ARMCLANG V6.14）+ STM32CubeG4 HAL
- 基座例程：LED 闪烁工程（`Core/`、`Drivers/`、`MDK-ARM/`），已作为本仓库 v0.1 基线

## 系统框图

```
 PC (Python Modbus 测试工具)
            │ USB-UART
   ┌────────▼─────────┐          ┌──────────────────┐
   │ STM32G474 Master │  RS485   │ STM32G474 Slave  │
   │ 轮询/读写/重试    │◄───────►│ 从站协议栈        │
   │ 超时/统计         │   A/B    │ 寄存器/参数/日志  │
   └──────────────────┘          └─┬───┬───┬───┬────┘
                                   LCD 24C02 W25Q128 按键
```

- A 板（Master）：周期轮询、读写、超时重试、通信统计、CLI
- B 板（Slave）：帧解析、CRC、异常响应、寄存器管理、参数掉电保存、日志、LCD 状态显示

## 目录结构

```
Modbus-RTU/
├── Core/                  CubeMX 生成 + 手写板级初始化（main/gpio/it/system）
│   ├── Inc/
│   └── Src/
├── Drivers/
│   ├── CMSIS/             内核与设备头文件
│   ├── STM32G4xx_HAL_Driver/  ST HAL 驱动
│   └── User/              用户驱动（led 等，BSP 自初始化风格）
├── App/                   modbus_app / master_app / slave_app / device_manager / system_manager
├── Protocol/              modbus / modbus_master / modbus_slave / modbus_crc / modbus_register
├── BSP/                   bsp_rs485 / bsp_uart / bsp_lcd / bsp_eeprom / bsp_w25q128 / bsp_led / bsp_key
├── Storage/               config（24C02 参数）/ log（W25Q128 环形日志）
├── Common/                ring_buffer / crc / types / 平台无关公共代码
├── Master/                master_main（A 板角色入口）
├── Slave/                 slave_main（B 板角色入口）
├── Tools/python/          PC Modbus 测试工具
├── docs/                  设计文档（本目录）
├── MDK-ARM/               Keil 工程
└── README.md
```

> 说明：CubeMX 例程未含 `.ioc`，故全部外设初始化采用「BSP 自初始化」模式
> （参照 `Drivers/User/led.c`：驱动内自行开时钟、配 GPIO/HAL），不依赖 CubeMX 重新生成。
> 新源码一律 UTF-8 且注释以英文为主，避免 Keil 中文编码问题；设计文档用中文写在 `docs/`。

## 双板角色编译

两块板硬件完全一致，固件角色由编译期宏区分（`BSP/bsp_board_cfg.h` 或工程 Define）：

| 宏                       | 值     | 用途           |
|--------------------------|--------|----------------|
| `MODBUS_NODE_ROLE`       | MASTER | A 板主站固件   |
| `MODBUS_NODE_ROLE`       | SLAVE  | B 板从站固件   |

`Core/Src/main.c` 完成 HAL/时钟/GPIO 后，将控制权交给角色入口
（`Master/master_main.c` 或 `Slave/slave_main.c`）。

## 版本路线（Git）

| 版本  | 内容                                   | 状态 |
|-------|----------------------------------------|------|
| v0.1  | 工程初始化：例程上移、骨架、设计文档   | ✅   |
| v0.2  | UART/RS485 驱动（USART3 PB10/PB11）    | ✅   |
| v0.3  | Modbus CRC16（含 PC 校验）             | ✅   |
| v0.4  | Slave 0x03 读保持寄存器                | ✅   |
| v0.5  | Slave 0x01 / 0x06 / 0x10               | ✅   |
| v0.6  | 异常响应                               | ✅   |
| v0.7  | Master 实现                            | ✅   |
| v0.8  | 超时 / 重试 / 统计                     | ✅   |
| v0.9  | 24C02 参数掉电保存                     | ✅   |
| v1.0  | LCD 状态显示                           | ⏳   |
| v1.1  | W25Q128 环形日志                       | ⏳   |
| v1.2  | Python 测试工具                        | ⏳   |
| v1.3  | 压力 / 稳定性测试                      | ⏳   |
| v1.4  | DMA + RingBuffer 优化                  | ⏳   |
| v2.0  | 最终整合与验收                         | ⏳   |

详见 `docs/06-版本路线图.md`。

## 快速开始

1. 用 Keil MDK 打开 `MDK-ARM/G474.uvprojx`，确认能编译烧录 LED 例程（基线）。
2. 按 `docs/02-寄存器映射表.md` 阅读寄存器语义。
3. 按 `docs/03-状态机设计.md` 阅读主从协议状态机。
4. 后续版本逐层加入 BSP → Protocol → App，每个版本独立可编译。

## 验收标准（最终目标）

- 10000 帧连续通信无丢帧；CRC 错误可检测
- 从站断电 → Master 超时重试并报通信故障
- 非法功能码/地址/数据 → 标准异常响应
- 参数修改 → 掉电 → 上电恢复；通信错误写入 W25Q128 日志
- LCD 实时显示状态与统计；Python 工具可读写寄存器
