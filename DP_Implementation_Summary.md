# DisplayPort (DP) 驱动实现梳理

> 仓库：embeddedsw.github.io（Xilinx Embedded Software）
> 整理日期：2026-06-06

---

## 1. 目录结构总览

仓库中与 DisplayPort 相关的模块共 15 个，按功能分为四类：

### 1.1 核心 DP 驱动（版本分级）

| 目录 | 说明 |
|------|------|
| `dp12/` | DisplayPort 1.2 核心驱动 |
| `dp14/` | DisplayPort 1.4 核心驱动 |
| `dp21/` | DisplayPort 2.1 核心驱动 |

### 1.2 TX/RX 子系统（Subsystem）

| 目录 | 说明 |
|------|------|
| `dp12txss/` | DP 1.2 发送子系统 |
| `dp14txss/` | DP 1.4 发送子系统 |
| `dp21txss/` | DP 2.1 发送子系统 |
| `dp12rxss/` | DP 1.2 接收子系统 |
| `dp14rxss/` | DP 1.4 接收子系统 |
| `dp21rxss/` | DP 2.1 接收子系统 |

### 1.3 配套组件

| 目录 | 说明 |
|------|------|
| `dpdma/` | DisplayPort DMA 驱动（视频/图形/音频搬运） |
| `dppsu/` | PS（Processing System）侧 DP 控制器驱动 |
| `dphy/` | MIPI D-PHY 物理层驱动 |

### 1.4 HDCP 内容保护

| 目录 | 说明 |
|------|------|
| `hdcp22_cipher_dp/` | HDCP 2.2 加密引擎（DP 专用） |
| `hdcp22_rx_dp/` | HDCP 2.2 接收侧 |
| `hdcp22_tx_dp/` | HDCP 2.2 发送侧 |

---

## 2. 关键文件说明

### 2.1 核心驱动文件（以 dp12 为例，dp14/dp21 结构相同）

```
dp12/src/
├── xdp.h           # 主头文件：数据结构定义、API 声明
├── xdp.c           # 核心实现：链路训练、主流属性、AUX 通道
├── xdp_hw.h        # 硬件寄存器映射与位域定义
├── xdp_intr.c      # 中断处理（TX/RX 分支）
├── xdp_edid.c      # EDID 读取与解析
├── xdp_mst.c       # MST（多路流传输）拓扑管理
├── xdp_spm.c       # 流策略管理（Stream Policy Maker）
├── xdp_sinit.c     # 静态初始化（配置表查找）
└── xdp_selftest.c  # 自检功能
```

### 2.2 子系统文件（以 dp12rxss 为例）

```
dp12rxss/src/
├── xdprxss.h            # 子系统主头文件
├── xdprxss.c            # 子系统主实现
├── xdprxss_dprx.h/c     # DP RX 核心封装
├── xdprxss_iic.h/c      # I2C-over-AUX 接口
├── xdprxss_hdcp1x.h/c   # HDCP 1.x 集成
└── xdprxss_intr.c       # 子系统中断聚合
```

```
dp12txss/src/
├── xdptxss.h              # 子系统主头文件
├── xdptxss.c              # 子系统主实现
├── xdptxss_dptx.h/c       # DP TX 核心封装
├── xdptxss_vtc.h/c        # 视频时序控制器（VTC）
├── xdptxss_dualsplitter.h/c  # 双流分路器
└── xdptxss_hdcp1x.h/c    # HDCP 1.x 集成
```

### 2.3 DPDMA 文件

```
dpdma/src/
├── xdpdma.h        # DMA 主头文件：描述符、通道、缓冲区结构
├── xdpdma.c        # DMA 通道初始化、启动、停止
└── xdpdma_intr.c   # DMA 中断（VSync、帧完成）
```

---

## 3. 核心数据结构

### 3.1 驱动实例（XDp）

```c
// xdp.h
typedef struct {
    XDp_Config Config;      // 硬件配置（基地址、能力）
    u32 IsReady;            // 初始化标志
    union {
        XDp_Tx TxInstance;  // TX 模式专用数据
        XDp_Rx RxInstance;  // RX 模式专用数据
    };
} XDp;
```

### 3.2 TX 链路配置

```c
typedef struct {
    u8  LaneCount;          // 使用通道数：1/2/4
    u8  LinkRate;           // 链路速率：1.62/2.7/5.4 Gbps
    u8  ScramblerEn;        // 扰码开关
    u8  EnhancedFramingMode;// 增强帧模式
    u8  DownspreadControl;  // 扩频控制
    u8  MaxLaneCount;       // RX 侧最大通道数
    u8  MaxLinkRate;        // RX 侧最大速率
} XDp_TxLinkConfig;
```

### 3.3 主流属性（MSA）

```c
typedef struct {
    u32 Htotal;             // 水平总像素（含消隐）
    u32 Vtotal;             // 垂直总行数（含消隐）
    u32 HStart;             // 有效区水平起始
    u32 VStart;             // 有效区垂直起始
    u32 HClkTotal;          // 水平像素时钟总数
    u32 HSyncPulseWidth;    // 水平同步脉冲宽度
    u32 VSyncPulseWidth;    // 垂直同步脉冲宽度
    u32 HResolution;        // 水平分辨率（有效像素）
    u32 VResolution;        // 垂直分辨率（有效行数）
    u32 PixelClockHz;       // 像素时钟频率（Hz）
    u8  BitsPerColor;       // 每分量色深（8/10/12/16 bit）
    u8  ComponentFormat;    // 色彩格式（RGB / YCbCr422 / YCbCr444）
} XDp_TxMainStreamAttributes;
```

### 3.4 DPDMA 描述符

```c
typedef struct {
    u32 Control;            // 控制字段
    u32 Config;             // 配置字段
    u64 NextDescAddr;       // 下一描述符地址（链式）
    u64 SrcAddr;            // 源帧缓冲地址
    u32 LineSize;           // 每行字节数
    u32 LineCount;          // 行数
    u32 LineStride;         // 行步长
} XDpDma_Descriptor;
```

---

## 4. 支持特性

### 4.1 链路层

- **通道数**：1 / 2 / 4 lane（dppsu 限制为 1/2 lane）
- **链路速率**：RBR(1.62Gbps) / HBR(2.7Gbps) / HBR2(5.4Gbps)；dp14 额外支持 HBR3(8.1Gbps)；dp21 支持 UHBR
- **链路训练**：时钟恢复（CR）→ 通道均衡（CE）两阶段自动训练
- **HPD**：热插拔检测，支持事件与脉冲两类中断
- **AUX 通道**：1 Mbps 双向，支持 DPCD 读写及 I2C-over-AUX

### 4.2 视频

| 特性 | dp12 | dp14 | dp21 |
|------|------|------|------|
| 像素宽度 | 1/2/4 | 1/2/4 | 1/2/4 |
| 色彩空间 | RGB / YCbCr422 / YCbCr444 | 同左 + DSC | 同左 |
| 最大色深 | 16 bit/分量 | 16 bit/分量 | 16 bit/分量 |
| 最大分辨率 | 4K×2K | 8K×4K | 16K |
| MST 流数 | 最多 4 路 | 最多 4 路 | 最多 4 路 |

### 4.3 音频

- 辅助通道（Secondary Channel）音频，最多 2 声道
- 支持音频信息帧（Audio InfoFrame）及扩展包
- RX 侧提供音频中断

### 4.4 内容保护

- **HDCP 1.x**：集成于 TX/RX 子系统内
- **HDCP 2.2**：独立 cipher / TX / RX 模块，需单独初始化

---

## 5. 驱动架构与初始化流程

### 5.1 分层架构

```
应用层 (Application)
    │
    ▼
子系统层 (dp12txss / dp12rxss)
  ├── DP TX/RX 核心封装
  ├── VTC / Dual Splitter（TX）
  └── HDCP 1.x / 2.2 集成
    │
    ▼
核心驱动层 (dp12 / dp14 / dp21)
  ├── 链路训练
  ├── AUX 通道
  ├── EDID 解析
  └── MST 管理
    │
    ▼
硬件抽象层 (xdp_hw.h)
  └── 寄存器读写宏
```

### 5.2 TX 初始化流程

```
1. XDp_LookupConfig(DeviceId)       // 从配置表获取硬件参数
2. XDp_CfgInitialize(&Dp, &Config)  // 初始化驱动实例
3. XDp_TxInitialize(&Dp)            // TX 模式专项初始化
4. 注册中断处理函数
5. XDp_TxHpdIsConnected()           // 检测显示器连接
6. XDp_TxGetEdid()                  // 读取显示器 EDID
7. XDp_TxEstablishLink()            // 链路训练（CR + CE）
8. XDp_TxCfgMsaUseStandardVideoMode() // 配置视频时序
9. XDp_TxSetVideoMode()             // 启动视频传输
```

### 5.3 RX 初始化流程

```
1. XDp_LookupConfig(DeviceId)
2. XDp_CfgInitialize(&Dp, &Config)
3. XDp_RxInitialize(&Dp)            // RX 模式专项初始化（硬件自动训练）
4. 注册中断处理函数
5. 使能中断（视频模式变化、训练完成、音频包）
6. 等待链路建立（中断驱动）
```

### 5.4 MST（多路流传输）流程

```
1. XDp_TxMstEnable()                // 使能 MST 模式
2. XDp_TxDiscoverTopology()         // 发现下游拓扑（分支/叶节点）
3. XDp_TxAllocatePayloadVcIdTable() // 分配虚拟通道 ID 表
4. 为每路流配置独立 MSA
5. XDp_TxSetVideoMode()             // 启动多路流传输
```

---

## 6. 中断体系

### TX 侧中断源

| 中断名 | 触发条件 |
|--------|----------|
| HPD Event | 显示器连接/断开 |
| HPD Pulse | DPCD 状态变化（如链路质量下降） |
| Reply Received | AUX 应答完成 |
| Reply Timeout | AUX 超时 |
| Training Done | 链路训练完成 |

### RX 侧中断源

| 中断名 | 触发条件 |
|--------|----------|
| Video Mode Change | 输入视频时序改变 |
| Training Loss | 链路训练丢失 |
| Training Done | 链路训练完成 |
| Audio Info Packet | 收到音频信息帧 |
| Audio Ext Packet | 收到音频扩展包 |
| MST Message | 收到 MST 控制消息 |
| Power State Change | 显示器电源状态变化 |

---

## 7. DPDMA 工作原理

DPDMA 为 PS 侧 DP 控制器（dppsu）配套使用，支持三类通道：

| 通道类型 | 数量 | 用途 |
|----------|------|------|
| Video Channel | 3 | YCbCr 三分量或 RGB 分平面传输 |
| Graphics Channel | 1 | RGBA 混合图形层 |
| Audio Channel | 2 | 音频 PCM 数据 |

工作流程：
1. 分配并填写 `XDpDma_Descriptor`（源地址、行宽、行数、步长）
2. `XDpDma_SetupChannel()` 提交描述符链
3. `XDpDma_Start()` 启动 DMA 传输
4. VSync 中断触发下一帧描述符切换（双缓冲）

---

## 8. 示例程序分布

各版本驱动均提供以下类别的示例：

| 示例类型 | 文件名模式 | 说明 |
|----------|-----------|------|
| 基础 TX | `xdp_tx_example.c` | 单流 TX 基本驱动 |
| 基础 RX | `xdp_rx_example.c` | 单流 RX 基本驱动 |
| MST | `xdp_mst_example.c` | 多路流传输 |
| 音频 | `xdp_audio_example.c` | 音频收发 |
| HDCP | `xdp_hdcp_example.c` | 内容保护 |
| EDID | `xdp_edid_example.c` | EDID 读取与解析 |
| 调试 | `xdp_debug_example.c` | 寄存器 dump、链路状态 |

目标平台：KC705、KCU105、ZCU102、ZCU104 等 Xilinx 评估板。

---

## 9. 版本差异速查

| 特性 | dp12 | dp14 | dp21 | dppsu |
|------|:----:|:----:|:----:|:-----:|
| 最大速率 | HBR2 (5.4G) | HBR3 (8.1G) | UHBR (20G) | HBR (2.7G) |
| 最大 Lane | 4 | 4 | 4 | 2 |
| DSC 压缩 | — | 支持 | 支持 | — |
| MST | 支持 | 支持 | 支持 | — |
| HDCP 2.2 | 可选 | 可选 | 可选 | — |
| PS 集成 | PL IP | PL IP | PL IP | PS 硬核 |
