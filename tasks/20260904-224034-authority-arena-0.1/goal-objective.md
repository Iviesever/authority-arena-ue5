# AuthorityArena UE5.8 完整交付

创建、实现、验证、审计并正式发布一个新的公开作品集仓库：

- GitHub：`Iviesever/authority-arena-ue5`
- 本地目录：在 `D:\program` 下选择不会覆盖任何现有目录的位置
- 引擎：本机已安装的 Unreal Engine 5.8
- 主要语言：C++，核心逻辑不得依赖蓝图实现
- 目标版本：`v0.1.0`
- 最终期限：2026 年 9 月 5 日 15:30（UTC+8）

这是一项完整交付任务，不是只创建骨架、设计文档或概念验证。必须持续推进到可运行、可测试、可审计、可解释、可发布的作品集状态；若确因真实技术障碍无法完成全部 P0，必须诚实降级为未发布的 Alpha/WIP，不得伪造完成度。

---

## 0. 开始前恢复真实状态

开始任何写入前：

1. 检查 GitHub 当前登录身份，必须是 `Iviesever`。
2. 检查目标仓库是否已经存在：
   - 不存在则创建公开仓库；
   - 已存在则读取真实默认分支、HEAD、分支、PR、Release 和工作流状态，不得覆盖已有内容。
3. 检查目标本地目录是否存在：
   - 不存在则创建；
   - 存在则先读取其真实 Git 状态，不得删除、重置或覆盖未知文件。
4. 检查本机：
   - UE 5.8 安装路径；
   - Visual Studio/MSVC；
   - Windows SDK；
   - PowerShell 7；
   - MQB；
   - Git；
   - 当前运行的 UE、UBT、RunUAT、ShaderCompileWorker、Cook、Package 和测试进程。
5. 不得结束、重启或干扰与本项目无关的进程。
6. 同一仓库只能由一个写入 Agent 工作。其他 Agent只能进行只读审计，不得并行修改该仓库。
7. 创建并完整遵守：
   - `AGENTS.md`
   - `.agents/AGENTS.md`
   - `docs/PRODUCT_CONTRACT.md`
   - `docs/ARCHITECTURE.md`
   - `docs/ACCEPTANCE_MATRIX.md`
   - `tasks/<timestamp>-authority-arena-0.1/goal-objective.md`
   - `tasks/<timestamp>-authority-arena-0.1/progress.md`
   - `tasks/<timestamp>-authority-arena-0.1/handoff.md`
8. 将本 `/goal` 原文保存到任务目录中，作为不可静默削弱的最高产品合同。

---

## 1. 最高产品目标

构建一个真正可运行的 UE 5.8 C++ 服务端权威多人战斗垂直切片，系统性证明：
```text
UE Client / Server
→ Actor Replication
→ Authority / Autonomous Proxy / Simulated Proxy
→ Replicated Properties / RepNotify / RPC
→ Gameplay Ability System
→ Client Prediction
→ Server Validation / Rejection
→ Real Multi-process Server + Clients
→ Network Emulation
→ Automated Verification
→ Packaged Win64 Evidence
```

项目名称暂定：

> **AuthorityArena — UE 5.8 Server-Authoritative Multiplayer Lab**

它不是大型游戏，也不是美术作品；它是面向 C++ 游戏开发实习、UE Gameplay、客户端、网络同步和技术面试的工程型作品集。

项目必须与 `RollbackLab` 明确区分：

- `RollbackLab`：自制确定性模拟、输入预测、状态快照、回滚重演、自定义 UDP；
- `AuthorityArena`：UE 原生客户端—服务端复制、Actor/Component 生命周期、RPC、RepNotify、GAS 和服务端权威。

必须提供：

- `docs/ROLLBACK_VS_UE_REPLICATION.md`

解释两种网络模型的目标、所有权、预测、纠错、带宽、确定性和适用场景差异。

---

## 2. 不可违反的 MQB 构建政策

### 2.1 MQB 优先级

本项目必须优先尝试使用本机的 `mqb` 构建工具。

开始开发后尽早执行一次有边界的 MQB 能力验证，确认 MQB 是否能够：

1. 编译本项目中的独立纯 C++ 模块或辅助工具；
2. 编译 UE C++ 源文件；
3. 正确处理 UE 生成代码、Include 路径、宏、链接依赖和目标配置；
4. 直接或通过稳定脚本编排 UE 项目构建；
5. 产生与正常 UE 构建路径一致且可运行的结果。

### 2.2 成功时的强制选择

若 MQB 能够成功、正确、可重复地构建对应目标：

- 必须将 MQB 作为该目标的首选本地构建入口；
- 必须优先使用 MQB，而不是 Clang、GCC、手工 `cl.exe`、普通 CMake/Ninja 或其他替代构建路径；
- `scripts/Build.ps1`、README 和验证脚本应默认进入 MQB 路径；
- 必须记录 MQB 版本、命令、构建输出、增量构建结果和产物身份；
- 不得在 MQB 已经成功的情况下，仅为了“常规做法”重新切回 Clang 或其他工具。

### 2.3 UE 专用工具的边界

Cook、Stage、Pak、IoStore、BuildCookRun、Automation、Commandlet 和正式 UE Target 构建若必须由 UBT/RunUAT 完成，可以使用 UE 官方工具。

优先顺序为：
```text
能由 MQB 正确完成
→ 使用 MQB

MQB 可以作为稳定前端或编排器调用 UE 官方工具
→ 使用 MQB 入口

MQB 经验证无法承担该 UE 专用目标
→ 记录准确原因，再回退到 UBT / RunUAT
```

不得宣称 MQB 替代了 UBT，除非真实证据证明它完成了同等必要步骤。

### 2.4 MQB 失败时

若 MQB 无法构建完整 UE 项目或插件：

- 保存精确命令、错误输出、版本和缺失能力；
- 在 `docs/BUILD_SYSTEM.md` 说明限制；
- 对 MQB 能成功构建的独立纯 C++ 部分仍然使用 MQB；
- UE 专用部分回退到 UBT/RunUAT；
- 不得无限循环尝试同一失败方案；
- 不得因为 MQB 完整 UE 支持失败，就默认改用 Clang；
- Clang 或其他编译器只允许在 P0 全部完成后作为附加兼容性验证，不得取代 MQB/MSVC 主线，也不得延误正式交付。

---

## 3. 不可违反的 Release 发布政策

最终必须进行真实本地构建、Cook、Package 和多进程运行验证，但：

> **不得把任何打包作品上传到 GitHub Release。**

GitHub Release 中禁止上传：

- Win64 打包游戏；
- `.exe`、`.dll`、`.pdb`；
- Packaged Demo ZIP；
- Plugin ZIP；
- SDK ZIP；
- 手工生成的 Source ZIP；
- Cook/Stage/Pak 文件；
- 日志压缩包；
- 大型截图包；
- Trace 包；
- 任何因体积大、上传慢而影响交付的自定义附件。

最终 `v0.1.0` Release 只使用 GitHub 自动生成的：

- `Source code (zip)`
- `Source code (tar.gz)`

Release 的 `assets` 必须为空。

本地打包仍然是强制验收项，但只保存在被 `.gitignore` 排除的本地 `Artifacts/` 或 `artifacts/` 目录中，并在 Release Notes、PR 和验证文档中记录：

- 对应源代码 SHA；
- 本地产物路径；
- 文件大小；
- SHA-256；
- Build/Cook/Stage/Pak/Archive 结果；
- 运行和测试结果。

仓库中允许提交少量、经过验证且体积合理的：

- `docs/images/*.png`
- 一张短 GIF，若体积可控
- 小型 JSON 示例
- 精简测试报告示例

这些文件必须来自真实运行，不得伪造或混用旧版本结果。

---

## 4. 明确范围与非目标

### P0 范围

必须完成一个灰盒竞技场：

- 一台服务端；
- 两个独立客户端进程；
- 角色移动和朝向；
- 攻击；
- Dash；
- Shield/Block；
- Health；
- Energy；
- Cooldown；
- Death；
- Respawn；
- Score；
- 服务端权威结算；
- 客户端预测和服务端拒绝；
- 自动化多进程测试；
- 网络模拟；
- 真实本地打包；
- 完整工程证据。

### 明确禁止

不得加入：

- EOS；
- Steam Online Subsystem；
- 登录、账号、数据库；
- 云服务器；
- 匹配服务；
- NAT 穿透；
- 商城；
- 背包；
- 装备；
- 剧情；
- 开放世界；
- 复杂 AI；
- 大量美术资产；
- 十几个技能；
- 自制通用网络协议；
- 再实现一套 RollbackLab；
- 无关的框架重构；
- 新建其他技术仓库。

所有场景、材质和视觉效果优先程序化生成或使用 UE 内置基础资源，不引入许可证不清晰的第三方资产。

---

## 5. 强制架构要求

核心逻辑必须使用 C++。蓝图只允许：

- 参数配置；
- DataAsset；
- 材质配置；
- 轻量表现连接；
- 不影响权威逻辑的原型或视觉调整。

至少包含以下 C++ 类型或等价职责：
```text
AAuthorityArenaGameMode
AAuthorityArenaGameState
AAuthorityArenaPlayerController
AAuthorityArenaPlayerState
AAuthorityArenaCharacter
UAuthorityArenaAbilitySystemComponent
UAuthorityArenaAttributeSet
UAuthorityArenaCombatComponent
UAuthorityArenaHealthComponent
UAuthorityArenaNetworkDiagnosticsSubsystem
UAuthorityArenaAutomationDriver
```

必须明确记录：

- 哪些对象只存在于服务端；
- 哪些对象复制到客户端；
- 哪些数据属于 PlayerState；
- 哪些状态属于 Character；
- 哪些行为是 Server RPC；
- 哪些行为是 Client RPC；
- 哪些事件适合 Multicast；
- 哪些状态使用 RepNotify；
- 哪些输入仅本地存在；
- 哪些预测可以回滚或纠正；
- 哪些结果只能由服务端决定。

---

## 6. PACT-00：仓库、基线与最小可运行项目

必须完成：

1. 创建公开 GitHub 仓库。
2. 建立最小 UE 5.8 C++ 项目。
3. 项目可由 Editor 打开。
4. Development Editor 构建通过。
5. Game Development 和 Game Shipping 目标可构建。
6. 创建程序化灰盒地图或自动生成地图脚本。
7. 无需手工在 Editor 中点击才能生成基本工程。
8. 创建基础测试和构建脚本。
9. 建立真实 `main` 基线提交。
10. 从真实 `origin/main` 创建功能分支：
    - `feat/authority-arena-0.1`
11. 创建 Draft PR。
12. `progress.md` 记录真实 SHA、命令、输出和待办。

PACT-00 未通过前，不得开始堆叠复杂 GAS 功能。

---

## 7. PACT-10：原生 UE 网络基础

实现并验证：

1. Listen Server 或 Dedicated Server 启动模式，优先支持 Dedicated Server；若本机引擎不含服务器目标所需组件，必须准确记录并使用可验证的替代模式。
2. 两个独立客户端连接同一服务端。
3. Character Movement 在真实多进程下复制。
4. 正确区分：
   - Authority；
   - Autonomous Proxy；
   - Simulated Proxy。
5. PlayerState 中保存玩家名、得分、死亡数和连接身份。
6. GameState 中保存比赛阶段、剩余时间和团队/回合状态。
7. 使用 Replicated Property 和 RepNotify。
8. 至少实现：
   - 一个可靠 Server RPC；
   - 一个不可靠高频 RPC 或说明为什么不需要；
   - 一个 Client RPC；
   - 一个有明确边界的 Multicast。
9. 不允许客户端直接写最终 Health、Score 或伤害结果。
10. 网络对象销毁、重生和断开连接后无悬挂引用。

必须提供真实 server + two clients 运行证据。

---

## 8. PACT-20：Gameplay Ability System

使用 UE Gameplay Ability System 完成三个能力：

### 8.1 Dash

- 客户端预测；
- Energy 消耗；
- Cooldown；
- 服务端确认；
- 服务端拒绝时有明确纠正；
- 禁止在死亡、眩晕或资源不足状态使用。

### 8.2 Projectile Attack

- 输入由客户端发起；
- 最终生成、命中和伤害由服务端权威决定；
- 使用 Gameplay Effect 改变 Health；
- 对重复、过快、无效目标和非法状态请求进行拒绝；
- 不信任客户端提交的最终伤害值。

### 8.3 Shield / Block

- 激活和持续状态通过 Gameplay Tags 表达；
- 消耗 Energy 或持续资源；
- 修改收到的伤害；
- 与攻击、死亡和冷却状态正确交互；
- 服务端和客户端 UI 状态一致。

必须正确使用：

- AbilitySystemComponent；
- AttributeSet；
- Gameplay Ability；
- Gameplay Effect；
- Gameplay Tags；
- Gameplay Cue 或等价表现边界；
- Prediction Key 或 UE 提供的预测机制；
- Ability Spec 生命周期。

不得仅通过普通 RPC 假装完成 GAS。

---

## 9. PACT-30：服务端权威与失败关闭

必须实现并测试：

1. 客户端不能自行设置 Health。
2. 客户端不能自行增加 Score。
3. 客户端不能伪造伤害数值。
4. 客户端不能超过合法攻击频率。
5. 客户端不能在无 Energy 时 Dash。
6. 客户端不能攻击无效或不可达目标。
7. 死亡角色不能施放能力。
8. 重生期间不能重复生成 Pawn。
9. RPC 参数和对象引用必须校验。
10. 非法输入必须：
    - 被拒绝；
    - 记录结构化原因；
    - 不污染服务端状态；
    - 不使进程崩溃；
    - 不导致客户端永久失同步。

这里的“安全”只针对游戏状态权威和请求验证，不宣称生产级反作弊、网络加密或恶意互联网环境防护。

---

## 10. PACT-40：真实多进程自动化

必须提供一个 PowerShell 7 入口，例如：
```powershell
./scripts/RunMultiplayerScenario.ps1
```

该脚本必须：

1. 自动发现 UE 5.8；
2. 使用唯一端口；
3. 启动服务端；
4. 启动两个独立客户端；
5. 等待 Ready/Connected 状态；
6. 执行固定脚本战斗；
7. 收集各进程结构化 JSON；
8. 验证：
   - 两个玩家均连接；
   - 移动发生；
   - Dash 预测和确认发生；
   - Projectile 攻击发生；
   - Shield 减伤发生；
   - 至少一次死亡；
   - 至少一次重生；
   - 服务端和客户端最终权威状态一致；
9. 有边界地退出全部自有进程；
10. 不结束无关进程；
11. 超时后失败关闭；
12. 保留失败日志。

至少测试以下配置：

- 本地近零延迟；
- 60 ms 延迟；
- 120 ms 延迟；
- 延迟 + 抖动；
- 延迟 + 少量丢包；
- 客户端中途断开；
- 服务端提前关闭；
- 第二客户端无法连接；
- 能力被服务端拒绝；
- 非法高频攻击；
- 错误目标；
- 超时/看门狗。

必须区分：

- 功能正确性；
- 网络时序观测；
- 调度相关指标；
- 不应被宣称为确定性的实时数据。

---

## 11. PACT-50：演示、诊断与可观察性

必须提供一个真正可看的灰盒演示：

- 两名玩家有明显不同的颜色或标识；
- Health/Energy/Cooldown 可见；
- 当前网络角色可见；
- 延迟、丢包、修正次数可见；
- Dash 预测、确认和拒绝有不同表现；
- Projectile 命中可见；
- Shield 状态可见；
- Death/Respawn/Score 可见；
- 服务端权威事件可在诊断面板中查看。

必须提供：

- 真实双客户端截图；
- 架构图；
- 复制流图；
- GAS 能力流图；
- 结构化网络报告；
- 可选 Network Insights Trace；
- 若 Trace 太大，仅保存在本地忽略目录，不提交、不上传 Release。

不得为了画面效果引入与工程目标无关的大量资产。

---

## 12. PACT-60：测试与本地交付验证

至少覆盖：

### 自动化测试

- 纯逻辑单元测试；
- Attribute/Effect 测试；
- Ability 激活和拒绝测试；
- RepNotify 状态测试；
- 服务端请求验证测试；
- Spawn/Destroy/Respawn 生命周期测试；
- 网络配置解析测试；
- 自动化多进程 E2E；
- 负面路径；
- 重复运行；
- 超时和进程清理。

### UE 验证

- Editor Development；
- Game Development；
- Game Shipping；
- UE Automation；
- 无头启动；
- 普通可交互启动；
- Win64 BuildCookRun；
- Cook；
- Stage；
- Pak；
- IoStore，若项目配置启用；
- Archive；
- 打包程序真实启动；
- 两客户端打包场景；
- clean checkout 或 clean worktree 重建；
- 产物 SHA-256；
- 源代码 SHA 绑定。

本地打包是强制的，但所有打包产物必须被忽略，不能上传至 GitHub Release。

---

## 13. PACT-70：作品集和面试材料

必须提供：

- `README.md`
- `README_ZH.md`，若时间允许
- `docs/ARCHITECTURE.md`
- `docs/NETWORK_MODEL.md`
- `docs/GAS_DESIGN.md`
- `docs/SERVER_AUTHORITY.md`
- `docs/MULTIPROCESS_TESTING.md`
- `docs/BUILD_SYSTEM.md`
- `docs/TESTING.md`
- `docs/KNOWN_LIMITATIONS.md`
- `docs/AI_ASSISTANCE.md`
- `docs/CODE_WALKTHROUGH.md`
- `docs/INTERVIEW_GUIDE.md`
- `docs/LIVE_CHANGE_DRILLS.md`
- `docs/ROLLBACK_VS_UE_REPLICATION.md`
- `docs/RELEASE_NOTES_0.1.0.md`

README 首屏必须快速回答：

1. 项目做什么；
2. 为什么对 UE/C++ 岗位有价值；
3. 如何运行；
4. 哪些证据来自真实多进程；
5. 哪些能力是预测的；
6. 哪些结果由服务端权威；
7. 有哪些明确限制；
8. AI 参与到什么程度。

Interview Guide 至少覆盖：

- UE 网络角色；
- Actor Ownership；
- RPC 调用条件；
- RepNotify；
- PlayerState 与 Character 的数据归属；
- GAS Prediction；
- 服务端拒绝；
- Character Movement；
- Reliable/Unreliable；
- Dormancy/Relevancy 基础；
- Rollback 与 UE Replication 的差异；
- 多进程测试；
- 进程清理；
- 最常见故障与定位方法。

Live Change Drills 至少提供三个可独立完成的小改动，例如：

1. 增加一个新的 Energy 恢复规则；
2. 修改 Dash 服务端拒绝条件；
3. 增加一个新的 RepNotify UI 状态。

---

## 14. P1 条件功能

只有以下全部通过后才允许进入 P1：

- 所有 P0；
- clean-source 完整重验；
- 本地打包；
- 真实双客户端；
- 全部负面路径；
- 文档；
- 独立审计；
- 没有 Blocker/High；
- 剩余时间和预算充足。

P1 可以从以下项目中最多选择一项：

- Iris 模式运行同一测试矩阵；
- Dormancy/Relevancy 与带宽对比；
- Network Prediction 插件集成；
- 简单服务器 rewind 命中验证；
- Gameplay Debugger 扩展。

不得同时铺开多个 P1，不得因 P1 破坏 P0。

---

## 15. Git 与 GitHub 工作流

1. 基线提交推送到 `main`。
2. 从真实 `origin/main` 创建：
   - `feat/authority-arena-0.1`
3. 创建 Draft PR。
4. 每个 PACT：
   - 先记录 RED；
   - 实现；
   - 编译；
   - 测试；
   - 真实运行；
   - 保存证据；
   - 更新 progress；
   - 提交；
   - 推送。
5. 不得把未测试代码写成已完成。
6. 不得重写或伪造失败历史。
7. 不得使用空洞的“全部完成”说明代替命令和结果。
8. 最终进行一次全新的独立只读审计。
9. 只修复审计确认存在的问题。
10. 修复后完整重新执行所有验证。
11. 若全部 P0、CI/本地验证、独立审计均通过：
    - 将 PR 标记为 Ready；
    - 使用正常 merge commit 合并；
    - 删除功能分支；
    - 创建 annotated tag `v0.1.0`；
    - 创建 GitHub Release。
12. Release 不上传任何自定义附件，只保留 GitHub 默认 Source code 归档。

---

## 16. AI 辅助与诚实边界

必须明确披露：

- 用户定义职业目标、选题、时间、范围、约束和验收方向；
- Codex GPT-5.6 Sol 完成架构细化、代码、测试、调试、打包、审计和文档；
- 用户没有独立手写本次交付代码；
- 不能把作品描述成完全手写；
- 面试前用户必须理解关键架构并完成至少一次 Live Change Drill。

不得通过删除 AI 披露来制造虚假作者身份。

---

## 17. 最终完成条件

只有同时满足以下条件，才能宣布 `AuthorityArena v0.1.0` 完成：

- 真实 UE 5.8 C++ 项目；
- 服务端 + 两个独立客户端；
- GAS 三个能力；
- 服务端权威；
- 客户端预测和拒绝；
- 网络模拟；
- 自动化多进程脚本；
- 正常和负面路径；
- UE Automation；
- 本地 Win64 打包；
- 打包程序真实运行；
- 无遗留自有进程；
- 文档完整；
- README 事实与代码一致；
- clean-source 重验；
- 独立审计无遗留 Blocker/High；
- PR 合并；
- annotated tag；
- source-only GitHub Release；
- GitHub Release 无任何自定义附件。

最终满足这些条件，不得创建虚假的正式 Release。持续自主工作，不要在只完成规划、只完成一项、只在 Debug 中运行或只创建 Draft PR 时提前结束。