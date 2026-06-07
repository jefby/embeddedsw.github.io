# MIPI CSI-2 驱动代码分析

> 仓库：embeddedsw.github.io（Xilinx/AMD Embedded Software）
> 整理日期：2026-06-07

---

## 1. 模块总览

仓库中与 MIPI CSI-2 相关的驱动共 4 个目录，分为「核心 IP 驱动」与「子系统（Subsystem）驱动」两个层次，分别覆盖 RX（接收）与 TX（发送）方向：

| 目录 | 角色 | 说明 |
|------|------|------|
| `csi/` | RX 核心驱动 | MIPI CSI-2 Rx Controller 驱动（`XCsi`），符合 CSI-2 v1.1 + D-PHY v1.2 规范 |
| `csi2tx/` | TX 核心驱动 | MIPI CSI-2 Tx Controller 驱动（`XCsi2Tx`） |
| `mipicsiss/` | RX 子系统驱动 | MIPI CSI Rx Subsystem 驱动（`XCsiSs`），封装 CSI Rx + D-PHY (+ IIC) |
| `csi2txss/` | TX 子系统驱动 | MIPI CSI2 Tx Subsystem 驱动（`XCsi2TxSs`），封装 CSI2 Tx + D-PHY |

整体分层关系：

```
应用层 (Application)
        │
        ▼
子系统驱动 XCsiSs / XCsi2TxSs   ← 对外统一 API，屏蔽子核细节
        │
        ├── CSI / CSI2TX 核心驱动 (XCsi / XCsi2Tx)
        ├── D-PHY 驱动 (XDphy，按需链接，条件编译 XPAR_XDPHY_NUM_INSTANCES)
        └── IIC 驱动 (XIic，仅 RX 子系统用于 CCI 摄像头控制接口)
```

每个目录均遵循 Xilinx 标准驱动结构：

| 文件 | 作用 |
|------|------|
| `src/x*.h` | 公共头文件：类型定义、内联位操作、函数原型 |
| `src/x*_hw.h` | 寄存器偏移、位掩码/位移定义 |
| `src/x*.c` | 主体功能：初始化、配置、复位、激活等 |
| `src/x*_intr.c` | 中断使能/状态/回调注册与中断处理函数 |
| `src/x*_selftest.c` | 自检（读寄存器版本号等） |
| `src/x*_sinit.c` | 根据 DeviceId 查找静态配置表 `XCsi*_LookupConfig` |
| `src/x*_g.c` | 由 BSP 工具生成的静态配置表（`ConfigTable[]`） |
| `data/*.mdd / *.tcl` | 驱动元数据与 TCL 生成脚本（用于 Vitis/EDK 工具链） |
| `examples/` | 自检 / 中断使用范例 |
| `doc/html/api/` | Doxygen 生成的 API 文档（HTML） |

---

## 2. CSI-2 RX 核心驱动 `csi/`（`XCsi`）

文件：`xcsi.h`(701L) / `xcsi.c`(557L) / `xcsi_hw.h`(695L) / `xcsi_intr.c`(350L) / `xcsi_selftest.c` / `xcsi_sinit.c`

### 2.1 功能定位

- 实现 MIPI Alliance **CSI-2 v1.1**（含 D-PHY v1.2）规范的接收控制器驱动；
- 支持 1~4 lane，单 lane 最高 1.5 Gbps；
- 支持长/短包，支持最多 16 个虚拟通道（VC）的图像数据交织；
- 可检测 SoT、CRC、ECC 等多种错误并通过中断上报。

### 2.2 关键数据结构（`xcsi.h`）

- `XCsi_Config`：硬件构建期配置（`DeviceId`、`BaseAddr`、`MaxLanesPresent`、`HasOffloadNonImageSupport`、`HasVCSupport`、`FixedVC`、`FixedLanes`）；
- `XCsi`：驱动实例，内含 `Config`、`ActiveLanes` 及 **7 类中断回调**（短包、帧接收、D-PHY 错误、包级错误、协议解码错误、VCx 错误、其它错误）及对应的 `*Ref` 上下文指针；
- 信息类结构体：`XCsi_SPktData`（短包数据/类型/VC）、`XCsi_VCInfo`（行计数/字节计数/数据类型）、`XCsi_ClkLaneInfo`、`XCsi_DataLaneInfo`（停止状态、SoT 错误、SoT 同步错误、Skew 校准状态）。

### 2.3 寄存器映射（`xcsi_hw.h`）

| 偏移 | 寄存器 | 说明 |
|------|--------|------|
| `0x00` | CCR | 核心配置（软复位 bit1、核心使能 bit0） |
| `0x04` | PCR | 协议配置（最大/激活 lane 数） |
| `0x10` | CSR | 核心状态（包计数、短包 FIFO 满/非空、流缓冲满、复位中标志） |
| `0x20/0x24/0x28` | GIER/ISR/IER | 全局中断使能 / 中断状态 / 中断使能 |
| `0x2C` | VC_SEL | 虚拟通道选择（按位过滤 VC） |
| `0x30` | SPKTR | 通用短包寄存器 |
| `0x34` | VCX_FE | VCx 帧错误寄存器（用于扩展 VC 16 个通道） |
| `0x3C` | CLKINFR | 时钟通道信息（停止状态） |
| `0x40~0x4C` | L0~L3 INFR | 数据 0~3 通道信息（停止状态/SoT 错误/SoT 同步错误/Skew 校准） |
| `0x60~0xDC` | VCx INF1R/INF2R | 16 个虚拟通道的图像信息（行计数、字节计数、数据类型） |

中断状态寄存器 `ISR` 中定义了约 25 个独立错误位，包括帧接收（FR）、VCx 帧错误、Skew 校准错误、YUV420 字计数错误、字计数损坏（WC）、Lane 配置错误（ILC）、短包 FIFO 满/非空、流缓冲满、Stop 状态、SoT/SoT 同步错误、ECC 1bit/2bit 错误、CRC 错误、未知数据 ID 错误、以及每个 VC（0~3）的帧同步/帧级错误。

### 2.4 主要 API（`xcsi.c`）

| 函数 | 功能 |
|------|------|
| `XCsi_CfgInitialize` | 初始化实例：拷贝配置、设置 `BaseAddr`，将所有中断回调置为 `StubErrCallBack`（防止用户未注册回调时跑飞），标记 `IsReady` |
| `XCsi_Reset` | 置软复位位并轮询 `RIPCD`（复位进行中）位直到完成或超时（`XCSI_RESET_TIMEOUT = 10000`） |
| `XCsi_Activate` | 使能/禁用核心：使能时打开全局中断+核心使能；禁用时关闭全局中断+核心禁用并等待复位完成 |
| `XCsi_Configure` | 按 `InstancePtr->ActiveLanes` 配置激活 lane 数：先备份中断使能寄存器→软复位→设置 active lanes→清复位→恢复中断使能→回读校验设置是否生效；若 `FixedLanes` 已设置则直接跳过编程 |
| `XCsi_SetVCSelection` / `XCsi_GetVCSelection` | 设置/读取 VC 过滤选择寄存器 |
| `XCsi_GetShortPacket` | 读取并解析通用短包（数据/VC/数据类型） |
| `XCsi_GetClkLaneInfo` / `XCsi_GetDataLaneInfo` | 读取时钟通道/数据通道（0~3）的链路状态信息 |
| `XCsi_GetVCInfo` | 读取某 VC 的行计数、字节计数、数据类型（自动根据奇偶 VC 号定位寄存器偏移） |
| `XCsi_IsActiveLaneCountValid` | 校验 active lane 取值合法性：必须在 1~4 之间且不超过 `MaxLanesPresent`；当 `FixedLanes=1` 时必须等于 `MaxLanesPresent` |

此外 `xcsi.h` 中提供大量 **内联位操作辅助函数**（`XCsi_BitSet/BitReset/GetBitField/SetBitField`、`XCsi_SetSoftReset/ClearSoftReset/Enable/Disable/IsCsiEnabled`、`XCsi_GetMaxLaneCount/GetActiveLaneCount/SetActiveLaneCount`、`XCsi_GetCurrentPacketCount`、`XCsi_IsShortPacketFIFOFull/NotEmpty`、`XCsi_IsStreamLineBuffFull`、`XCsi_IsSoftResetInProgress`、`XCsi_SetGlobalInterrupt/ResetGlobalInterrupt`），均直接映射到 `xcsi_hw.h` 的寄存器位域。

### 2.5 中断处理（`xcsi_intr.c`）

- `XCsi_IntrEnable/IntrDisable/GetIntrEnable/GetIntrStatus/InterruptClear`：基本的中断屏蔽位读写，均带 `Xil_AssertVoid` 校验掩码合法性（不能超出 `XCSI_IER_ALLINTR_MASK`）；
- `XCsi_SetCallBack`：按 `HandleType`（`XCSI_HANDLER_DPHY/PROTLVL/PKTLVL/SHORTPACKET/FRAMERECVD/VCXERR/OTHERERROR` 共 7 种）注册回调函数与上下文指针；
- `XCsi_IntrHandler`：核心中断服务程序：
  1. 读取 `ISR & IER` 得到当前激活的中断；
  2. 按位掩码分类依次分发到 7 个回调（帧接收、其它错误、短包、D-PHY 错误、VCx 帧错误、协议解码错误、包级错误）；
  3. **VCx 帧错误**特殊处理：单独读取 `XCSI_VCX_FE_OFFSET` 寄存器获取具体的 VC 错误位，回调后再写回以清除；
  4. 最后统一调用 `XCsi_InterruptClear` 清除已处理的中断状态位。

### 2.6 典型使用流程（来自头文件文档与范例）

```
XCsi_LookupConfig(DeviceId)
  → XCsi_CfgInitialize(Instance, Config, BaseAddr)
  → XCsi_Configure()                     // 设置 Active Lanes
  → XCsi_SetCallBack(..., handler, ref)   // 注册各类中断回调
  → XCsi_IntrEnable(Instance, Mask)
  → XCsi_Activate(Instance, XCSI_ENABLE)  // 启动核心接收
```

---

## 3. CSI-2 TX 核心驱动 `csi2tx/`（`XCsi2Tx`）

文件：`xcsi2tx.h`(811L) / `xcsi2tx.c`(421L) / `xcsi2tx_hw.h`(231L) / `xcsi2tx_intr.c`(426L) / `xcsi2tx_selftest.c` / `xcsi2tx_sinit.c`

### 3.1 功能定位

- 通过 Native / AXI4-Stream 接口接收图像数据流，打包为 CSI-2 包结构（同步包 + 像素到字节转换），经由 D-PHY 接口发送；
- 支持 ECC（包头）/ CRC（payload）生成；
- 支持 1~4 lane、最高 1.5 Gbps/lane、单/双/四像素模式；
- 支持多种数据类型：RAW8/10/12/14、RGB888、YUV422-8bit、用户自定义类型；
- 支持 1~4 个虚拟通道、低功耗状态（LPS）插入、超低功耗（ULP）模式；
- 支持 **帧结束（Frame End, FE）生成**特性（v1.1 新增）。

### 3.2 关键数据结构（`xcsi2tx.h`）

- `XCsi2Tx_Config`：含 `DeviceId/BaseAddr/MaxLanesPresent/ActiveLanes/FEGenEnabled`；
- `XCsi2Tx`：实例结构，含 **9 类中断回调**（错误 lane 配置、通用短包 FIFO 满、ULPS、行缓冲满、错误数据类型、像素下溢，以及 VC0~VC3 各自的行计数错误）；
- `XCsi2Tx_LCStatus` 枚举：`XCSI2TX_LC_LESS_LINES` / `XCSI2TX_LC_MORE_LINES`，用于 FE 生成功能下报告某 VC 实收行数偏少/偏多；
- `XCsi2Tx_SPktData`：与 RX 端相同结构的短包描述。

### 3.3 寄存器映射（`xcsi2tx_hw.h`）

| 偏移 | 寄存器 | 说明 |
|------|--------|------|
| `0x00` | CCR | 核心配置：核心使能(bit0)、软复位(bit1)、核心就绪 RIPCD(bit2)、ULPS(bit3)、时钟模式(bit4) |
| `0x04` | PCR | 协议配置：激活 lane(bit0-1)、最大 lane(bit3-4)、像素模式(bit13-14)、行生成模式 LineGen(bit15) |
| `0x20/0x24/0x28` | GIER/ISR/IER | 全局中断/状态/使能 |
| `0x30` | SPKTR | 通用短包寄存器 |
| `0x40~0x4C` | LINE_COUNT_VC0~VC3 | 各 VC 的行计数寄存器（FE 生成特性使用） |
| `0x78` | GSP | GSP 状态（在 FIFO 满前还可安全写入的 GSP 数量） |

中断位定义（`XCSI2TX_IER/ISR_ALLINTR_MASK = 0x3F` 基础 6 位 + 扩展行计数状态位）：
`UNDERRUN_PIXEL`(bit0)、`WRONG_DATATYPE`(bit1)、`LINE_BUFF_FULL`(bit2)、`DPHY_ULPS`(bit3)、`GPSFIFO`(bit4)、`INCORT_LANE`(bit5)；
以及每个 VC 的 **行计数状态** 中断（`XCSITX_LCSTAT_VCx_IER/ISR_MASK`，分别位于 bit8/10/12/14，2 位编码：1=行数偏少，2=行数偏多）。

### 3.4 主要 API（`xcsi2tx.c`）

| 函数 | 功能 |
|------|------|
| `XCsi2Tx_CfgInitialize` | 实例初始化，回调置为桩函数，标记就绪 |
| `XCsi2Tx_Reset` | 软复位并轮询超时（`XCSI2TX_RESET_TIMEOUT`） |
| `XCsi2Tx_Activate` | 使能/禁用核心（与 RX 不同，TX 禁用时**不**轮询等待复位完成） |
| `XCsi2Tx_Configure` | 配置 active lanes：备份中断寄存器→`Reset`→设置 active lanes→恢复中断寄存器→回读校验 |
| `XCsi2Tx_GetShortPacket` | 读取并解析通用短包寄存器 |
| `XCsi2Tx_IsActiveLaneCountValid` | 校验 lane 数合法性（1~4 且不超过 `MaxLanesPresent`） |
| `XCsi2Tx_SetLineCountForVC` / `XCsi2Tx_GetLineCountForVC` | 设置/读取指定 VC 的行计数寄存器（仅 `FEGenEnabled=1` 时可用，否则返回 `XST_NO_FEATURE`） |

同样在头文件中提供了大量内联位域操作（`BitSet/BitReset/GetBitField/SetBitField`、`SetSoftReset/ClearSoftReset/Enable/Disable/IsCsiEnabled`、`GetMaxLaneCount/GetActiveLaneCount/SetActiveLaneCount`、`SetGlobalInterrupt/ResetGlobalInterrupt`，以及 ULPS/时钟模式/行生成/像素模式相关的 Get/Set 内联函数）。

### 3.5 中断处理（`xcsi2tx_intr.c`）

- 与 RX 端类似的 `IntrEnable/IntrDisable/GetIntrEnable/GetIntrStatus/InterruptClear`；
- `XCsi2Tx_SetCallBack`：支持 10 种 `XCSI2TX_HANDLER_*` 类型（错误 lane、GSP FIFO 满、ULPS、行缓冲满、错误数据类型、像素下溢、VC0~VC3 行计数错误）；
- `XCsi2Tx_IntrHandler`：解析 `ISR & IER`，按位掩码分发到对应回调，最后清除已处理的中断状态位；其中 VC0~VC3 的行计数错误会进一步根据 2-bit 编码值判断是 `LC_LESS_LINES` 还是 `LC_MORE_LINES` 并传递给回调。

---

## 4. MIPI CSI Rx 子系统驱动 `mipicsiss/`（`XCsiSs`）

文件：`xcsiss.h`(295L) / `xcsiss.c`(711L) / `xcsiss_hw.h`(156L) / `xcsiss_intr.c`(156L) / `xcsiss_selftest.c` / `xcsiss_sinit.c`

### 4.1 功能定位

子系统驱动是对 **CSI Rx Controller + D-PHY (+ IIC)** 的一层封装，向上层应用提供统一、简化的 API，屏蔽底层子核的差异和复杂的寄存器编程细节。

支持特性：
- 1~4 PPI lane，线速率 80~1500 Mbps；
- 多种数据类型（RAW/RGB/YUV422-8bit）；
- 通过 AXI IIC 支持 CCI（Camera Control Interface）摄像头控制；
- 基于虚拟通道 ID 的包过滤；
- 单/双/四像素输出模式（兼容 UG934 格式）。

GUI（IP Integrator）层面可配置项包括：lane 数、像素格式、虚拟通道、每周期像素数、D-PHY 是否带寄存器接口、线速率、缓冲深度、是否含嵌入式非图像数据、是否集成 IIC 等；这些静态参数生成在 `xcsiss_g.c` 的配置表中。

### 4.2 关键数据结构（`xcsiss.h`）

- `CsiRxSsSubCore`：子核描述（是否存在、DeviceId、相对子系统基址的偏移）；
- `XCsiSs_Config`：子系统级配置，整合了 `IicInfo/CsiInfo/DphyInfo` 三个子核配置以及众多设计期参数（`LanesPresent/PixelCount/PixelFormat/VcNo/CsiBuffDepth/IsEmbNonImgPresent/IsDphyRegIntfcPresent/DphyLineRate/EnableCrc/EnableActiveLanes/EnableCSIv20/EnableVCx` 等）；
- `XCsiSs`：实例结构，持有 `CsiPtr`（必选）、`DphyPtr`（条件编译 `XPAR_XDPHY_NUM_INSTANCES > 0`）、`IicPtr`（条件编译 `XPAR_XIIC_NUM_INSTANCES > 0`）三个子核句柄，以及聚合的状态信息缓存（`ClkInfo/DLInfo[4]/SpktData/VCInfo[16]`）；
- 中断 Handler 类型 **直接复用** RX 核心驱动的宏定义（如 `XCSISS_HANDLER_DPHY = XCSI_HANDLER_DPHY`），体现"紧密耦合、句柄透传"的设计思路。

### 4.3 主要 API（`xcsiss.c`）

| 函数 | 功能 |
|------|------|
| `XCsiSs_CfgInitialize` | 拷贝配置→`CsiSs_GetIncludedSubCores` 探测各子核是否存在→依次初始化 IIC（如有）/CSI（必选）/D-PHY（如有寄存器接口）子核→标记就绪 |
| `XCsiSs_GetIicInstance` | 返回内部 IIC 实例指针，供应用层直接调用 IIC 驱动访问 CCI（条件编译） |
| `XCsiSs_Configure` | 校验 `ActiveLanes` 合法性→使能指定中断掩码（与 `XCSI_ISR_ALLINTR_MASK` 相与）→设置 `CsiPtr->ActiveLanes` 并调用 `XCsi_Configure` |
| `XCsiSs_Activate` | 先激活/禁用 CSI 子核，成功后再联动激活/禁用 D-PHY（如存在寄存器接口） |
| `XCsiSs_Reset` | 仅复位 CSI 子核（D-PHY/IIC 由应用单独处理） |
| `XCsiSs_ReportCoreInfo` | 打印子系统包含的子核信息 |
| `XCsiSs_SetVCSelection` / `GetVCSelection` | 透传到 `XCsi_SetVCSelection/GetVCSelection` |
| `XCsiSs_GetShortPacket` / `GetLaneInfo` / `GetVCInfo` | 调用核心驱动相应函数并缓存结果到自身的 `SpktData/ClkInfo/DLInfo/VCInfo` 字段中 |

### 4.4 中断处理（`xcsiss_intr.c`）

`XCsiSs_IntrHandler` / `XCsiSs_SetCallBack` / `XCsiSs_IntrDisable` 基本是对 `XCsi_IntrHandler` / `XCsi_SetCallBack` / `XCsi_IntrDisable`（核心驱动）的直接转发，子系统层不额外处理逻辑，只是统一了对外暴露的入口名字。

### 4.5 典型使用流程（`examples/xcsiss_intr_example.c`）

```
XCsiSs_LookupConfig(DeviceId)
  → XCsiSs_CfgInitialize(Instance, Config, BaseAddr)
  → 注册各类回调：CsiSs_DphyEventHandler / PktLvlEventHandler /
                  ProtLvlEventHandler / SPktEventHandler /
                  ErrEventHandler / FrameRcvdEventHandler
  → XCsiSs_SetCallBack(...) 逐一安装
  → 配置中断系统（连接到中断控制器 XIntc）
  → 上层完成摄像头初始化 (CCI / IIC) 与流源配置
  → 帧接收中断打印帧计数；短包中断打印短包内容；
    出错中断打印错误信息并复位子系统
```

---

## 5. MIPI CSI2 Tx 子系统驱动 `csi2txss/`（`XCsi2TxSs`）

文件：`xcsi2txss.h`(287L) / `xcsi2txss.c`(732L) / `xcsi2txss_hw.h`(108L) / `xcsi2txss_intr.c`(169L) / `xcsi2txss_selftest.c` / `xcsi2txss_sinit.c`

### 5.1 功能定位

对 **CSI2 Tx Controller + D-PHY** 的封装层，简化上层对发送通路的编程。

### 5.2 关键数据结构（`xcsi2txss.h`）

- `SubCoreCsi2Tx`：子核描述（与 RX 端 `CsiRxSsSubCore` 同构）；
- `XCsi2TxSs_Config`：含 `LanesPresent/DataType/PixelFormat/CsiBuffDepth/DphyLineRate/IsDphyRegIntfcPresent/FEGenEnabled` 及 `CsiInfo/DphyInfo` 子核配置；
- `XCsi2TxSs`：实例结构，持有 `CsiPtr`（必选）、`DphyPtr`（条件编译）及短包缓存 `SpktData`；
- `XCsi2TxSS_LCStatus` 枚举与时钟模式宏 `XCSI2TXSS_CCM`(连续时钟) / `XCSI2TXSS_NCCM`(非连续时钟) 直接对应核心驱动定义；
- 中断 Handler 宏（`XCSI2TXSS_HANDLER_*`）同样是对核心驱动 `XCSI2TX_HANDLER_*` 的别名透传。

### 5.3 主要 API（`xcsi2txss.c`）

| 函数 | 功能 |
|------|------|
| `XCsi2TxSs_CfgInitialize` | 探测并初始化子核（CSI2 Tx 必选，D-PHY 视配置可选） |
| `XCsi2TxSs_Configure` | 配置 active lanes 与中断掩码，转调 `XCsi2Tx_Configure` |
| `XCsi2TxSs_Activate` | 联动激活/禁用 CSI2 Tx 与 D-PHY |
| `XCsi2TxSs_Reset` | 复位 CSI2 Tx 子核 |
| `XCsi2TxSs_ReportCoreInfo` | 打印子核信息 |
| `XCsi2TxSs_GetShortPacket` | 转调核心驱动并缓存到 `SpktData` |
| `XCsi2TxSs_LineGen` / `SetGSPEntry` | 行生成模式控制 / 写入通用短包条目 |
| `XCsi2TxSs_GetPixelMode` / `GetMaxLaneCount` | 读取像素模式 / 最大 lane 数 |
| `XCsi2TxSs_SetClkMode` / `GetClkMode` | 设置/读取连续(CCM)与非连续(NCCM)时钟模式 |
| `XCsi2TxSs_IsUlps` / `SetUlps` | 查询/设置超低功耗状态(ULPS) |
| `XCsi2TxSs_SetLineCountForVC` / `GetLineCountForVC` | 设置/读取某 VC 的行计数（FE 生成特性，转调核心驱动） |

### 5.4 中断处理（`xcsi2txss_intr.c`）

`XCsi2TxSs_IntrHandler` / `SetCallBack` / `IntrDisable` 同样是对核心驱动 `XCsi2Tx_*` 接口的直接转发封装，不引入额外业务逻辑。

---

## 6. 设计共性总结

1. **分层一致**：RX 与 TX 方向均遵循「核心驱动 + 子系统驱动」两层模型，子系统层基本是核心驱动 API 的透传 + 多子核协同（联动 Activate/Reset、聚合状态缓存），不重复实现寄存器级逻辑。
2. **统一的初始化套路**：`LookupConfig → CfgInitialize → Configure(设置 lane 数/中断) → SetCallBack(注册中断回调) → IntrEnable → Activate(使能核心)`，RX/TX、核心/子系统四个驱动均遵循该范式。
3. **统一的位域操作风格**：所有寄存器访问都通过 `xcsi*_hw.h` 中的宏 + `x*.h` 中的内联 `BitSet/BitReset/GetBitField/SetBitField` 完成，保证可读性与一致性。
4. **配置流程的"备份-复位-恢复"模式**：`XCsi_Configure`/`XCsi2Tx_Configure` 在修改 active lanes 前都会备份中断使能寄存器（IER/GIER），软复位完成后再恢复，避免中断配置在复位过程中丢失；并通过回读校验配置是否真正生效。
5. **回调驱动的中断模型**：中断处理函数统一先取 `ISR & IER` 得到"已使能且发生"的中断集合，再按位掩码分类调用上层注册的回调（未注册则调用断言式的 `Stub*CallBack` 防止裸奔），最后清除已处理状态位；VCx 类错误（RX 的 VCX_FE、TX 的行计数错误）有专门的二次解析逻辑。
6. **条件编译的子核耦合**：子系统驱动通过 `XPAR_XDPHY_NUM_INSTANCES`、`XPAR_XIIC_NUM_INSTANCES` 等由 BSP 工具生成的宏，决定是否编译/链接 D-PHY、IIC 相关代码，使同一套子系统驱动源码可以适配"是否集成该子核"的不同硬件配置。
7. **`FixedLanes`/`FEGenEnabled` 等"特性开关"模式**：部分功能（如固定 lane 数、帧结束生成）由生成期参数控制，驱动在运行期通过 `if (Config.xxx)` 分支决定是否允许动态编程或直接返回 `XST_NO_FEATURE`/跳过配置，体现 IP 可裁剪的设计思想。

---

## 7. 文件速查索引

| 模块 | 头文件 | 寄存器定义 | 主体实现 | 中断 | 自检 | 静态初始化 |
|------|--------|-----------|---------|------|------|-----------|
| CSI Rx 核心 | `csi/src/xcsi.h` | `xcsi_hw.h` | `xcsi.c` | `xcsi_intr.c` | `xcsi_selftest.c` | `xcsi_sinit.c` / `xcsi_g.c` |
| CSI2 Tx 核心 | `csi2tx/src/xcsi2tx.h` | `xcsi2tx_hw.h` | `xcsi2tx.c` | `xcsi2tx_intr.c` | `xcsi2tx_selftest.c` | `xcsi2tx_sinit.c` / `xcsi2tx_g.c` |
| CSI Rx 子系统 | `mipicsiss/src/xcsiss.h` | `xcsiss_hw.h` | `xcsiss.c` | `xcsiss_intr.c` | `xcsiss_selftest.c` | `xcsiss_sinit.c` / `xcsiss_g.c` |
| CSI2 Tx 子系统 | `csi2txss/src/xcsi2txss.h` | `xcsi2txss_hw.h` | `xcsi2txss.c` | `xcsi2txss_intr.c` | `xcsi2txss_selftest.c` | `xcsi2txss_sinit.c` / `xcsi2txss_g.c` |

范例代码：
- `csi/examples/xcsi_example_selftest.c`
- `csi2tx/examples/xcsi2tx_example_selftest.c`
- `mipicsiss/examples/xcsiss_selftest_example.c`、`xcsiss_intr_example.c`
- `csi2txss/examples/xcsi2txss_selftest_example.c`、`xcsi2txss_intr_example.c`
