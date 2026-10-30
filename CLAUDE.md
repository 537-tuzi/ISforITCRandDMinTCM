# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概览

中医舌苔智能识别与诊疗管理一体化系统：RV1106 边缘摄像头本地 YOLOv8 推理 → 串口上报 → Ubuntu C++ 服务端（规划中）→ 三角色 Qt 客户端（病人端/医生端/管理员端）。四个组成部分：

| 部分 | 位置 | 状态 |
|---|---|---|
| 模型训练 | `training/`（YOLOv8n，5 类舌苔） | 已完成，best.pt 已转 rknn |
| 采集端（板端 C++） | `采集端代码/` | 已完成并在 RV1106 上验证 |
| 部署模型 | `models/tongue.rknn` | 已部署板端 |
| 病人端客户端 | `客户端/Patient/`（Qt 6 Widgets） | 原型阶段开发中（演示数据，无真实网络层） |

服务端（epoll + 线程池 + MySQL 8.0）尚未开始，规划放 `服务端/`（新建须登记）。医生端、管理员端客户端尚未开始。

文档、注释、提交信息全部使用中文。

## 常用命令

### 病人端 Qt 客户端（Windows，Qt 6.11.1 MinGW 64-bit）

```powershell
cd 客户端\Patient
D:\qt6\6.11.1\mingw_64\bin\qmake.exe Patient.pro -spec win32-g++
mingw32-make            # 或 mingw32-make debug / release；通常直接用 Qt Creator 打开 Patient.pro
```

- 需把 `D:\qt6\Tools\mingw1310_64\bin` 加入 PATH，否则 g++ 找不到 `cc1plus`
- **qmake 的 Makefile 不跟踪 style.qss 依赖**：改动 `.qss` 后 `rcc` 不会重跑，exe 里仍是旧样式。改样式后须删除 `release/qrc_resources.cpp`、`debug/qrc_resources.cpp`（或重跑 qmake）再编译；改 `.pro` 文件清单后也须重跑 qmake

- TTS 默认关闭：`.pro` 中 `QT += texttospeech` 与 `DEFINES += PATIENT_TTS` 两行被注释，未启用时 `view/Speak.h` 静默跳过，不影响业务。
- 无测试设施；不要虚构测试命令。

### 模型训练与导出（Ubuntu 22.04 + Miniconda + ultralytics）

```bash
python training/train.py        # 注意：脚本内 data 路径硬编码为 /home/ma-user/work/data.yaml，本地跑需先改
yolo export model=training/best.pt format=onnx
python convert.py best.onnx rv1106 i8 models/tongue.rknn   # 校准数据集在 training/转rknn模型用的校准数据集/
```

### 采集端（RV1106 板端）

`采集端代码/main.cc` 依赖凌智视觉模块 SDK（lockzhiner_vision_module）与 arm-rockchip830 交叉工具链，只能在 Ubuntu 22.04 虚拟机上编译（Makefile/产物在虚拟机侧，不入库），Windows 上无法构建。`yolov8.h`、`yolov8_main` 等编译产物不在仓库中。

## 核心架构

### 舌苔类别顺序（全链路强一致，禁止改动）

```
0:灰黑苔  1:镜面舌  2:薄白苔  3:白腻苔  4:黄腻苔
```

该顺序同时约束 `training/data.yaml`、`采集端代码/tongue_def.h`（TongueClass 枚举）、串口协议类别码（0xFF=未识别/异常）、病人端检测结果库。任何一处的类别顺序变动都是跨端变更。

### 业务判定规则（来自项目手册，有据可查，不可自行更改）

- 置信度 ≥80% 才判定有效（`TONGUE_CONF_THRESHOLD`）
- 薄白苔=健康，其余=病体；灰黑苔/镜面舌=重症，触发系统自动预约
- 严重度分级：灰黑苔 > 镜面舌 > 黄腻苔 > 白腻苔 > 薄白苔
- 就诊前 30 分钟内不可取消预约

### 串口协议（详见 `docs/RV1106串口通信协议说明.md`）

- 下行（PC→RV1106）4 字节：`0x5A | CMD_H | CMD_L | 0xA5`（0x0010 单帧推理、0x0100 连续推理等）
- 上行（RV1106→PC）6 字节：`0x5B | CMD_H | CMD_L | 类别码 | 置信度 | 0xA5`
- 115200 8N1；**图片不走串口**（仅 6 字节结果帧；图片规划走网络通道）

### 病人端架构（`客户端/Patient/病人端设计说明.md` 是实现规格）

原型阶段：**WinManager**（窗口创建/显示/跳转/导航守卫，信号槽接线中枢）+ **view 层**（LoginView/MainWindow/HomeView/ProfileView/DetectView/RecordView，业务演示逻辑全部内聚在各 View；弹窗由各 View 自行 exec()）。接真实服务端时计划补 service 层（QTcpSocket + JSON），View/WinManager 结构不变。

**该文档中所有界面文案均为逐字标准，实现时不得改写**——改 UI 文案前先核对设计说明的 toast 总表（20 条）、演示对话脚本（4 轮）、五类检测结果库。

### 数据库

`docs/zysms_db.sql` 一键建库 `zysms_db`（utf8mb4，10 张表：patient/staff/department/schedule/tongue_record/medical_record/appointment/prescription/system_config/system_log），密码存 MD5，联调测试数据默认密码 123456。表设计见 `docs/数据库设计文档.md`。

## 文件归类规则（新增任何文件前必读 `docs/文件分类整理规则.md`）

- 仓库只放交付物：自己写的源码、项目文档、小体积必要产物（.rknn/.pt）
- 不入库（已被 .gitignore 忽略，仅存本地）：`references/`（第三方/课程资料）、`board-rv1106/`（厂商资料包 5.6GB）、`客户端/*原型*.html`、`training/yolo_dataset.zip`、进度表/日报/手册 PDF、一切编译产物（ELF/*.o/build/Debug/Release）
- 单文件 >50MB 提交前警惕，>100MB 禁止提交
- 命名：文档用中文自解释命名；代码文件用英文小写；日期 `YYYY-MM-DD`；版本 `_v2` 递增，禁止"最终版/改/新"等模糊后缀
- 确需新建目录：在 `docs/文件分类整理规则.md` 的"目录变更登记表"登记，并同步 `.gitignore` 与 `README.md` 目录结构

## 决策记录

`docs/采集端与数据库_自定决策清单.md` 记录了资料未明确规定、由 AI 自行做出的工程决策（重试语义、抓拍节流、JPG 编码策略等），多处"你的结论"栏待用户逐项核对——改动这些设计前先查此文档，涉及业务规则（阈值/类别顺序/重症定义）的以项目手册为准，不在此列。
