2026-11-05 WAN 时序与协商排查（问题未解决）
===========================================
基线是已发布四核ad406a镜像，持久内核/DT/U-Boot未改。用户确认：同板、同线、
同对端在原厂Tina下稳定千兆。此事实应作为软件对照，不擅自认定线材故障。

实际 Tina 构建路径更正（后续只读核验）
------------------------------------
从SDK已有bImage和vmlinux各自提取内嵌config，二者完全相同且
CONFIG_REALTEK_PHY未设置。System.map没有rtl8211f_config_init/rtlgen_read_status，
有genphy_driver；已启用的Motorcomm驱动精确ID表不匹配001cc916。因此这份
实际SDK产物的RTL8211F走phy_attach_direct的Generic PHY回退，不执行源码树中
存在但未编入的Realtek config_init。此前用该函数推断Tina实际会设置ALDPS、
关闭RGMII RX/TX延迟的说法已撤回；保留下面历史试验事实，不将错误前提继承
为根因判断。原始构建产物hash/config/symbol证据见tina-actual-driver-evidence.json。
这项核验未重新启动原厂系统，且SDK当前bImage/vmlinux的SHA尚未与用户当时
实际烧录的Tina文件核对。用户确认稳定对照为同板同线同对端同EC20，仅代表
硬件连接条件相同，不表示已确认镜像内容一致。

已完成对照
----------
A 发布配置：MAC TX9/RX6，phy-mode=rgmii-rxid，PHY TX无延迟/RX2ns。
B 临时RAM DT：仅MAC RX6改0，保持TX9和PHY RX2ns。
C 临时RAM DT：MAC TX0/RX0，phy-mode=rgmii-id，PHY TX/RX各2ns。
B/C用U-Boot fdt move从原FIT复制DT到0x4f000000，再bootm原FIT+外置DT。
原FIT CRC/SHA未破坏，四核fixup保留；Linux hexdump确认相应tap=0。

A/B/C均观察到降100Mbps；B中重启协商、仅广告1000Full也出现千兆短暂上线后
掉线。C初次协商直接100Mbps。PHY实际ID001c:c916；A页d08 reg11=0009，
reg15=0019；C分别0109/0019，证实PHY延迟位变更生效。
MAC CRC为0的片段有效数据量很少，不能据此宣称125MHz RGMII数据通路通过。
1000-only阶段16MiB HTTP尝试因尚无可用链路而失败，没有产生测速结果。

PHY初始化差异尝试
-----------------
早期根据Tina源码中未启用的Realtek驱动尝试ALDPS mask1006；C上原PHYCR1=2118，
保存其它位后改311e并重新协商，仍100Mbps。实际SDK Generic PHY和当前主线
均不主动改ALDPS，不能将这项失败尝试称为恢复真实Tina设置。
主线清a43:19 bit5（PHY-mode EEE），实际Tina Generic PHY不改该位；原值0843，
临时改0863。
直接改位和随后一次标准BMCR软复位/重新协商均未得到稳定千兆。
重要限定：本实际内核的stmmac_ioctl调用phylink_mii_ioctl；SIOCSMIIREG经
phylink_phy_write直接mdiobus_write，不经过phy_mii_ioctl，因此写BMCR_RESET
不会主动触发phy_init_hw/config_init，也不会同步phylib的软件协商状态。
此前“复位会由config_init再次清EEE bit5”的推断不适用于这条实际调用链。
日志只确认复位前a43:19=0863及1秒后BMCR=1040，未复核复位后EEE位值和
其它PHY设置；该尝试仍不能视为严格验证或排除PHY-mode EEE的单变量A/B。
源码依据：实际构建树stmmac_main.c:6207-6220，phylink.c:3568-3613、3672-3701。
EEE ethtool查询报告not supported；downshift tunable也不支持。
用户态ethtool的master-slave参数要求netlink，本镜像工具不支持，返回1。
临时普通MDIO CTRL1000=0600尝试preferred-master+1000Full并重新协商，仍
出现掉线。随后恢复普通广告，再普通重启恢复发布配置；未永久保存PHY设置。

源码及实机核对
--------------
- Tina活跃omnigate/linux-5.10-origin真实DTB的WAN也是MAC9/6，无delay-maps。
- Tina源码中未编入的Realtek驱动会在rgmii下关闭PHY TX/RX延迟，但实际SDK
  Generic PHY不执行该操作。原11/4负对照证明本主线下关闭PHY RX延迟导致
  严重CRC和ping失败；不能仅据DTS=rgmii或未启用的驱动源码盲目回退。
- GPIO PG0..15 mux5、40mA、上拉、PJ3低有效复位10ms/释放后30ms，与SDK相同。
- PHY供电控制在活跃Tina DTS中也是注释，未找到主线漏启用的已声明电源。
- clk_summary PHY25MHz；AHB200MHz。GMAC gate父时钟显示24MHz是占位，
  两端均明确clk_csr=4，ethtool寄存器dump的CR=4，排除自动按24M选错MDC这条路径。
- 普通reboot中断后只读U-Boot：CCU020020a0=b8216310，02003908=0fff0fff，
  已有EN/LOCK_EN/LOCK/OUTPUT和SDK整组分发mask。因此准备中的CCU候选未采用、
  未编译、未应用。source SPL未写这些PERI0寄存器，普通重启读数不能证明
  冷上电的初始来源，不应把它当冷启动时钟验收。
- 未在当前SDK或实际rootfs发现自动改写TX/RX校准值的启动程序；Tina calibrate
  同样要求已有carrier，仅显式sysfs写触发，不接受速度参数，不保存NVMEM。

当前结论与下一项
----------------
尚无可证明修复千兆的参数组合，不能把任一试验参数固化成修复版。
用户的原厂稳定对照仍需与实际PHY/RGMII状态结合定位。更直接的下一步是
有界的1Gbps MAC/PHY内部回环，分别检验DMA/MAC与125MHz RGMII数字通路。
当前CONFIG_STMMAC_SELFTESTS未开；仅删除上游selftest的carrier检查不够，
dev_direct_xmit仍检查carrier。尚未实现、编译或执行专用回环诊断。

后续缓存中发现的两次非本轮命令发起的重启，用户已在本次2026-11-05反馈中
明确确认都是手动操作。对应日志未见正常reboot、panic或oops；这两次重启
不应归因于SMP异常。该段缓存最后一条loopback命令仅激活控制台，没有执行
结果，不能据这段缓存报告回环通过/失败；后续试验须另据实际结果记录。

最后只读状态：OpenWrt uptime 9min，CPU online0-3，WAN100Mbps Full link up；
实际DT RX tap恢复6。没有任何回环模式或实验内核被安装。未采用的CCU草案
保留在构建机logs/experiments中，不属于可部署源码或发布产物。
