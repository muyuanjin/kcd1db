# 首发与上传配置

在 Nexus 的 [Kingdom Come: Deliverance](https://www.nexusmods.com/kingdomcomedeliverance) 建立模组页。按[首发页面草稿](../../../../docs/nexusmods/first-release.md)填写名称、摘要和安装说明，按实际表单选择分类。首次通过网页添加 `build/release-assets/kcd1db.zip` 为主文件，文件说明取[对应版本](../../../../docs/nexusmods/files/0.1.0.txt)。安装需要把 ASI 放进游戏的 `Bin/Win64`，文件设为手动下载。

后续上传复用这个主文件条目。在 Nexus 的 **Files → Advanced → API Info** 或 **Manage → Files** 的编辑菜单中读取稳定的 v3 文件 ID。Mod 页面 ID 和旧下载链接中的 `?file_id=` 各自有其他用途。[Nexus 上传 Action 的文件 ID 说明](https://github.com/Nexus-Mods/upload-action#how-to-find-the-file-id)

## GitHub 凭据

用 `gh secret list --repo muyuanjin/kcd1db --json name,updatedAt` 核对 secret 元数据。复用已有可用的 `NEXUSMODS_API_KEY`。需新增或轮换时，用户从 [Nexus API Keys](https://www.nexusmods.com/settings/api-keys) 取值，在本机遮罩窗口输入。

```powershell
powershell.exe -NoProfile -File .agents/skills/kcd1-nexus-publishing/scripts/set-github-secret.ps1 -Repository muyuanjin/kcd1db
```

窗口通过标准输入向 `gh secret set` 提交值。提交结果不明时先查 GitHub secret 元数据。API key 只在 Nexus 取值和本机遮罩窗口中使用。

## 上传后续版本

[上传工作流](../../../../.github/workflows/publish-nexusmods.yml)须位于默认分支。对应版本的文件说明 `docs/nexusmods/files/<version>.txt` 随 GitHub tag 一并发布。待公开 GitHub Release 可下载 `kcd1db.zip`，并确认 Nexus 主条目缺少该版本后，触发上传。

```powershell
gh workflow run publish-nexusmods.yml --repo muyuanjin/kcd1db `
  -f tag=v1.2.3 -f 'file_id=<稳定文件 ID>'
```

工作流从标签读取文件说明，并从标签推导 Nexus 版本号。Nexus v3 API 的文件说明上限为 255 字符，完整版本说明放到 Posts 的置顶帖。修正已有 GitHub Release 的文件时，用可选 `description` 输入提供更新后的说明。将旧文件下架时启用 `archive_existing_version`。上传成功后，用公开 Files 页验收版本、说明和下载。
