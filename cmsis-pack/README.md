# CMSIS Pack 目录

此目录用于存放由 GitHub Actions 自动生成的 CMSIS Pack 文件。

## 自动生成流程

当代码推送到 `develop` 分支时，GitHub Actions 工作流会：

1. 运行 `gen_pack.sh` 脚本生成 CMSIS Pack
2. 将生成的 `.pack` 文件复制到此目录
3. 自动提交并推送到仓库

## 手动下载

你也可以从 GitHub Actions 的 Artifacts 中下载最新生成的 pack 文件。

## 文件说明

- `fool_cat.fc_embed.<version>.pack` - fc_embed 库的 CMSIS Pack 包
