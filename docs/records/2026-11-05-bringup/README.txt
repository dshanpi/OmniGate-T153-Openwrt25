2026-11-05 CPU0基线实机记录
==========================
镜像42df6e8f2755e362eb82c7121671855984cd68c65fea037ac0d3bbcd87e6aad8。
FES写入校验/post-reboot、U-Boot全盘读回、Linux eMMC ext4和OpenWrt通过。
WAN持久rgmii-rxid、DHCP、1000Mbps Full下网关5/5 ping；仅短测，不能证明长期稳定。
reboot efex触发watchdog复位后仍从eMMC启动，没有USB FEL；未自动重试。
四核工具负对照在CPU0系统正确退出1，不能当成四核正向成功。
四核候选及SPL早期FEL修复的软件记录另见../2026-11-05-smp-software/。
原始私有日志的SHA、发布副本SHA和脱敏规则见provenance.json。
