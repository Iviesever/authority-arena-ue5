# AuthorityArena 0.1 技术蓝图

## 决策状态

用户要求持续自主工作，不得在只完成规划时停下。此前提出的视角问题没有收到人工选择，因此执行 Agent 采用本蓝图的推荐方案：第三人称、程序化灰盒、UE 原生复制与 GAS、共享纯 C++ 权威规则核心。该选择不改变产品合同，且在不破坏网络/测试接口的前提下可替换表现层。

## 方案比较

### 方案 A：UE 原生垂直切片 + 共享纯 C++ 权威规则核心（采用）

将无引擎依赖的请求判定、频率限制、资源门槛和网络场景配置放入 `AuthorityArenaCore`，由 MQB 与 UBT 编译同一份 C++；Actor、GAS、复制、UI 和生命周期放入 `AuthorityArena` UE 模块。竞技场和 HUD 由 C++ 运行时生成，不需要手工编辑资产。

优点：同时满足 MQB 证据、快速单测、UE 原生能力与作品集可解释性；失败边界清晰。代价：需要维护一个很薄的标准 C++/UE 类型适配边界。

### 方案 B：所有逻辑直接放入单个 UE 模块

优点：文件更少，直接使用 UE 类型。缺点：MQB 很难独立构建任何有意义的目标，单元测试启动成本高，权威判定与引擎生命周期耦合，不利于 PACT-30 和面试讲解，因此不采用。

### 方案 C：先抽成可复用 UE 插件再构建示例游戏

优点：模块化与复用叙事较强。缺点：插件打包、宿主兼容和额外 API 设计会扩大范围，挤压真实多进程、Cook/Package 与负面路径验证时间，违反 P0 优先和 YAGNI，因此不采用。

## 总体架构

```text
PowerShell Scenario Orchestrator
  -> server process + client 1 + client 2
  -> unique port / bounded PIDs / network emulation
  -> JSONL event streams + final snapshots
  -> verifier -> artifacts/report.json

UE AuthorityArena module
  -> GameMode (server-only match/lifecycle authority)
  -> GameState (replicated match phase/time)
  -> PlayerState (persistent ASC, attributes, score/deaths/identity)
  -> Character (movement, orientation, avatar lifecycle)
  -> Combat + Health components (request boundary and death state)
  -> Native GAS abilities/effects/tags (Dash, Projectile, Shield)
  -> replicated Projectile (server spawn/hit/damage)
  -> Diagnostics subsystem + Automation driver
  -> C++ HUD + runtime graybox arena

AuthorityArenaCore module
  -> engine-independent validation/config/report primitives
  -> compiled and tested by MQB
  -> also linked into the UE game through UBT
```

## 模块与文件边界

### `AuthorityArenaCore`

标准 C++20/23 可移植模块，不包含 Unreal 头文件。提供：

- `AuthorityRules`：Dash/attack/target/dead/respawn 判定，返回稳定的 `DecisionCode`。
- `RateLimiter`：基于显式单调时间输入的攻击节流，不读取全局时钟。
- `NetworkScenario`：近零延迟、60 ms、120 ms、抖动、丢包和故障注入的强类型配置与校验。
- `ReportModel`：事件/最终快照所需的纯数据结构与一致性判定。

它是 MQB 的可重复目标，也是 UE 自动化测试的共享规则源，避免“脚本测试了一套、游戏运行另一套”。

### `AuthorityArena`

- `AAuthorityArenaGameMode`：仅服务端存在；分配连接身份、生成/重生 Pawn、推进比赛阶段、记分、防止重复重生。
- `AAuthorityArenaGameState`：服务端写入，客户端复制；比赛阶段、剩余时间、场景运行 ID。
- `AAuthorityArenaPlayerController`：本地输入、可靠 `ServerRequestRespawn`、不可靠诊断采样、`ClientRequestRejected`。
- `AAuthorityArenaPlayerState`：跨 Pawn 生存；玩家显示名/连接 ID/Score/Deaths，以及 Mixed replication mode 的 ASC/AttributeSet。
- `AAuthorityArenaCharacter`：CharacterMovement 原生复制；在 `PossessedBy`/`OnRep_PlayerState` 初始化 GAS ActorInfo；保存仅与当前 Pawn 生命周期有关的死亡/屏蔽输入状态。
- `UAuthorityArenaAbilitySystemComponent`：能力输入映射、结构化激活/拒绝事件、预测确认/回滚观测。
- `UAuthorityArenaAttributeSet`：Health/MaxHealth/Energy/MaxEnergy/Damage 元属性，使用 RepNotify 与 GAS 宏。
- `UAuthorityArenaCombatComponent`：服务端目标/速率/状态校验与攻击请求边界，不接受客户端伤害数值。
- `UAuthorityArenaHealthComponent`：监听 Health 变化，服务端触发一次性死亡并通知 GameMode。
- `UAuthorityArenaNetworkDiagnosticsSubsystem`：进程级事件流、网络角色、延迟/丢包/修正计数和 JSONL 输出。
- `UAuthorityArenaAutomationDriver`：读取命令行场景和玩家身份，执行固定动作，写 Ready/Connected/Action/FinalSnapshot。
- `AAuthorityArenaProjectile`：只允许服务端生成和判定命中，通过 GameplayEffect Spec 施加固定伤害；有边界的 Multicast 只播放命中特效。
- `AAuthorityArenaHUD`：纯 C++ `DrawHUD` 显示 Health/Energy/Cooldown、网络角色、延迟/丢包/修正、Shield/Death/Score 和最近权威事件。
- `AAuthorityArenaWorldBuilder`：复制的默认子组件构成地面、墙体和出生标记，使用 Engine BasicShapes 与动态材质；无需第三方资产或编辑器点击。

## 网络所有权与数据归属

| 数据/行为 | 所有者与写入方 | 复制/调用方式 |
|---|---|---|
| MatchPhase、RemainingTime、RunId | 服务端 GameState | Replicated + RepNotify |
| ConnectionId、DisplayName、Score、Deaths | 服务端 PlayerState | Replicated + RepNotify |
| ASC、Health、Energy、Cooldown/状态 Tags | 服务端权威，拥有客户端可预测 GAS | PlayerState ASC Mixed replication；Attributes RepNotify |
| 移动/朝向 | Autonomous Proxy 输入，服务端验证 CharacterMovement | UE CharacterMovement replication/correction |
| Dash | 本地预测能力，服务端确认/拒绝 | GAS Prediction Key + predicted root motion |
| Projectile Attack | 客户端请求，服务端生成/命中/伤害 | GAS 激活 + 服务端 Actor spawn；无客户端伤害参数 |
| Shield | 客户端激活请求，服务端权威 Gameplay Tag/Effect | GAS replicated effect/tag |
| Respawn | 客户端可靠请求，服务端唯一决定 | Reliable Server RPC + Client rejection RPC |
| 视角/诊断采样 | 拥有客户端 | Unreliable Server RPC，仅观测，不影响权威状态 |
| 命中表现 | 服务端已确认结果 | 有边界的 Unreliable NetMulticast，仅表现 |
| UI 输入 | 本地 PlayerController | 不复制 |

最终 Health、Damage、Score、Death、Respawn 和能力合法性永远不能由客户端直接写入。

## Gameplay Ability System 设计

### Dash

`UGA_Dash` 使用 `LocalPredicted`，从输入方向或前向计算固定距离，以预测 Root Motion AbilityTask 驱动；Energy cost 与 cooldown 由 GAS effect/spec 管理。`Dead`、`Stunned`、`Shield.Active` 或能量不足时拒绝。服务器拒绝沿 GAS 预测回滚路径纠正位置/资源，并额外记录稳定拒绝码供诊断，不自行实现 rollback 协议。

### Projectile Attack

`UGA_ProjectileAttack` 接收输入但不接收最终伤害。服务器从权威角色位置、朝向、攻击间隔和候选目标状态生成 `AAuthorityArenaProjectile`；命中时构造固定伤害 GameplayEffect Spec。无效引用、不可达目标、死亡态和高频请求在改动状态之前拒绝。

### Shield / Block

`UGA_Shield` 应用持续 GameplayEffect 与 `State.Shield.Active` Tag，周期性或按固定间隔消耗 Energy；伤害执行前读取该 Tag 并应用固定减伤系数。Energy 耗尽、死亡或能力取消会移除效果。客户端 UI 从复制的 Tag/Attribute 得出状态，不维护独立真相。

## 灰盒演示与地图

默认运行 `/Engine/Maps/Entry`，全局 C++ GameMode 在服务器生成 `AAuthorityArenaWorldBuilder`，其 C++ 默认子组件在所有实例上形成相同碰撞几何；GameMode 使用固定出生变换生成玩家，不依赖手工放置的 PlayerStart。第三人称相机采用 SpringArm + Camera，角色用 Engine BasicShapes 与动态颜色区分，Projectile 与 Shield 使用简单网格/材质和短暂调色，不引入外部资产。

如果验证证明 Engine Entry 不适合作为打包 GameDefaultMap，则使用 Editor target 中的 C++ commandlet 自动生成 `/Game/Maps/Arena.umap`；该决策只由真实 Cook/启动结果触发并记录，不通过手工点击补救。

## 多进程数据流

1. `RunMultiplayerScenario.ps1` 发现 UE 5.8，创建唯一 `RunId`、TCP/UDP 端口和隔离 artifacts 目录。
2. 脚本记录启动前相关进程快照，只保存自己启动的 PID/启动时间/可执行路径。
3. 启动独立 server（优先 Server target；Launcher 分发不支持时使用 Game/Editor `-server -nullrhi`），等待 JSONL `ServerReady`。
4. 启动两个独立客户端，传入 `PlayerId`、场景名、RunId 和报告路径，等待两条 `ClientConnected`。
5. AutomationDriver 按复制的 MatchStart 时间执行固定战斗，不依赖脚本向窗口发送按键。
6. 每个进程写 append-only JSONL；服务器写权威事件，客户端写预测、确认、拒绝、修正和最终观察快照。
7. 脚本等待终态或 watchdog，验证合同矩阵，生成包含源 SHA、命令行、配置和结果的 JSON 报告。
8. `finally` 块只终止本次记录且身份仍匹配的 PID，保留失败日志并复核无自有遗留进程。

## 失败关闭策略

- 所有请求先验证 authority、owner、对象有效性、生命周期、资源、冷却、速率和可达性，之后才修改状态。
- 拒绝结果使用稳定枚举码与结构化字段，不把非确定性日志文本作为唯一测试接口。
- GameplayEffect 修改后由服务端钳制 Health/Energy；NaN、负资源和越界值 fail closed。
- 重生令牌以 PlayerState/Controller 为单位，确保每次死亡至多一个待执行 timer；断开时取消。
- orchestrator 的连接/就绪/动作/终态均有独立超时；超时不当作通过，脚本以非零状态退出。
- Dedicated Server 若因 Launcher 引擎分发缺少目标组件失败，保存精确 UBT 输出，并切换到合同允许的独立 listen/game server 进程；不宣称 dedicated 已通过。
- MQB 仅在公共 CLI 能准确表达并产生同等产物时拥有目标；UHT、UBT target、Cook/Stage/Pak/IoStore/Archive 的回退证据写入 `BUILD_SYSTEM.md`。

## 测试策略

### RED/GREEN 单元层

- MQB 先构建并运行 `AuthorityArenaCoreTests.exe`：规则决策、速率边界、网络配置和最终快照一致性。
- UE Automation 覆盖 Attribute/Effect、能力激活/拒绝、RepNotify helper、服务端参数校验、Spawn/Destroy/Respawn 生命周期。
- 每个 PACT 在生产实现前保存相应失败测试或失败命令输出；同一命令在实现后转绿。

### 集成与真实进程层

- Development Editor、Game Development、Game Shipping 分别构建。
- 无头 server + 两个 `-game` 客户端执行 baseline，并运行 60 ms、120 ms、jitter、loss 与全部负面/故障场景。
- 普通可交互双客户端运行用于截图，截图只能来自当前源 SHA。
- RunUAT BuildCookRun 执行 Build/Cook/Stage/Pak/Archive；若启用 IoStore 则验证 `.utoc/.ucas`。
- 打包 exe 真实启动，并执行打包 server + two clients 场景。
- 从干净 clone/worktree 对同一提交完整重建并复验；计算 archive 文件树与主 exe 的 SHA-256。

### 发布门禁

- CI 与本地门禁均通过，README/Release Notes 中的每条“通过”均能映射到 artifacts 证据。
- 新的只读审计 Agent 按 `ACCEPTANCE_MATRIX.md` 逐项核验；任何 Blocker/High 必须修复并完整重跑。
- 仅当全部 P0 通过，Draft PR 才转 Ready 并使用 merge commit；之后创建 annotated tag 和 GitHub Release。
- Release API 必须证明自定义 assets 数组为空；只保留 GitHub 自动源代码归档。

## Git 工作流

1. 在不存在目标仓库的已验证状态下创建本地 Git 与公开 GitHub 仓库。
2. `main` 只包含 PACT-00 基线并推送；从真实 `origin/main` 创建 `feat/authority-arena-0.1`。
3. 创建 Draft PR；每个 PACT 遵循 RED -> change -> build/test/run -> evidence -> progress -> commit -> push。
4. 不 force-push、不重写失败历史、不上传 `Artifacts/`、二进制或大证据包。
5. P0 完成且审计通过后 Ready、merge commit、删除功能分支、annotated tag、source-only Release。

## 计划拆分

- PACT-00：治理合同、Git/GitHub 基线、UE 最小项目、MQB 能力实验、三个构建目标、程序化场景基线。
- PACT-10：网络角色、PlayerState/GameState、移动、RPC/RepNotify、真实 server + two clients。
- PACT-20：ASC/attributes/tags/effects 与三项 GAS 能力。
- PACT-30：权威验证、拒绝码、异常请求与生命周期防护。
- PACT-40：PowerShell 多进程编排、JSONL、网络/故障矩阵与有界清理。
- PACT-50：C++ HUD、可视反馈、截图/图示和结构化网络报告。
- PACT-60：完整自动化、构建配置、BuildCookRun、打包运行、clean-source 重验和哈希绑定。
- PACT-70：作品集、面试、AI 披露、Release Notes、审计修复、合并/tag/Release。

## 蓝图自我审查

- 占位符：无 `TBD`、模糊待补章节或未选择的核心方案。
- 一致性：ASC/Attributes 位于 PlayerState，与跨重生持久状态和 Mixed replication 一致；Character 只承担当前 Pawn 生命周期。
- 范围：所有 PACT 均独立形成可运行/可测试检查点；P1 不在实现计划中，除非 P0 全绿且时间充足。
- 歧义：第三人称已作为执行 Agent 在自主授权下的明确选择；Dedicated Server 与 map asset 均使用真实验证驱动的单一路径回退。
- 合同覆盖：架构、数据流、错误处理、测试、构建、证据、GitHub 发布和 AI 诚实边界均已映射。
