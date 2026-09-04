# AuthorityArena 0.1 实现计划

> **针对 Agent 工作者：** 必需的子 Skill：使用 `executing-tasks` 在同一会话逐任务执行；同一仓库始终只有主 Agent 写入。步骤使用复选框追踪。

**目标：** 交付并发布一个具有真实 UE 5.8 C++/GAS 服务端权威、双客户端多进程、网络模拟、自动化证据和本地 Win64 打包验证的 AuthorityArena v0.1.0。

**架构：** 无引擎依赖的 `AuthorityArenaCore` 封装权威规则和场景数据，由 MQB 与 UBT 共同编译；`AuthorityArena` UE 模块封装复制、GAS、角色生命周期、诊断、自动化和纯 C++ 灰盒表现。PowerShell 7 只编排本轮拥有的进程并验证各进程 JSONL 证据。

**技术栈：** Unreal Engine 5.8.0、C++20、GameplayAbilities/GameplayTags/GameplayTasks、MSVC、MQB 5.4.0、PowerShell 7.6、Git/GitHub CLI。

## 全局约束

- 核心逻辑不得依赖蓝图实现。
- 同一仓库只能由一个写入 Agent；其他 Agent 只能只读审计。
- MQB 能正确、重复构建的目标必须优先使用 MQB；UE 专用 UHT/UBT/Cook/Stage/Pak/Archive 仅在证据支持时回退到官方工具。
- PACT-00 通过前不得堆叠复杂 GAS 功能。
- 任何打包程序、二进制、PDB、Cook/Stage/Pak、日志包或 Trace 不得提交或上传 GitHub Release。
- 只有所有 P0、clean-source、打包双客户端、独立审计无 Blocker/High 全部通过时才发布 `v0.1.0`；否则维持 Draft Alpha/WIP。
- 截止时间：2026-09-05 15:30（UTC+8）。

---

### Task 1：治理合同、本地 Git 与公开远程基线

**文件：**

- 创建：`AGENTS.md`
- 创建：`.agents/AGENTS.md`
- 创建：`.gitignore`
- 创建：`docs/PRODUCT_CONTRACT.md`
- 创建：`docs/ARCHITECTURE.md`
- 创建：`docs/ACCEPTANCE_MATRIX.md`
- 创建：`tasks/20260904-224034-authority-arena-0.1/progress.md`
- 创建：`tasks/20260904-224034-authority-arena-0.1/handoff.md`
- 保留：`tasks/20260904-224034-authority-arena-0.1/goal-objective.md`
- 保留：`tasks/20260904-224034-authority-arena-0.1/issue.md`
- 保留：`tasks/20260904-224034-authority-arena-0.1/implementation_plan.md`
- 保留：`tasks/20260904-224034-authority-arena-0.1/task.md`

**接口：**

- 消费：最高产品合同与 2026-09-04 只读状态审计。
- 产出：全仓库写入/证据规则、逐需求验收矩阵、可追踪进度日志、`main` 基线和 `origin/main`。

- [x] **Step 1：记录 PACT-00 RED**

运行：

```powershell
pwsh -NoProfile -File .\scripts\Verify-Contracts.ps1
```

预期：在脚本尚不存在时以非零退出，作为“治理与验收入口缺失”的 RED 证据写入 `progress.md`，不得记为测试通过。

- [x] **Step 2：创建不可绕过的规则与矩阵**

`AGENTS.md` 明确：先读 `.agents/AGENTS.md` 和当前任务合同；单写入 Agent；TDD；证据先于声明；禁止 Release 自定义 assets；只清理由本轮启动并核验身份的 PID。

`.agents/AGENTS.md` 明确每个 PACT 的顺序：

```text
RED -> minimum implementation -> build/test -> real run -> evidence -> progress -> commit -> push
```

`ACCEPTANCE_MATRIX.md` 为每个编号要求记录：状态、验证命令、证据路径、源 SHA、失败/限制，默认状态必须为 `NOT RUN`，不能预填 PASS。

- [x] **Step 3：建立安全忽略边界**

`.gitignore` 至少忽略：`.vs/`、`.idea/`、`Binaries/`、`Build/`、`DerivedDataCache/`、`Intermediate/`、`Saved/`、`Artifacts/`、`.mqb/`、`*.sln`、IDE 用户文件；不得忽略 `Config/`、`Content/`、`Source/`、`docs/images/` 和精简 JSON 示例。

- [x] **Step 4：初始化本地和公开仓库**

运行：

```powershell
git init -b main
git config user.name
git config user.email
gh repo create Iviesever/authority-arena-ue5 --public --source . --remote origin
git remote -v
gh repo view Iviesever/authority-arena-ue5 --json nameWithOwner,visibility,defaultBranchRef,url
```

预期：远程为 PUBLIC、owner 为 `Iviesever`，不包含自动生成 README 或未知提交。

- [x] **Step 5：实现并运行合同验证器**

创建 `scripts/Verify-Contracts.ps1`，以显式路径数组检查全部强制治理文件、目标合同 SHA-256 非空、`.gitignore` 禁止产物规则和验收矩阵 PACT 标题；缺一项即 `throw`。

运行：

```powershell
pwsh -NoProfile -File .\scripts\Verify-Contracts.ps1
```

预期：`PASS contracts`。

- [x] **Step 6：提交并推送真实 main 基线**

```powershell
git add AGENTS.md .agents .gitignore docs tasks scripts/Verify-Contracts.ps1
git commit -m "chore: establish AuthorityArena delivery contract"
git push -u origin main
git rev-parse HEAD
git ls-remote origin refs/heads/main
```

预期：本地 HEAD 与 `origin/main` SHA 相同；SHA 和命令输出写入 `progress.md`。

---

### Task 2：AuthorityArenaCore TDD 与 MQB 能力验证

**文件：**

- 创建：`Source/AuthorityArenaCore/AuthorityArenaCore.Build.cs`
- 创建：`Source/AuthorityArenaCore/Public/AuthorityRules.h`
- 创建：`Source/AuthorityArenaCore/Public/NetworkScenario.h`
- 创建：`Source/AuthorityArenaCore/Public/ReportModel.h`
- 创建：`Source/AuthorityArenaCore/Private/AuthorityRules.cpp`
- 创建：`Source/AuthorityArenaCore/Private/NetworkScenario.cpp`
- 创建：`Source/AuthorityArenaCore/Private/ReportModel.cpp`
- 创建：`Tests/AuthorityArenaCoreTests.cpp`（位于 UE 模块目录外，避免 UBT 收集独立 `main`）
- 创建：`scripts/Test-Core.ps1`
- 创建：`docs/BUILD_SYSTEM.md`

**接口：**

- 产出：

```cpp
enum class DecisionCode : std::uint8_t {
  Allowed, NotAuthority, NotOwner, Dead, Stunned, InsufficientEnergy,
  OnCooldown, RateLimited, InvalidTarget, TargetOutOfRange, RespawnPending
};

struct AbilityRequest { bool authority; bool owner; bool alive; bool stunned;
  double now_seconds; double last_use_seconds; double cooldown_seconds;
  double energy; double energy_cost; };

DecisionCode ValidateAbilityRequest(const AbilityRequest&) noexcept;
DecisionCode ValidateAttackTarget(bool target_valid, bool target_alive,
                                  double squared_distance,
                                  double max_squared_distance) noexcept;
NetworkScenario ParseScenarioName(std::string_view);
ConsistencyResult CompareSnapshots(const FinalSnapshot&, const FinalSnapshot&) noexcept;
```

- [x] **Step 1：编写失败的纯 C++ 测试**

测试至少逐项断言：允许请求、非 owner、死亡、眩晕、能量不足、冷却边界、攻击频率边界、空/死亡/越距目标、重复重生；解析 `baseline/lag60/lag120/jitter/loss`；客户端/服务端快照不一致必须失败。

- [x] **Step 2：用 MQB 观察 RED**

```powershell
mqb run Tests/AuthorityArenaCoreTests.cpp `
  Source/AuthorityArenaCore/Private/AuthorityRules.cpp `
  Source/AuthorityArenaCore/Private/NetworkScenario.cpp `
  Source/AuthorityArenaCore/Private/ReportModel.cpp `
  /ISource/AuthorityArenaCore/Public --std 20 /WX
```

预期：因接口/实现缺失而编译或链接失败；保存精确输出。

- [x] **Step 3：实现最小规则核心**

实现必须纯函数优先、无全局时钟、无 Unreal 类型、无异常依赖。校验顺序固定，确保非法请求在状态变化前得到稳定拒绝码。

- [x] **Step 4：验证 MQB GREEN 与增量构建**

连续两次运行 Step 2 命令，第二次附加 `--timings=json`；预期测试进程返回 0、无 warning，第二次显示可解释的 cache/incremental 行为。MQB 5.4 自有默认 `/W3`，因此使用 `/WX` 而不额外覆盖为 `/W4`，避免 MSVC `D9025` 命令行警告。记录 MQB 版本从 `mqb --help` 首行取得，因为 `--version` 实测不受支持。

- [x] **Step 5：有边界地探测 MQB 的 UE 能力**

在 UE 模块最小源文件产生后仅执行一次代表性 `mqb build`，验证 UHT generated header、Engine include、宏和链接语义。若失败，保存命令与第一根因，不循环伪造 include 参数；在 `BUILD_SYSTEM.md` 划定 MQB 负责 Core、UBT 负责 UE Target/UHT、RunUAT 负责 Cook/Package。

- [x] **Step 6：提交并推送**

```powershell
git add Source/AuthorityArenaCore scripts/Test-Core.ps1 docs/BUILD_SYSTEM.md tasks/*/progress.md
git commit -m "test: establish MQB authority rules core"
git push
```

---

### Task 3：PACT-00 最小 UE 5.8 项目与程序化灰盒

**文件：**

- 创建：`AuthorityArena.uproject`
- 创建：`Config/DefaultEngine.ini`
- 创建：`Config/DefaultGame.ini`
- 创建：`Config/DefaultInput.ini`
- 创建：`Source/AuthorityArena.Target.cs`
- 创建：`Source/AuthorityArenaEditor.Target.cs`
- 创建：`Source/AuthorityArenaServer.Target.cs`
- 创建：`Source/AuthorityArena/AuthorityArena.Build.cs`
- 创建：`Source/AuthorityArena/Public/AuthorityArena.h`
- 创建：`Source/AuthorityArena/Private/AuthorityArena.cpp`
- 创建：`Source/AuthorityArena/Public/Game/AuthorityArenaGameMode.h`
- 创建：`Source/AuthorityArena/Private/Game/AuthorityArenaGameMode.cpp`
- 创建：`Source/AuthorityArena/Public/Character/AuthorityArenaCharacter.h`
- 创建：`Source/AuthorityArena/Private/Character/AuthorityArenaCharacter.cpp`
- 创建：`Source/AuthorityArena/Public/World/AuthorityArenaWorldBuilder.h`
- 创建：`Source/AuthorityArena/Private/World/AuthorityArenaWorldBuilder.cpp`
- 创建：`scripts/Find-UE58.ps1`
- 创建：`scripts/Build.ps1`
- 创建：`scripts/Run-Smoke.ps1`

**接口：**

- `Find-UE58.ps1` 返回包含 `Root`、`EditorCmd`、`BuildBat`、`RunUat`、`Version` 的对象，版本必须为 `5.8.x`。
- `Build.ps1 -Target Editor|Game|Server -Configuration Development|Shipping` 先运行 MQB Core tests，再调用有证据边界的 UBT。
- `AAuthorityArenaGameMode::ChooseSpawnTransform(int32 PlayerIndex) const` 返回固定出生点。
- `AAuthorityArenaWorldBuilder` 的 C++ 默认子组件构建 floor/四墙/中心标记。

- [x] **Step 1：编写 PACT-00 RED 检查**

`scripts/Test-ProjectStructure.ps1` 检查 `.uproject` EngineAssociation `5.8`、目标文件、模块依赖、默认 map/GameMode、禁止蓝图权威逻辑和 required C++ 类型。首次运行应因项目文件缺失而失败。

- [x] **Step 2：创建最小项目和 C++ 类型**

`.uproject` 启用 `GameplayAbilities`、`GameplayTags`、`GameplayTasks`；主模块依赖 `AuthorityArenaCore`。Character 构造 Capsule、CharacterMovement、SpringArm、Camera 与可着色 BasicShape；GameMode 生成 WorldBuilder 与 Pawn，不依赖 Editor 点击。

- [x] **Step 3：验证 Editor 构建**

```powershell
& 'D:\program\UnrealEngine\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
  AuthorityArenaEditor Win64 Development `
  '-Project=D:\program\authority-arena-ue5\AuthorityArena.uproject' -WaitMutex -NoHotReload
```

预期：exit 0，`Binaries/Win64/UnrealEditor-AuthorityArena.dll` 存在。

- [x] **Step 4：无头打开与普通可交互启动**

```powershell
& 'D:\program\UnrealEngine\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  '.\AuthorityArena.uproject' -game -nullrhi -unattended -nosplash -stdout -FullStdOutLogOutput `
  -ExecCmds='quit' -log='.\Artifacts\pact00\headless.log'
```

然后以可见窗口启动 `UnrealEditor.exe .\AuthorityArena.uproject -game -windowed -ResX=960 -ResY=540`，等待 Ready marker 后只结束记录的该 PID。预期两者均无 fatal/crash。

- [x] **Step 5：构建 Development Game 与 Shipping Game**

运行 `Build.ps1 -Target Game -Configuration Development` 和 `Build.ps1 -Target Game -Configuration Shipping`；预期各自 exit 0。

- [x] **Step 6：探测 Dedicated Server target**

运行 `Build.ps1 -Target Server -Configuration Development`。成功则将 Server target 作为后续首选；若 Launcher engine 报 distribution 不支持 Server target，保存精确日志并锁定 `Game -server -nullrhi` 替代路径，不把失败写成通过。

- [x] **Step 7：PACT-00 GREEN、基线提交与功能分支**

运行结构检查、Core tests、三个可用 target、headless/interactive smoke；更新矩阵。提交到 `main` 并推送后，从真实 `origin/main` 创建：

```powershell
git switch -c feat/authority-arena-0.1 origin/main
git push -u origin feat/authority-arena-0.1
gh pr create --draft --base main --head feat/authority-arena-0.1 `
  --title 'feat: AuthorityArena 0.1 vertical slice' --body-file .\docs\PR_BODY.md
```

---

### Task 4：PACT-10 原生复制、网络角色与生命周期

**文件：**

- 创建：`Source/AuthorityArena/Public/Game/AuthorityArenaGameState.h`
- 创建：`Source/AuthorityArena/Private/Game/AuthorityArenaGameState.cpp`
- 创建：`Source/AuthorityArena/Public/Player/AuthorityArenaPlayerController.h`
- 创建：`Source/AuthorityArena/Private/Player/AuthorityArenaPlayerController.cpp`
- 创建：`Source/AuthorityArena/Public/Player/AuthorityArenaPlayerState.h`
- 创建：`Source/AuthorityArena/Private/Player/AuthorityArenaPlayerState.cpp`
- 创建：`Source/AuthorityArena/Public/Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h`
- 创建：`Source/AuthorityArena/Private/Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.cpp`
- 创建：`Source/AuthorityArena/Private/Tests/ReplicationTests.cpp`
- 创建：`docs/NETWORK_MODEL.md`

**接口：**

- GameState：`FName MatchPhase`、`int32 RemainingSeconds`、`FGuid ScenarioRunId`，全部服务端写入 + RepNotify。
- PlayerState：`FString ConnectionId`、`FString DisplayName`、`int32 Score`、`int32 Deaths`，服务端 mutator + RepNotify。
- Reliable Server RPC：`ServerRequestRespawn()`；Client RPC：`ClientRequestRejected(FName Action, EAuthorityRejectionCode Code)`。
- Unreliable Server RPC：`ServerReportViewSample(FRotator ViewRotation, uint32 Sequence)`，只写诊断、限速且永不影响命中/伤害。
- Multicast 保留给已确认表现事件，不复制权威状态。

- [x] **Step 1：编写失败 UE Automation 测试**

覆盖字段默认值/服务端 mutator、RepNotify helper 更新观察状态、重复重生门闩、无效 view sample 不污染 gameplay state。运行：

```powershell
UnrealEditor-Cmd.exe AuthorityArena.uproject -unattended -nullrhi `
  -ExecCmds='Automation RunTests AuthorityArena.Network;Quit' -TestExit='Automation Test Queue Empty'
```

预期：新测试因类型/行为缺失而失败。

- [x] **Step 2：实现最小复制与所有权边界**

使用 `DOREPLIFETIME`，所有写 API 在 `HasAuthority()` 不成立时 fail closed；Character 的移动只通过 CharacterMovement；PlayerState 保存跨 respawn 数据，Character 不复制分数真相。

- [x] **Step 3：运行 UE Automation GREEN**

重复 Step 1，预期全部 `AuthorityArena.Network` 通过且进程 exit 0。

- [x] **Step 4：真实 server + two clients 连接/移动证据**

先实现最小 `RunMultiplayerScenario.ps1 -Scenario ConnectionMovement`，使用唯一端口启动一个 server 和两个独立 client；验证三个不同 PID、两个连接 ID、每个客户端均看到 Autonomous + Simulated Proxy、权威位置一致、断开后引用清理。

- [x] **Step 5：更新证据并提交**

保存精简 `docs/examples/pact10-connection-report.json` 和真实日志路径（忽略）；更新 `NETWORK_MODEL.md`、矩阵、progress，提交 `feat: add native multiplayer replication foundation` 并 push。

---

### Task 5：PACT-20 原生 GAS、Attributes 与三项能力

**文件：**

- 创建：`Source/AuthorityArena/Public/Ability/AuthorityArenaAbilitySystemComponent.h`
- 创建：`Source/AuthorityArena/Private/Ability/AuthorityArenaAbilitySystemComponent.cpp`
- 创建：`Source/AuthorityArena/Public/Ability/AuthorityArenaAttributeSet.h`
- 创建：`Source/AuthorityArena/Private/Ability/AuthorityArenaAttributeSet.cpp`
- 创建：`Source/AuthorityArena/Public/Ability/AuthorityArenaGameplayTags.h`
- 创建：`Source/AuthorityArena/Private/Ability/AuthorityArenaGameplayTags.cpp`
- 创建：`Source/AuthorityArena/Public/Ability/GA_Dash.h`
- 创建：`Source/AuthorityArena/Private/Ability/GA_Dash.cpp`
- 创建：`Source/AuthorityArena/Public/Ability/GA_ProjectileAttack.h`
- 创建：`Source/AuthorityArena/Private/Ability/GA_ProjectileAttack.cpp`
- 创建：`Source/AuthorityArena/Public/Ability/GA_Shield.h`
- 创建：`Source/AuthorityArena/Private/Ability/GA_Shield.cpp`
- 创建：`Source/AuthorityArena/Public/Ability/AuthorityArenaEffects.h`
- 创建：`Source/AuthorityArena/Private/Ability/AuthorityArenaEffects.cpp`
- 创建：`Source/AuthorityArena/Public/Combat/AuthorityArenaProjectile.h`
- 创建：`Source/AuthorityArena/Private/Combat/AuthorityArenaProjectile.cpp`
- 创建：`Source/AuthorityArena/Private/Tests/AbilityTests.cpp`
- 创建：`docs/GAS_DESIGN.md`

**接口：**

- ASC 位于 PlayerState，`ReplicationMode = Mixed`；Character 在服务端 possession 与客户端 `OnRep_PlayerState` 调用 `InitAbilityActorInfo(PlayerState, Character)`。
- Attributes：Health/MaxHealth/Energy/MaxEnergy/IncomingDamage，Health/Energy 使用 RepNotify。
- Tags：`Ability.Dash`、`Ability.Attack`、`Ability.Shield`、`State.Dead`、`State.Stunned`、`State.Shield.Active`、`Cooldown.Dash`、`Cooldown.Attack`。
- 三个 `UGameplayAbility` 子类由 C++ grant；Dash `LocalPredicted`，Attack 服务端生成 Projectile，Shield 以 replicated effect/tag 表达。

- [x] **Step 1：RED—Attribute/Effect 与能力拒绝测试**

先写测试：damage 降低 Health 并钳制；shield 减伤；Dash 消耗 Energy/应用 cooldown；不足资源、dead/stunned 拒绝；Attack 不接受客户端 damage；ability spec 在 respawn 后存在且不重复 grant。

- [x] **Step 2：验证 RED**

运行 `AuthorityArena.GAS` 测试组，确认因生产类型/行为缺失而失败，而不是测试注册错误。

- [x] **Step 3：GREEN—ASC/Attributes/Tags/Effects**

实现复制属性与 `GAMEPLAYATTRIBUTE_REPNOTIFY`；`PostGameplayEffectExecute` 只在 authority 应用 IncomingDamage/Shield modifier 并钳制；native tags 启动注册；effects 在 C++ 构造且不依赖 GameplayEffect 蓝图。

- [x] **Step 4：GREEN—Dash**

使用 `LocalPredicted` 和支持网络预测的 Root Motion AbilityTask；cost/cooldown 由 GAS commit；拒绝时依赖 Prediction Key 回滚并记录 predicted/confirmed/rejected 三种事件。

- [x] **Step 5：GREEN—Projectile Attack**

服务器从权威 transform 和固定 tuning 生成复制 Projectile；碰撞仅服务器施加 GameplayEffect damage；`NetMulticast` 只播放已确认 impact 表现。

- [x] **Step 6：GREEN—Shield**

GameplayEffect/Tag 表达持续状态，固定周期消耗 Energy；能量耗尽/死亡/取消时移除。UI 只读取 ASC/Attributes。

- [x] **Step 7：运行单元、UE Automation 和真实三进程能力场景**

验证 Dash 预测+确认、至少一次 Dash 拒绝、Projectile 命中、Shield 减伤、Death、Respawn、Score，以及三进程最终状态一致。

- [x] **Step 8：文档、证据、提交**

更新 `GAS_DESIGN.md` 的 Prediction Key、Spec 生命周期、cost/cooldown/Tag 流；提交 `feat: implement predicted GAS combat abilities` 并 push。

---

### Task 6：PACT-30 服务端权威、失败关闭与负面路径

**文件：**

- 创建：`Source/AuthorityArena/Public/Combat/AuthorityArenaCombatComponent.h`
- 创建：`Source/AuthorityArena/Private/Combat/AuthorityArenaCombatComponent.cpp`
- 创建：`Source/AuthorityArena/Public/Combat/AuthorityArenaHealthComponent.h`
- 创建：`Source/AuthorityArena/Private/Combat/AuthorityArenaHealthComponent.cpp`
- 创建：`Source/AuthorityArena/Public/Net/AuthorityRejectionCode.h`
- 创建：`Source/AuthorityArena/Private/Tests/AuthorityValidationTests.cpp`
- 创建：`docs/SERVER_AUTHORITY.md`

**接口：**

- `FValidationResult ValidateDashRequest(...)`、`ValidateAttackRequest(...)`、`ValidateRespawnRequest(...)` 适配 Core `DecisionCode`，返回稳定拒绝码。
- `HandleServerDeath(AController* InstigatorController)` 幂等；`ScheduleRespawn(AAuthorityArenaPlayerController&)` 每个 controller 最多一个 timer。

- [x] **Step 1：RED—逐项负面测试**

分别测试客户端 Health/Score 写入、伪造伤害、攻击过快、无 Energy Dash、错误/越距目标、死亡施法、重复 respawn、无效/非 owner RPC 引用；每项断言拒绝码、状态不变、无 crash。

- [x] **Step 2：验证 RED**

运行 `AuthorityArena.Authority`，保存每个缺失防护的失败断言。

- [x] **Step 3：实现最小 fail-closed 校验**

所有 RPC/ability 路径在任何 mutation 前完成 Core 判定；客户端提供的数值只可作为不可信观测且不能进入 damage/score 计算；死亡/respawn 使用幂等门闩与弱引用 timer。

- [x] **Step 4：GREEN 与真实攻击注入**

运行 UE Automation，再运行多进程 `RejectedAbility`、`AttackFlood`、`InvalidTarget` 场景；服务器报告必须含稳定拒绝码，权威快照不变，客户端最终重新一致。

- [x] **Step 5：提交**

更新威胁边界与“不宣称生产级反作弊/加密”，提交 `feat: enforce server-authoritative request validation` 并 push。

---

### Task 7：PACT-40 完整多进程编排与网络/故障矩阵

**文件：**

- 创建：`Source/AuthorityArena/Public/Automation/AuthorityArenaAutomationDriver.h`
- 创建：`Source/AuthorityArena/Private/Automation/AuthorityArenaAutomationDriver.cpp`
- 创建：`Source/AuthorityArena/Public/Diagnostics/AuthorityArenaEvent.h`
- 创建：`scripts/RunMultiplayerScenario.ps1`
- 创建：`scripts/Invoke-NetworkMatrix.ps1`
- 创建：`scripts/Verify-ScenarioReport.ps1`
- 创建：`scripts/tests/RunMultiplayerScenario.Tests.ps1`
- 创建：`docs/MULTIPROCESS_TESTING.md`

**接口：**

- CLI：`RunMultiplayerScenario.ps1 -Build Editor|Packaged -Scenario <name> -TimeoutSeconds <bounded> -KeepFailedLogs`。
- JSONL event fields：`schemaVersion, runId, processRole, pid, sequence, utc, event, playerId, authorityRole, payload`。
- final report fields：source SHA、UE/MQB/toolchain versions、launch args（去敏）、port、scenario、assertions、process exit codes、cleanup result、final snapshots。

- [x] **Step 1：RED—Pester/PowerShell harness tests**

用受控短命测试进程验证唯一端口、Ready timeout、第二客户端失败、server 提前退出、client 中途退出、watchdog、PID 身份校验和 finally 清理。测试不得结束审计前已存在的进程。

- [x] **Step 2：实现有界进程拥有权**

记录 `Id/StartTime/Path/RunId`，清理前四项再次匹配；只对本轮集合调用 `Stop-Process -Id`，并等待退出。任何身份不匹配都记录为 cleanup failure，而不是扩大终止范围。

- [x] **Step 3：实现 AutomationDriver 与 JSONL**

驱动器基于复制 MatchStart/PlayerId 排程移动、Dash、Shield、Attack、Death/Respawn；每个进程单独文件，按 sequence 追加。诊断 subsystem 串行化写入，flush 终态。

- [x] **Step 4：baseline 功能 GREEN**

验证两个玩家连接、移动、Dash predicted/confirmed、Projectile、Shield、Death、Respawn、Score、三方最终快照一致。

- [x] **Step 5：网络矩阵 GREEN**

依次运行 baseline、lag60、lag120、jitter、loss；功能断言必须相同。延迟、修正次数和调度时长只标记为观测值，不宣称确定性。

- [x] **Step 6：故障矩阵 GREEN**

运行 client disconnect、server shutdown、second client fail、rejected ability、attack flood、invalid target、watchdog；每项期望非零/特定终态必须由 verifier 明确解释，保留失败日志。

- [x] **Step 7：重复运行与清理证明**

baseline 连续至少三次；每次端口不同、runId 不同、结果通过；最后对本轮 PID 集合逐个证明不存在，不使用名称级 kill。

- [x] **Step 8：提交**

提交精简示例报告与文档，Artifacts 保持 ignored；提交 `test: add real multiprocess network matrix` 并 push。

---

### Task 8：PACT-50 C++ HUD、反馈与当前 SHA 视觉证据

**文件：**

- 创建：`Source/AuthorityArena/Public/UI/AuthorityArenaHUD.h`
- 创建：`Source/AuthorityArena/Private/UI/AuthorityArenaHUD.cpp`
- 创建：`docs/images/authority-arena-two-clients.png`
- 创建：`docs/images/architecture.png`
- 创建：`docs/images/replication-flow.png`
- 创建：`docs/images/gas-flow.png`
- 创建：`docs/examples/network-report.json`

**接口：**

- `AAuthorityArenaHUD::DrawHUD()` 仅从复制/本地诊断读取，不改权威状态。
- 显示 Health/Energy/Cooldowns、LocalRole/RemoteRole、ping/loss/corrections、Dash predicted/confirmed/rejected、Shield、Death/Respawn/Score 和最近权威事件。

- [ ] **Step 1：RED—HUD view-model 测试**

将格式化逻辑放入可测 helper，先测试 role 名称、百分比钳制、cooldown、拒绝事件与断连状态；观察失败。

- [ ] **Step 2：实现纯 C++ HUD 与表现反馈**

两名角色使用稳定高对比颜色；Dash 预测/确认/拒绝、Projectile impact、Shield 用不同短暂颜色/形状；支持窗口化双客户端同屏截图。

- [ ] **Step 3：GREEN 与可交互双客户端**

运行 helper tests 与 baseline；以两个真实独立客户端窗口获得截图。截图前记录源 SHA，确认图中可见两个 client 的不同 playerId/role/状态。

- [ ] **Step 4：生成真实图示与精简报告**

架构/复制/GAS 流图必须与当前代码类型和 RPC 名称一致；network-report 从本轮 JSON 抽取并去除机器敏感绝对路径。

- [ ] **Step 5：提交**

核验 PNG 尺寸/体积与来源 SHA，提交 `feat: add observable multiplayer demonstration` 并 push。

---

### Task 9：PACT-60 全量测试、Cook/Package 与 clean-source 重验

**文件：**

- 创建：`scripts/Run-Automation.ps1`
- 创建：`scripts/Package-Win64.ps1`
- 创建：`scripts/Verify-PackagedBuild.ps1`
- 创建：`scripts/Verify-CleanSource.ps1`
- 创建：`docs/TESTING.md`
- 修改：`docs/BUILD_SYSTEM.md`
- 修改：`docs/ACCEPTANCE_MATRIX.md`

**接口：**

- `Package-Win64.ps1 -Configuration Shipping -OutputDirectory Artifacts/package/<sha>` 调用 RunUAT BuildCookRun，并输出 manifest JSON。
- manifest 包含 source SHA、Build/Cook/Stage/Pak/IoStore/Archive 结果、根路径、总大小、主 exe SHA-256、文件树摘要。

- [ ] **Step 1：RED—交付验证器拒绝缺失产物**

先运行 `Verify-PackagedBuild.ps1`，预期因 manifest/exe/Pak/哈希缺失失败。

- [ ] **Step 2：运行完整自动化**

Core MQB tests、ProjectStructure、UE `AuthorityArena.*` Automation、headless、interactive、Editor Development、Game Development、Game Shipping 全部从脚本运行并记录 exit code。

- [ ] **Step 3：RunUAT BuildCookRun**

```powershell
& $RunUat BuildCookRun -project="$Project" -noP4 -platform=Win64 `
  -clientconfig=Shipping -build -cook -stage -pak -iostore -archive `
  -archivedirectory="$Archive" -utf8output
```

若项目未启用 IoStore，移除 `-iostore` 并把该项记为 NOT APPLICABLE（附配置证据），不能假报通过。

- [ ] **Step 4：验证打包普通启动与双客户端**

先启动单个打包 exe 可交互窗口取得 Ready/退出证据；再用 packaged server + two clients 跑 baseline 与至少一项 lag 场景，验证同一功能断言。

- [ ] **Step 5：计算产物证据**

使用 `Get-FileHash -Algorithm SHA256` 计算主 exe/Pak/manifest；记录总大小、本地路径、源 SHA。所有产物仍位于 ignored `Artifacts/`。

- [ ] **Step 6：clean-source 重验**

在 `Artifacts/clean-checkout/<sha>` 创建临时 clone（目标路径先验证在 Artifacts 下），checkout 精确 feature SHA，不复制 Binaries/Intermediate/Saved；重新运行 MQB、UBT、Automation、BuildCookRun 与 packaged baseline。不得用当前脏工作区结果代替。

- [ ] **Step 7：提交文档证据**

更新矩阵与 `TESTING.md`，只提交小型去敏摘要，不提交本地包；提交 `test: verify clean packaged Win64 delivery` 并 push。

---

### Task 10：PACT-70 作品集材料、独立审计与 source-only Release

**文件：**

- 创建：`README.md`
- 创建：`README_ZH.md`
- 创建：`docs/KNOWN_LIMITATIONS.md`
- 创建：`docs/AI_ASSISTANCE.md`
- 创建：`docs/CODE_WALKTHROUGH.md`
- 创建：`docs/INTERVIEW_GUIDE.md`
- 创建：`docs/LIVE_CHANGE_DRILLS.md`
- 创建：`docs/ROLLBACK_VS_UE_REPLICATION.md`
- 创建：`docs/RELEASE_NOTES_0.1.0.md`
- 创建：`docs/PR_BODY.md`
- 修改：`tasks/20260904-224034-authority-arena-0.1/handoff.md`
- 修改：`tasks/20260904-224034-authority-arena-0.1/progress.md`

**接口：**

- README 首屏回答产品价值、运行方式、真实多进程证据、预测/权威边界、限制和 AI 参与。
- AI 披露逐字保持：用户定义目标/范围/约束；Codex GPT-5.6 Sol 完成架构细化、代码、测试、调试、打包、审计和文档；用户未独立手写本次代码；不能声称完全手写；面试前需理解架构并完成至少一次 Drill。

- [ ] **Step 1：文档事实检查 RED**

创建 `scripts/Verify-Documentation.ps1`，检查所有必需文档、README 八项首屏答案、AI 披露、代码类型/RPC/命令与仓库实际匹配。首次因文档缺失失败。

- [ ] **Step 2：编写作品集与面试材料**

逐项引用真实命令、报告和限制；`ROLLBACK_VS_UE_REPLICATION.md` 比较目标、所有权、预测、纠错、带宽、确定性和适用场景；Live Drills 提供 Energy 恢复、Dash 拒绝条件、RepNotify UI 三个独立小改动及验证命令。

- [ ] **Step 3：文档验证 GREEN**

运行 `Verify-Documentation.ps1`，再人工逐项比对 README 与当前代码/报告，不把未跑配置写成 PASS。

- [ ] **Step 4：派发全新只读独立审计**

审计 Agent 禁止写仓库；按目标原文和 `ACCEPTANCE_MATRIX.md` 逐项检查 Git status、代码、命令、日志/JSON、截图、包 manifest、clean-source、PR/CI，按 Blocker/High/Medium/Low 输出带证据发现。

- [ ] **Step 5：仅修复确认发现并全量重验**

若有 Blocker/High，先写失败测试/复现，再最小修复；随后重新运行 Task 9 全矩阵并更新源 SHA/包哈希。若无法清零，不得发布，保持 Draft Alpha/WIP 并完成 handoff。

- [ ] **Step 6：准备 Ready/merge 门禁**

确认 feature 工作树干净、全部提交已 push、CI green、矩阵无 P0 NOT RUN/FAIL、审计无 Blocker/High、Release Notes 源 SHA 与最终 feature HEAD 相同。

- [ ] **Step 7：正常合并、annotated tag 与 Release**

仅在 Step 6 成功时运行：

```powershell
gh pr ready <PR_NUMBER>
gh pr merge <PR_NUMBER> --merge --delete-branch
git switch main
git pull --ff-only origin main
git tag -a v0.1.0 -m 'AuthorityArena v0.1.0'
git push origin v0.1.0
gh release create v0.1.0 --title 'AuthorityArena v0.1.0' `
  --notes-file .\docs\RELEASE_NOTES_0.1.0.md --verify-tag
```

不得向 `gh release create` 传入文件参数。

- [ ] **Step 8：最终 Release 审计**

```powershell
gh api repos/Iviesever/authority-arena-ue5/releases/tags/v0.1.0 `
  --jq '{tag_name,draft,prerelease,assets:[.assets[].name]}'
git ls-remote origin refs/heads/main refs/tags/v0.1.0
```

预期：`draft=false`、`prerelease=false`、`assets=[]`；tag 为 annotated tag 且 main 含 merge commit。复核无本轮自有进程残留后，才能完成 goal。

## 计划自我审查

- 规范覆盖：PACT-00～70、MQB、GAS、权威、全部网络/故障场景、可观察性、Automation、三种构建、Cook/Stage/Pak/IoStore/Archive、打包运行、clean-source、文档、审计、merge/tag/source-only Release 均有任务。
- 占位符：命令中的 `<PR_NUMBER>` 只在 GitHub 创建 PR 后由真实 API 返回值替换，不是未定义需求；任何测试状态默认 NOT RUN。
- 类型一致：Core `DecisionCode` 经 UE `FValidationResult` 适配；ASC/Attributes 始终位于 PlayerState；Character 只作为 avatar；所有最终状态由服务端写。
- 顺序一致：PACT-00 完成后才进入 GAS；每个行为任务均先 RED、再最小实现、再 GREEN/真实运行、证据、提交、push。
- 发布安全：若门禁失败，计划唯一允许的结果是 Draft Alpha/WIP + 准确 handoff，没有创建虚假 tag/Release 的替代分支。
