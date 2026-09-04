# AuthorityArena 0.1 需求单

## 原始意图

在 2026-09-05 15:30（UTC+8）前创建并交付公开仓库 `Iviesever/authority-arena-ue5`：一个可运行、可测试、可审计、可解释、可发布的 Unreal Engine 5.8 C++ 服务端权威多人战斗垂直切片。完整原文以同目录 `goal-objective.md` 为最高产品合同，任何摘要都不得静默削弱它。

## 产品定位

`AuthorityArena — UE 5.8 Server-Authoritative Multiplayer Lab` 面向 UE Gameplay/C++/网络同步岗位与技术面试，重点证明 UE 原生复制、网络角色、RPC、RepNotify、Gameplay Ability System、客户端预测、服务端验证/拒绝、真实多进程网络模拟和可复现的 Win64 本地交付链路。

它与 `RollbackLab` 的自制确定性模拟、快照回滚和自定义 UDP 路线不同；本项目不重新实现 rollback 框架。

## P0 结果

- 一个程序化灰盒竞技场，一台权威服务端与两个独立客户端进程。
- C++ 实现角色移动/朝向、Projectile Attack、Dash、Shield/Block、Health、Energy、Cooldown、Death、Respawn、Score。
- GAS 真实承担三项能力，Dash 使用 UE 预测机制，伤害和最终状态只能由服务端决定。
- 非法频率、资源不足、死亡态、错误目标、伪造数值与重复生成等请求 fail closed，并保留结构化拒绝原因。
- PowerShell 7 多进程入口覆盖正常、延迟、抖动、少量丢包、断连、服务端关闭、连接失败、非法请求和 watchdog。
- Editor/Game Development、Game Shipping、UE Automation、Cook/Stage/Pak/Archive、打包启动、打包双客户端、clean-source 重验全部有真实证据。
- 作品集 README、架构/网络/GAS/测试/构建/AI 披露/面试/Live Change Drill/Release Notes 文档与真实运行证据一致。
- 通过独立只读审计且无 Blocker/High 后，正常合并 PR、创建 annotated `v0.1.0` tag 与 source-only GitHub Release；Release 不得含自定义附件。

## 不可违反的约束

- 核心权威逻辑使用 C++；蓝图仅可用于参数与非权威表现连接。
- 开发初期对 MQB 5.4.0 做有边界、可重复的能力验证；MQB 能正确承担的目标必须优先由 MQB 构建，UE 专用步骤仅在证据支持时回退到 UBT/RunUAT。
- 只允许一个写入 Agent；其他 Agent 仅可只读审计。
- 不覆盖未知文件，不干扰无关进程，不上传本地二进制、Cook/Stage/Pak、日志或 Trace 到 Release。
- 不引入在线服务、账号、数据库、云服务器、匹配、NAT、商城、背包、剧情、复杂 AI、大量第三方美术、自制通用网络协议或无关重构。
- 若任何 P0 门槛无法真实证明，则维持 Draft PR 和 Alpha/WIP 交接，不创建虚假的正式 Release。

## 已恢复的真实起始状态

- 2026-09-04 22:38（UTC+8）当前 GitHub 活动身份为 `Iviesever`。
- `Iviesever/authority-arena-ue5` 不存在：目标 API 返回 404，账号 owner 仓库列表无精确同名项。
- `D:\program\authority-arena-ue5` 原先不存在；`D:\program` 不是 Git 仓库；未发现 AuthorityArena 命名冲突。
- `D:\program\.agents` 原先不存在，因此没有可继承的目录规则或工作流。
- UE 5.8.0 位于 `D:\program\UnrealEngine\Epic Games\UE_5.8`，CL 55116800；Editor-Cmd、Build.bat、RunUAT.bat 均存在。
- Visual Studio Community 2026 18.7.3、MSVC 14.51.36231（并存 14.44.35207）、Windows SDK 10.0.26100.0 已安装。
- PowerShell 7.6.0、MQB 5.4.0、Git 2.51.2.windows.1、GitHub CLI 2.96.0 可用。
- 审计快照中无 UnrealEditor、UBT、RunUAT、ShaderCompileWorker、Cook、Package、Automation 或测试进程。
- 已有且不得覆盖的相邻项目：`D:\program\RollbackLab`。

## 已锁定设计决策

- 依据最新目标的持续自主执行授权，采用第三人称灰盒视角；该决定由执行 Agent 作出，不冒充用户亲自选择。
- 采用 UE 原生垂直切片 + 共享纯 C++ 权威规则核心，使 MQB 和 UBT 编译同一份无引擎依赖规则代码。
- Launcher 版 UE 对 Dedicated Server target 的可用性以 PACT-00 实测决定；若缺少所需组件，则按合同降级为独立 listen/game server 进程并准确记录。

## 完成定义

只有同目录 `goal-objective.md` 第 17 节的全部条件具有可复现命令、日志、JSON、截图/报告、源 SHA 与本地产物 SHA-256 证据，并通过独立审计，才可宣称 `v0.1.0` 完成。
