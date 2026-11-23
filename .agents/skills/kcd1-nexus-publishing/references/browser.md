# 页面与媒体

用 `browser-skill` 操作已登录的 Nexus 会话。按页面实际控件选择编辑入口。

- 模组说明可用 **View source** 粘贴 BBCode，切回可视模式检查后保存。摘要、支持构建和安装信息与下载包一致。
- 更新置顶帖前按帖子 ID 找到现有帖子。编辑完成后在公开 Posts 页核对置顶状态和版本内容。
- 发布媒体前检查画面和元数据中的个人信息。逐张上传并记录 URL，再设置标题和缩略图。替换正文用图时，先保存并公开验证新的引用，再删除本次获授权移除的旧图。

如果 Chromium 打开 Windows 原生文件选择框，可以用[文件选择脚本](../scripts/select-upload-files.ps1)提交文件路径。浏览器窗口标题取自当前窗口。

```powershell
& .\.agents\skills\kcd1-nexus-publishing\scripts\select-upload-files.ps1 `
  -Files 'build\release-assets\kcd1db.zip' -WindowTitle '<浏览器窗口完整标题>'
```

文件选择框关闭表示路径已提交。以页面显示正确的文件名、媒体预览或公开条目确认效果。结果不明时检查同一个对话框和页面。
