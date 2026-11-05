官方 OpenWrt v25.12.6 原生构建记录（2026-11-05）
================================================
本目录记录软件构建；本轮板卡验收另见 ../2026-11-05-native-hardware/，
其记录绑定同一最终 raw SHA，旧 snapshot 结果不能替代本版本。软件记录
本身不声明冷启动或长期稳定性通过，实机通过项与未测项以独立板验为准。

复现入口：官方 v25.12.6 归档 + patches/openwrt-25.12.6-t153.patch。
标准用户步骤：patch -p1，feeds update -a，feeds install -a，menuconfig
选择 sunxi/cortexa7/T153，保存后 make defconfig、make -j8 V=s。
详细操作以 docs/OPENWRT-25.12.6-NATIVE.txt 为准。

本次实际过程：维护者先用 apply-native.sh 将适配放入新官方归档目录，
最终导出的单补丁又在另一份干净归档完整应用并逐一核对 82 个文件。
新构建目录单独构建 tools/toolchain，只共享已下载源码 dl。
为等待 feeds 网络下载时节约时间，先用三行 target/profile 种子构建主机工具
及工具链；feeds 全部安装后重新从三行种子展开，49 项自动选择检查全通过。
真正打开 PTY menuconfig，T153 profile 可见，退出码 0。完整 .config 与
本次 buildinfo 已保存。工具阶段未保存完整原始 CLI，不补猜并行等参数。

首次顶层 make 返回 0，但独立检查发现 make_ext4fs 的 inode 位图尾部填充
未置位，e2fsck -fn 返回 4。修正 T153 镜像步骤：只规范化私有 root.ext4
副本，固定两项 fsck 时间，最后要求只读检查返回 0。第二次顶层 make 为
增量构建，返回 0；并非第二次从零重建。最终启动区、普通内核/FIT、DTB、
文件系统、已安装版本均通过独立验证。之后复用同次编译输入重跑镜像步骤，
raw 逐字节相同。详见 ../2026-11-05-native-image/。

feeds 首轮仅 video 的 TLS 下载中断；原地址单项有界重试成功，所有提交和
官方 feeds.conf.default 完全未变。日志保留 Prosody 缺包及上游 weechat
配置循环诊断，未为了消除提示修改官方 feeds；本次所选配置完成构建。

文件说明：
- source.json / patch-apply.json：来源和最终补丁应用。
- feeds-validation.json：官方 pin、网络重试与安装结果。
- native-config-validation.json / menuconfig-validation.json：配置检查。
- build-report.json：步骤、边界、退出码、最终镜像身份。
- *.log.gz：脱敏原始输出；保留错误、警告和 PTY 控制序列。
- log-manifest.json：原日志、脱敏日志及 gzip 各自的大小/摘要。
- *.exit / *.rc：真实保留的退出码；没有记录的返回值不补造。
- openwrt.config.gz / *.buildinfo / *.manifest / profiles.json：本次配置产物。
- uboot-build.json：同次源码 DDR、四核 U-Boot 与启动区校验报告。

日志只替换 Linux 构建目录、适配仓库目录和 home 路径前缀；原始日志仍在
构建机。未收集 Windows .tools、private-backups、SSH 凭据或完整原厂镜像。
