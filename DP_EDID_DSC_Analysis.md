# DisplayPort 驱动：EDID 与 DSC 实现分析

> 仓库：embeddedsw.github.io（Xilinx Embedded Software）
> 分析日期：2026-06-06

---

## 一、EDID 实现

### 1.1 总体架构

EDID 读取基于 **I2C-over-AUX** 通道机制，实现文件在三个版本中完全相同：

```
dp12/src/xdp_edid.c   ← 实现（三版本内容一致）
dp14/src/xdp_edid.c
dp21/src/xdp_edid.c

dp14/src/xdp.h        ← API 声明（Line 1523–1536）
dp14/src/xdp_hw.h     ← 寄存器/偏移定义（Line 2361, 2922–3043）
```

EDID 读取分两条路径：

```
直连路径（Direct）
  XDp_TxGetEdid()
    └─ XDp_TxGetEdidBlock()
         └─ XDp_TxIicRead()          I2C Addr=0x50, block=128B

远端路径（Remote / MST）
  XDp_TxGetRemoteEdid()
    └─ XDp_TxGetRemoteEdidBlock()
         └─ XDp_TxRemoteIicRead()    sideband 消息，需 LinkCountTotal + RelativeAddress
```

### 1.2 EDID 数据帧结构

#### Base Block（128 字节，Block 0）

| 偏移 | 字节数 | 含义 |
|------|--------|------|
| 0x00 | 8 | 固定头 `00 FF FF FF FF FF FF 00` |
| 0x08 | 2 | 厂商 ID |
| 0x12 | 1 | EDID 版本 |
| 0x13 | 1 | EDID 修订 |
| 0x36 | 18 | Detailed Timing Descriptor #1（优选模式 PTM） |
| 0x48 | 18 | Detailed Timing Descriptor #2 |
| 0x7E | 1 | Extension Block 数量 |
| 0x7F | 1 | Checksum |

#### Detailed Timing Descriptor（DTD）字段解析

驱动在 `xdp_hw.h` 中定义了 DTD 内全部偏移宏（`XDP_EDID_DTD_*`）：

| 偏移 | 宏名 | 含义 |
|------|------|------|
| 0x00–0x01 | `PIXEL_CLK_KHZ_*` | 像素时钟（10 kHz 单位，16 bit） |
| 0x02 | `HRES_LSB` | 水平有效像素低 8 bit |
| 0x03 | `HBLANK_LSB` | 水平消隐低 8 bit |
| 0x04 | `HRES_HBLANK_U4` | 水平有效/消隐高 4 bit |
| 0x05 | `VRES_LSB` | 垂直有效行数低 8 bit |
| 0x06 | `VBLANK_LSB` | 垂直消隐低 8 bit |
| 0x07 | `VRES_VBLANK_U4` | 垂直有效/消隐高 4 bit |
| 0x08–0x0A | `HSO/HSW/VSO_VSW` | 水平/垂直同步前肩与宽度 |
| 0x0C–0x0D | `HSIZE_MM / VSIZE_MM` | 物理尺寸（mm） |
| 0x11 | `SIGNAL` | Bit1=H极性，Bit2=V极性，Bit4-5=立体声 |

> **像素时钟计算**：`CLK_Hz = (MSB << 8 | LSB) × 10000`

### 1.3 Extension Block 处理

驱动支持多种扩展块，重点支持 **DisplayID** 扩展（tag = `0x70`）：

```
XDp_TxGetRemoteEdidDispIdExt()
  ├─ 读 Base Block → 获取扩展块数量（offset 0x7E）
  ├─ 循环读每个扩展块
  └─ 匹配 tag == 0x70 → 返回 DisplayID 块

XDp_TxGetDispIdDataBlock()
  ├─ 在 DisplayID 载荷中搜索指定 Section Tag
  └─ Section 格式：[tag(1B)][rev(1B)][size(1B)][data(N B)]

XDp_TxGetRemoteTiledDisplayDb()
  └─ 组合操作：GetDispIdExt → 搜索 Tiled Display Topology(tag=0x12)
     用于多屏拼接场景的拓扑发现
```

### 1.4 调用示例

```c
/* 读取直连显示器的 EDID base block */
u8 EdidBuf[XDP_EDID_BLOCK_SIZE];
Status = XDp_TxGetEdid(DpPtr, EdidBuf);

/* 读取 MST 下游设备 EDID */
Status = XDp_TxGetRemoteEdid(DpPtr,
    LinkCountTotal, RelativeAddress, EdidBuf);

/* 提取像素时钟（kHz） */
u32 PixClk = (EdidBuf[XDP_EDID_DTD_PIXEL_CLK_KHZ_MSB] << 8 |
              EdidBuf[XDP_EDID_DTD_PIXEL_CLK_KHZ_LSB]) * 10;

/* 提取水平分辨率 */
u32 HRes = ((EdidBuf[XDP_EDID_DTD_HRES_HBLANK_U4] >> 4) << 8) |
             EdidBuf[XDP_EDID_DTD_HRES_LSB];
```

### 1.5 错误处理

| 返回值 | 含义 |
|--------|------|
| `XST_SUCCESS` | 读取并校验成功 |
| `XST_ERROR_COUNT_MAX` | AUX/I2C 事务超时 |
| `XST_DEVICE_NOT_FOUND` | 未检测到 RX 设备 |
| `XST_FAILURE` | 读取或数据校验失败 |

AUX 通道重试机制在底层 `XDp_TxIicRead()` 内处理，上层 EDID 函数感知最终结果。

### 1.6 版本差异

| 特性 | dp12 | dp14 | dp21 |
|------|:----:|:----:|:----:|
| Base EDID 读取 | ✓ | ✓ | ✓ |
| Extension Block | ✓ | ✓ | ✓ |
| DisplayID (0x70) | ✓ | ✓ | ✓ |
| Tiled Display Topology | ✓ | ✓ | ✓ |
| 实现代码差异 | — | — | 完全相同 |

三个版本 `xdp_edid.c` 内容**完全一致**，EDID 功能无版本分化。

---

## 二、DSC（Display Stream Compression）实现

### 2.1 实现层级与定位

> **关键结论**：驱动层只提供 **DPCD 寄存器宏定义**，用于能力读取与协商；DSC 编解码由硬件引擎完成，驱动不实现软件编码器，也不生成 PPS（Picture Parameter Set）。

```
软件驱动层（xdp_hw.h）
  └─ DPCD 0x00060–0x0006E 寄存器宏         ← 能力读取
  └─ XDp_TxAuxRead() / XDp_TxAuxWrite()   ← 寄存器访问

应用/框架层（用户负责）
  └─ 读取能力 → 决策是否启用 DSC
  └─ 计算 BPP、slice 配置

硬件层
  └─ DSC 编码引擎（自动处理 PPS 生成与传输）
```

### 2.2 DSC DPCD 寄存器全表

寄存器位于 DPCD 地址空间 `0x00060–0x0006E`，在 `dp14/src/xdp_hw.h` 和 `dp21/src/xdp_hw.h` 中定义完全相同。

#### 0x00060：DSC_SUPPORT

| 位 | 掩码 | 含义 |
|----|------|------|
| 0 | `0x1` | Sink 是否支持 DSC（1=支持） |

#### 0x00061：DSC_ALGORITHM_REVISION

| 位域 | 掩码 | 含义 |
|------|------|------|
| [3:0] | `0x0F` | DSC 规范主版本号（Major） |
| [7:4] | `0xF0` | DSC 规范次版本号（Minor） |

#### 0x00062：DSC_RC_BUFFER_BLOCK_SIZE

| 值 | 含义 |
|----|------|
| 0x0 | 1 KB |
| 0x1 | 4 KB |
| 0x2 | 16 KB |
| 0x3 | 64 KB |

#### 0x00063：DSC_RC_BUFFER_SIZE

- `[3:0]` 为块数量，实际 RC Buffer 大小 = 块数 × 块大小（0x62）

#### 0x00064：DSC_SLICE_CAPABILITIES_1

| Bit | 掩码 | 支持的 Slice 数 |
|-----|------|-----------------|
| 0 | `0x01` | 1 slice |
| 1 | `0x02` | 2 slices |
| 3 | `0x08` | 4 slices |
| 4 | `0x10` | 6 slices |
| 5 | `0x20` | 8 slices |
| 6 | `0x40` | 10 slices |
| 7 | `0x80` | 12 slices |

#### 0x00065：DSC_LINE_BUFFER_BIT_DEPTH

| 值 | 行缓冲位深 |
|----|-----------|
| 0x8 | 8 bpc |
| 0x0 | 9 bpc |
| 0x1 | 10 bpc |
| 0x2 | 11 bpc |
| 0x3 | 12 bpc |
| 0x4 | 13 bpc |
| 0x5 | 14 bpc |
| 0x6 | 15 bpc |
| 0x7 | 16 bpc |

#### 0x00066：DSC_BLOCK_PREDICTION_SUPPORT

| 位 | 掩码 | 含义 |
|----|------|------|
| 0 | `0x1` | 支持块预测优化 |

#### 0x00069：DSC_DECODER_COLOR_FORMAT_CAPABILITIES

| Bit | 掩码 | 色彩格式 |
|-----|------|----------|
| 0 | `0x01` | RGB |
| 1 | `0x02` | YCrCb 4:4:4 |
| 2 | `0x04` | YCrCb 4:2:2 Simple |
| 3 | `0x08` | YCrCb 4:2:2 Native |
| 4 | `0x10` | YCrCb 4:2:0 Native |

#### 0x0006A：DSC_DECODER_COLOR_DEPTH_CAPABILITIES

| 位 | 掩码 | 色深 |
|----|------|------|
| 1 | `0x02` | 8 bpc |
| 2 | `0x04` | 10 bpc |
| 3 | `0x08` | 12 bpc |

#### 0x0006B：DPCD_PEAK_DSC_THROUGHPUT

| 值 | 吞吐量 |
|----|--------|
| 0 | 不支持 |
| 1 | 340 Mbps |
| 2 | 400 Mbps |
| 3–14 | 450~1000 Mbps（步进 50 Mbps） |

- `[3:0]`：Mode 0 峰值吞吐量
- `[7:4]`：Mode 1 峰值吞吐量

#### 0x0006C：DSC_MAXIMUM_SLICE_WIDTH

- `[7:0]`：最大 slice 宽度（像素），限制水平分割粒度

#### 0x0006D：DSC_SLICE_CAPABILITIES_2（DP 1.4+）

| Bit | 掩码 | 支持的 Slice 数 |
|-----|------|-----------------|
| 0 | `0x01` | 16 slices |
| 1 | `0x02` | 20 slices |
| 2 | `0x04` | 24 slices |

#### 0x0006E：DPCD_BITS_PER_PIXEL_INCREMENT

| 值 | BPP 步进精度 |
|----|-------------|
| 0x0 | 1/16 BPP |
| 0x1 | 1/8 BPP |
| 0x2 | 1/4 BPP |
| 0x3 | 1/2 BPP |
| 0x4 | 1 BPP |

### 2.3 DSC 能力读取流程

```c
/* Step 1：检查 Sink 是否支持 DSC */
u8 DscSupport;
XDp_TxAuxRead(DpPtr, 0x00060, 1, &DscSupport);
if (!(DscSupport & 0x1)) {
    /* DSC 不支持，使用非压缩模式 */
}

/* Step 2：读取 DSC 算法版本 */
u8 AlgoRev;
XDp_TxAuxRead(DpPtr, 0x00061, 1, &AlgoRev);
u8 Major = AlgoRev & 0x0F;
u8 Minor = (AlgoRev >> 4) & 0x0F;   /* 通常期望 1.2 版本 */

/* Step 3：读取 RC Buffer 配置 */
u8 RcBuf[2];
XDp_TxAuxRead(DpPtr, 0x00062, 2, RcBuf);
u32 BlockSize = (1 << (10 + 2 * (RcBuf[0] & 0x3)));  /* 1K/4K/16K/64K */
u32 TotalRcBuf = (RcBuf[1] & 0xF) * BlockSize;

/* Step 4：读取 Slice 能力 */
u8 SliceCap1, SliceCap2;
XDp_TxAuxRead(DpPtr, 0x00064, 1, &SliceCap1);
XDp_TxAuxRead(DpPtr, 0x0006D, 1, &SliceCap2);
u8 MaxSliceWidth;
XDp_TxAuxRead(DpPtr, 0x0006C, 1, &MaxSliceWidth);

/* Step 5：读取色彩格式/深度能力 */
u8 ColorFmt, ColorDepth;
XDp_TxAuxRead(DpPtr, 0x00069, 1, &ColorFmt);
XDp_TxAuxRead(DpPtr, 0x0006A, 1, &ColorDepth);

/* Step 6：读取峰值吞吐量和 BPP 步进 */
u8 Throughput, BppInc;
XDp_TxAuxRead(DpPtr, 0x0006B, 1, &Throughput);
XDp_TxAuxRead(DpPtr, 0x0006E, 1, &BppInc);
```

### 2.4 DSC 在链路中的作用

DSC 的核心价值是**降低有效带宽需求**，使高分辨率/高刷新率场景在既有链路速率下可行：

```
未压缩带宽 = PixelClock × (BitsPerColor/8) × ColorComponents × PixelWidth
压缩后带宽 = 未压缩带宽 × (TargetBPP / UncompressedBPP)

例：4K@120Hz RGB 10-bit
  未压缩 ≈ 594MHz × 30bpp = 17.82 Gbps
  HBR3 4-lane ≈ 8.1Gbps × 4 × 0.8 ≈ 25.92 Gbps  → 可传
  HBR2 4-lane ≈ 5.4Gbps × 4 × 0.8 ≈ 17.28 Gbps  → 不可传
  启用 DSC 3:1 压缩 → 约 5.94 Gbps → HBR2 可传
```

#### DSC 与 MST 的联动

在 MST 模式下，DSC 压缩影响 **PBN（Payload Bandwidth Number）** 的计算：

```
PBN = ceil(PixelClock × BitsPerPixel / (54 × 0.9824 × 8))
启用 DSC 后：PBN ∝ 压缩后 BPP，可在同一链路承载更多 MST 流
```

### 2.5 DSC 与硬件集成点

| 阶段 | 软件行为 | 硬件行为 |
|------|----------|----------|
| 链路训练 | AuxRead DPCD 0x00060 | 自动发现 DSC 能力 |
| 流建立前 | 读取全部 DSC 能力寄存器，决策是否启用 | — |
| 流建立 | 通过 DPCD 写入 DSC 使能标志（应用侧） | DSC 编码引擎启动 |
| PPS 传输 | — | 硬件自动生成并发送 PPS 到 Sink |
| 视频传输 | 正常配置 MSA（分辨率、时序） | 按 DSC 压缩参数编码输出 |

> 注：DSC 编码引擎的寄存器配置（slice 数、BPP 目标等）通过 Vivado IP 参数完成，不通过本驱动 API 设置。

### 2.6 版本支持对比

| 特性 | dp12 | dp14 | dp21 | dppsu |
|------|:----:|:----:|:----:|:-----:|
| DSC DPCD 寄存器定义 | — | ✓ | ✓ | — |
| DSC 能力读取 API | — | `XDp_TxAuxRead` | `XDp_TxAuxRead` | — |
| DSC 软件编码器 | — | — | — | — |
| PPS 软件生成 | — | — | — | — |
| DSC 1.2 版本支持 | — | ✓ | ✓ | — |
| 扩展 Slice 数（16/20/24） | — | ✓ | ✓ | — |

---

## 三、EDID 与 DSC 联动

在实际 TX 驱动流程中，EDID 和 DSC 能力读取是**顺序执行**的协商步骤：

```
1. XDp_TxGetEdid()
   └─ 获取显示器支持的视频模式（分辨率/刷新率/色彩）

2. 从 EDID 选定目标视频模式（如 4K@120Hz RGB 10-bit）

3. 计算未压缩带宽需求

4. 与当前链路能力（LaneCount × LinkRate）比较
   ├─ 带宽足够 → 跳过 DSC
   └─ 带宽不足
        ├─ XDp_TxAuxRead(0x00060) 检查 DSC_SUPPORT
        ├─ 读取 Slice/BPP/Throughput 能力
        ├─ 计算最优 DSC 压缩比（目标 BPP）
        └─ 配置硬件 DSC 参数，重新建立链路

5. XDp_TxEstablishLink() / XDp_TxSetVideoMode()
```

---

## 四、关键 API 速查

### EDID

| 函数 | 说明 |
|------|------|
| `XDp_TxGetEdid(inst, buf)` | 读取直连显示器 Base EDID（128B） |
| `XDp_TxGetEdidBlock(inst, buf, n)` | 读取第 n 个 EDID Block |
| `XDp_TxGetRemoteEdid(inst, lct, ra, buf)` | 读取 MST 下游设备 EDID |
| `XDp_TxGetRemoteEdidBlock(inst, buf, n, lct, ra)` | 读取 MST 下游设备指定 Block |
| `XDp_TxGetRemoteEdidDispIdExt(inst, buf, lct, ra)` | 获取 DisplayID 扩展块 |
| `XDp_TxGetDispIdDataBlock(buf, tag, db)` | 在 DisplayID 中搜索指定 Section |
| `XDp_TxGetRemoteTiledDisplayDb(inst, buf, lct, ra, db)` | 获取 Tiled Display Topology 数据块 |

### DSC（均通过通用 AUX 接口访问）

| DPCD 地址 | 含义 |
|-----------|------|
| `0x00060` | DSC 支持标志 |
| `0x00061` | DSC 算法版本（Major.Minor） |
| `0x00062–0x00063` | RC Buffer 块大小与总数 |
| `0x00064` | Slice 能力 1（1–12 slices） |
| `0x00065` | 行缓冲位深 |
| `0x00066` | 块预测支持 |
| `0x00069` | 解码器色彩格式能力 |
| `0x0006A` | 解码器色深能力 |
| `0x0006B` | 峰值 DSC 吞吐量 |
| `0x0006C` | 最大 Slice 宽度 |
| `0x0006D` | Slice 能力 2（16–24 slices，DP1.4+） |
| `0x0006E` | BPP 步进精度 |
