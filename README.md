# STM32H7 ART-Pi 驱动架构文档

## 概述

本项目采用了仿照 Linux 内核设计的驱动框架，实现了 **Bus-Device-Driver** 模型，为 STM32H7 平台提供了一个灵活、可扩展的设备驱动管理系统。

## 核心架构

### 1. 整体架构图

```mermaid
graph TB
    subgraph "初始化流程"
        A[系统启动] --> B[early_init]
        B --> C[bus_type_init]
        B --> D[device_init]
        B --> E[driver_init]
    end

    subgraph "总线子系统"
        C --> F[Virtual Bus]
        C --> G[I2C Bus]
        C --> H[SPI Bus]
    end

    subgraph "设备注册"
        D --> I[扫描 board_device_list 段]
        I --> J[调用设备 init 函数]
        J --> K[设备注册到设备链表]
        K --> L[尝试匹配驱动]
    end

    subgraph "驱动注册"
        E --> M[扫描 device_driver_list 段]
        M --> N[调用驱动 init 函数]
        N --> O[驱动注册到驱动链表]
        O --> P[遍历设备进行匹配]
    end

    L --> Q[Match & Probe]
    P --> Q
    Q --> R[设备-驱动绑定成功]
```

### 2. Bus-Device-Driver 模型

```mermaid
classDiagram
    class bus_type {
        +char* name
        +int (*probe)(device*)
        +int (*remove)(device*)
        +int (*match)(device*, driver*)
        +list_head list
    }

    class device {
        +char name[32]
        +char* init_name
        +list_head list
        +device_driver* driver
        +bus_type* bus
        +void* private_data
        +void (*init)(device*)
    }

    class device_driver {
        +char* name
        +list_head list
        +int (*probe)(device*)
        +int (*remove)(device*)
        +size_t private_data_size
        +void* private_data
        +device_match_table* match_ptr
        +bus_type* bus
        +void (*init)(device_driver*)
    }

    bus_type "1" --> "*" device : manages
    bus_type "1" --> "*" device_driver : manages
    device "*" --> "0..1" device_driver : bound to
    device_driver "*" --> "*" device : matches
```

### 3. 设备匹配流程

```mermaid
sequenceDiagram
    participant Dev as Device
    participant Bus as Bus Type
    participant Drv as Driver
    participant List as Device/Driver List

    Note over Dev,Drv: 设备注册时匹配
    Dev->>List: device_register()
    Dev->>List: 添加到设备链表
    Dev->>Drv: driver_probe(dev)
    Drv->>Drv: 遍历驱动链表
    Drv->>Bus: bus->match(dev, drv)
    Bus->>Bus: 比对 init_name 和 compatible
    alt 匹配成功
        Bus-->>Drv: 返回 1
        Drv->>Bus: bus->probe(dev)
        Bus->>Drv: drv->probe(dev)
        Drv->>Dev: 设备初始化
        Dev->>Dev: dev->driver = drv
    else 匹配失败
        Bus-->>Drv: 返回 0
        Drv->>Drv: 继续遍历
    end

    Note over Dev,Drv: 驱动注册时匹配
    Drv->>List: driver_register()
    Drv->>List: 添加到驱动链表
    Drv->>Dev: device_probe(drv)
    Dev->>Dev: 遍历设备链表
    Dev->>Bus: bus->match(dev, drv)
    Bus->>Drv: 匹配并探测
```

### 4. 子系统架构

#### 4.1 I2C 子系统

```mermaid
graph LR
    subgraph "I2C 架构"
        A[i2c_bus_type] --> B[i2c_adapter]
        A --> C[i2c_driver]
        B --> D[i2c_client]
        C --> D
        B --> E[i2c_algorithm]
        E --> F[master_xfer]
        E --> G[smbus_xfer]

        subgraph "具体实现"
            H[stm32-gpio-i2c] -.implements.-> B
            I[mpu6050 driver] -.implements.-> C
            J[mpu6050 device] -.implements.-> D
        end
    end
```

#### 4.2 SPI 子系统

```mermaid
graph LR
    subgraph "SPI 架构"
        A[spi_bus_type] --> B[spi_master]
        A --> C[spi_driver]
        B --> D[spi_device]
        C --> D
        B --> E[spi_transfer/spi_message]

        subgraph "操作接口"
            F[setup]
            G[transfer]
            H[max_transfer_size]
            B --> F
            B --> G
            B --> H
        end
    end
```

#### 4.3 TTY/UART 子系统

```mermaid
graph LR
    subgraph "TTY 架构"
        A[virtual_bus_type] --> B[tty_device]
        A --> C[tty_driver]
        B --> C
        B --> D[tty_operations]

        D --> E[open]
        D --> F[close]
        D --> G[read]
        D --> H[write]
        D --> I[ioctl]

        subgraph "具体实现"
            J[stm32-uart driver] -.implements.-> C
            K[ttyS3 - UART3] -.implements.-> B
            L[ttyS4 - UART4] -.implements.-> B
        end

        subgraph "工作模式"
            K --> M[TTY_MODE_STREAM]
            L --> N[TTY_MODE_CONSOLE]
        end
    end
```

## 关键特性

### 1. 自动注册机制

使用 GCC 的 `section` 属性实现设备和驱动的自动注册：

```c
// 设备注册宏
#define register_device(__name, __dev)  \
    static struct device *__name##_device \
    __attribute__((used, section("board_device_list"))) = &__dev

// 驱动注册宏
#define register_driver(__name, __drv)  \
    static struct device_driver *__name##_driver \
    __attribute__((used, section("device_driver_list"))) = &__drv

// 总线注册宏
#define register_bus_type(_bus) \
    static struct bus_type __attribute__((used, section("bus_type_list"))) \
    *_##_bus = &_bus
```

### 2. 链表管理

采用 Linux 内核风格的侵入式双向链表：

```mermaid
graph LR
    A[list_head] --> B[prev]
    A --> C[next]
    D[container_of] --> E[通过成员指针获取结构体指针]
```

关键宏：
- `list_add_tail()` - 添加到链表尾部
- `list_for_each_entry()` - 遍历链表
- `container_of()` - 通过成员获取容器结构体

### 3. 设备树风格的匹配表

```c
struct device_match_table {
    const char *compatible;  // 兼容字符串
    uint32_t data;          // 附加数据
};
```

驱动通过 `match_ptr` 指向匹配表，总线负责遍历匹配。

## 初始化流程

```mermaid
flowchart TD
    A[系统启动] --> B[调用 early_init]
    B --> C[bus_type_init]
    C --> C1[扫描 bus_type_list 段]
    C1 --> C2[注册所有总线类型]

    B --> D[device_init]
    D --> D1[扫描 board_device_list 段]
    D1 --> D2[调用设备 init 函数]
    D2 --> D3[设备进行初始化配置]

    B --> E[driver_init]
    E --> E1[扫描 device_driver_list 段]
    E1 --> E2[调用驱动 init 函数]
    E2 --> E3[驱动注册并匹配设备]

    D3 --> F[设备-驱动匹配和探测]
    E3 --> F
    F --> G[系统就绪]
```

## 支持的总线类型

### 1. Virtual Bus（虚拟总线）
- 用于不需要物理总线的设备
- 通过字符串匹配 `compatible` 字段
- 适用于：TTY/UART、GPIO 等

### 2. I2C Bus
- 支持标准 I2C 协议
- 实现了 `i2c_algorithm` 接口
- 支持设备：MPU6050 等传感器

### 3. SPI Bus
- 支持标准 SPI 协议
- 支持消息队列和传输管理
- 可配置片选、速度、模式等

## 实际应用示例

### 板级设备定义（board.c）

```c
// UART 设备
static struct stm32_uart stm32h7_uart4 = {
    .tty = {
        .dev = {
            .init_name = "stm32-uart",
            .name = "ttyS4",
            .init = stm32h7_uart4_init,
        },
        .port_num = 4,
        .mode = TTY_MODE_CONSOLE
    }
};

// I2C 适配器
static struct i2c_adapter stm32_gpio_i2c = {
    .dev = {
        .init_name = "stm32-gpio-i2c",
        .init = stm32_gpio_i2c_preinit,
    }
};

// I2C 设备
static struct mpu6050_device mpu6050 = {
    .dev = {
        .dev = {
            .init = mpu6050_preinit,
            .init_name = "mpu6050-device"
        },
        .adap = &stm32_gpio_i2c,
        .addr = 0x68,
        .name = "mpu6050"
    }
};

// 注册设备
register_device(stm32h7_uart4, stm32h7_uart4.tty.dev);
register_device(stm32_gpio_i2c, stm32_gpio_i2c.dev);
register_device(mpu6050, mpu6050.dev.dev);
```

## 目录结构

```
User/
├── Drivers/
│   ├── Inc/
│   │   ├── bus.h              # 总线类型定义
│   │   ├── common.h           # 通用定义
│   │   ├── init.h             # 初始化宏
│   │   ├── list.h             # 链表实现
│   │   └── device/
│   │       ├── device.h       # 设备结构
│   │       ├── driver.h       # 驱动结构
│   │       ├── i2c/i2c.h     # I2C 子系统
│   │       ├── spi/spi.h     # SPI 子系统
│   │       └── tty/tty.h     # TTY 子系统
│   └── Src/
│       ├── kernel/
│       │   └── kernel.c       # 核心初始化
│       └── drivers/
│           ├── base/          # 基础驱动框架
│           │   ├── bus.c
│           │   ├── device.c
│           │   └── driver.c
│           ├── i2c/           # I2C 驱动实现
│           ├── spi/           # SPI 驱动实现
│           └── tty/           # TTY 驱动实现
├── board.c                    # 板级设备定义
└── App/
    └── shell/                 # Shell 应用
```

## 设计优势

1. **解耦性强**：设备、驱动、总线相互独立
2. **可扩展性好**：添加新设备/驱动无需修改框架代码
3. **自动化管理**：使用 section 属性自动收集和注册
4. **类 Linux 接口**：熟悉 Linux 驱动开发者容易上手
5. **灵活的匹配机制**：支持设备树风格的匹配表
6. **统一的探测流程**：通过总线抽象统一管理

## 与 Linux 内核对比

| 特性 | Linux Kernel | 本项目 |
|------|--------------|--------|
| Bus-Device-Driver 模型 | ✓ | ✓ |
| 设备树 | 完整支持 | 简化版（匹配表） |
| 链表实现 | 侵入式双向链表 | 相同 |
| Kobject/Kref | ✓ | ✗ |
| Sysfs | ✓ | ✗ |
| 热插拔 | ✓ | ✗ |
| 电源管理 | ✓ | ✗ |

## 总结

本项目成功地将 Linux 内核的驱动模型移植到了 STM32H7 裸机/RTOS 环境中，提供了一个轻量级但功能完整的设备驱动管理框架。通过 Bus-Device-Driver 模型，实现了设备和驱动的解耦，使得系统更加模块化和易于维护。
