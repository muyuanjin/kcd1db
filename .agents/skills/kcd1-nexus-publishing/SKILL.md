---
name: kcd1-nexus-publishing
description: 为 Kingdom Come Deliverance 的 kcd1db 准备 Nexus Mods 首发、版本更新、上传配置和页面维护。适用于主文件、稳定文件 ID、GitHub Actions 凭据和中断恢复。
---

# kcd1db Nexus Mods 发布

从仓库根目录按 [README](../../../README.md) 构建 Release，再运行 `pwsh -File scripts/package-release.ps1`。主文件为 `build/release-assets/kcd1db.zip`。`kcd1db_fake_db.lua` 随 GitHub Release 单独提供给模组作者。按[手动测试指南](../../../docs/manual-testing.md)核对本次受影响的功能。

- 首发、文件 ID、密钥和上传工作流见[发布配置](references/setup.md)。
- 更新、中断恢复和公开下载验收见[更新与恢复](references/updates.md)。
- 模组说明、置顶帖和图库见[页面与媒体](references/browser.md)。

以 Nexus 上实际可下载的版本为更新基线。用 `gh` 检查 GitHub Releases 和 Actions。操作登录态 Nexus 页面前读取 `browser-skill`。按本次任务的授权执行外部写入。
