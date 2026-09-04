# AuthorityArena（中文）

AuthorityArena 是一个 UE 5.8 C++ 多人网络工程实验：两名玩家在程序化生成的第三人称灰盒竞技场中战斗，项目覆盖原生复制、GAS 预测、服务端权威、恶劣网络测试、三进程自动化与 Win64 本地打包。

![两个真实 UE 5.8 客户端视口与网络诊断](docs/images/authority-arena-two-clients.png)

| 问题 | 答案 |
|---|---|
| 项目做什么 | 演示 CharacterMovement、Ownership/Role、RepNotify、RPC 校验、GAS Dash/Attack/Shield、伤害、死亡、复活、计分、JSONL 证据与打包。 |
| 为什么有岗位价值 | 把 UE/C++ 网络面试概念落实成可运行的边界、失败路径与证据，而非单进程视觉样例。 |
| 如何运行 | 安装 UE 5.8、MSVC、PowerShell 7 与 MQB 5.4，在仓库根目录执行下方命令。 |
| 真实多进程证据 | 一个独立服务端进程和两个独立客户端进程，拥有不同 PID、同一 runId、各自 JSONL、真实代理角色与有界清理。 |
| 本地预测 | Dash、Shield 为 `LocalPredicted`；Attack 接受本地预测输入。 |
| 服务端权威 | 弹体生成/命中、Health、伤害、资源/冷却合法性、目标/距离、Death、Respawn、Score。 |
| 明确限制 | Epic 安装版不能构建 Server Target；Shipping 禁止命令行 URL 覆盖；详见限制文档。 |
| AI 参与 | 用户定义目标和约束，Codex GPT-5.6 Sol 完成交付；不得声称为用户独立手写。 |

## 快速运行

```powershell
pwsh -NoProfile -File .\scripts\Test-Core.ps1
pwsh -NoProfile -File .\scripts\Build.ps1 -Target Editor -Configuration Development
pwsh -NoProfile -File .\scripts\Run-Automation.ps1
pwsh -NoProfile -File .\scripts\RunMultiplayerScenario.ps1 `
  -Scenario Combat -NetworkProfile Baseline
pwsh -NoProfile -File .\scripts\Invoke-NetworkMatrix.ps1
pwsh -NoProfile -File .\scripts\Invoke-FailureMatrix.ps1
pwsh -NoProfile -File .\scripts\Package-Win64.ps1 -Configuration Shipping
```

本机验证环境为 UE 5.8.0 CL 55116800、UBT 选定 MSVC 14.44、Windows SDK 26100、PowerShell 7.6、MQB 5.4。打包件、Cook 输出、完整日志和 Trace 只保存在被忽略的本地目录，不提交也不上传 Release。

## 核心边界

- GameMode：只存在于服务端，负责连接、出生、死亡与复活。
- GameState：复制比赛阶段、轮次、runId 和统一场景时钟。
- PlayerState：持有 ASC、Attributes、Score、Deaths，跨 Pawn 重生存续。
- Character：可替换 Avatar，使用 UE 原生 CharacterMovement 预测与纠正。
- Projectile：仅服务端生成并应用 GameplayEffect；Multicast 只承载确认后的表现。
- Runner：严格持有三个进程，验证结构化事件，仅清理精确匹配的本轮 PID。

完整说明见[验收矩阵](docs/ACCEPTANCE_MATRIX.md)、[测试](docs/TESTING.md)、[网络模型](docs/NETWORK_MODEL.md)、[GAS 设计](docs/GAS_DESIGN.md)、[已知限制](docs/KNOWN_LIMITATIONS.md)和[AI 披露](docs/AI_ASSISTANCE.md)。面试前应阅读[代码导览](docs/CODE_WALKTHROUGH.md)，并至少完成一次[现场改动练习](docs/LIVE_CHANGE_DRILLS.md)。
