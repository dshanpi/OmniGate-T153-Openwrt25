T153 1Gbps PHY internal-loopback packet diagnostic (2026-11-05)
============================================================

Status and scope
----------------
Source and a statically linked ARM/musl helper have been built. Host-only
selftests pass; runtime board results belong in docs/records/2026-11-05-wan.
This is a diagnostic, not a WAN fix. No released bootloader/kernel/DT/image is
modified. The helper never writes a PHY/MAC register or changes networking.
It only tests a PHY loopback that the operator has already enabled, and the
operator remains responsible for restoring that temporary configuration.

Use only from the independent serial console. Running the setup on the very
WAN connection used for administration would interrupt that connection.
Do not interrupt a currently useful normal-link comparison merely to run this.

Build
-----
From the remote openwrt25 source directory:

export STAGING_DIR="$PWD/openwrt/staging_dir"
openwrt/staging_dir/toolchain-arm_cortex-a7+neon-vfpv4_gcc-14.3.0_musl_eabi/bin/arm-openwrt-linux-muslgnueabi-gcc \
  -Wall -Wextra -Werror -Wno-misleading-indentation -O2 -static \
  -o tests/diagnostics/t153-phy-loop-packets \
  tests/diagnostics/t153-phy-loop-packets.c

The exact command, compiler version, binary/source hashes and validation are
recorded in build-manifest.json. artifacts/diagnostics contains the HTTP copy.
The binary is ignored by Git; the C source and manifest are the source backup.

Read-only prerequisites and setup semantics
------------------------------------------
1. Record the current ethtool settings (including *all* advertised modes,
   Pause/Asym_Pause), PHY ID, raw BMCR(0), ADVERTISE(4), CTRL1000(9), and page(31).
   Keep the interface UP; stmmac refuses MDIO ioctls if it is administratively
   down. Avoid changing the page: standard registers suffice for this test.
2. First use ethtool to request speed 1000, duplex full, autoneg off. This
   synchronizes phylib's software configuration. Then write raw BMCR 0x4140:
   bit14 LOOPBACK + bit8 FULLDPLX + bit6 SPEED1000; ANENABLE, RESET, PDOWN,
   ISOLATE and SPEED100 must all be clear.
3. Wait a bounded time (up to 4 seconds) for carrier=1, speed=1000 and full
   duplex. Read BMCR back. If these do not agree, do not count a packet test as
   a timing failure: setup itself was not established. Record the raw BMSR
   too; its link bit is latched low, so distinguish first and second reads.
4. If the prior 100M carrier never transitions down, the MAC may retain the
   previous configuration. A separate bounded pre-step may temporarily set
   BMCR_PDOWN and wait for carrier=0 before restoring power with 0x4140.
   Never use a reset as a substitute: that adds another initialization variable.

This setup is intentionally not embedded in the packet helper. Keep a shell
trap or independent bounded controller around the setup/test/restore sequence;
restoration must run after test failure, signal, timeout or carrier loss too.

Actual Linux 6.12.113 call chain (important)
-----------------------------------------
stmmac_ioctl -> phylink_mii_ioctl -> phylink_phy_write -> mdiobus_write.
The actual driver bypasses phy_mii_ioctl's special BMCR handling. Therefore a
raw SIOCSMIIREG BMCR write does NOT update phydev->autoneg/speed, does NOT set
phydev->loopback_enabled, and BMCR_RESET does NOT itself call phy_init_hw.
The PHY state machine continues polling during a raw MDIO loopback.

Also, __genphy_config_aneg intentionally retains hardware autonegotiation for
1000BASE-T even when software autoneg is off. That is why ethtool alone may
leave BMCR_ANENABLE set; the subsequent raw 0x4140 write is required here.
RTL8211F rtlgen_read_status reads vendor PHYSR after genphy_read_status and
can overwrite the reported speed, so never infer usable 1G carrier from the
BMCR speed bit alone.

Run
---
/tmp/t153-phy-loop-packets --selftest
/tmp/t153-phy-loop-packets eth1 16 100

This sends 16 total frames alternating 64 and 1514 bytes (Ethernet header
included, FCS excluded), with a 100ms receive deadline per frame. Bounds are
1..128 frames and 10..500ms per frame. SIGINT/SIGTERM/SIGHUP stop the test.
Before every frame and once at the end, the helper rechecks page 0/a42, BMCR
and sysfs carrier/speed/duplex. It requires exact RTL8211F ID 001cc916. At the
beginning and end, read-only ETHTOOL_GREGS verifies the DWMAC4 MAC_CONFIG word:
PS/FES=0 (1G), DM=1, LM=0 (MAC internal loopback OFF), TX/RX enabled. Thus a
MAC-only loop cannot masquerade as a PHY/RGMII pass. The driver internally
collects a full register dump even when requesting only the first word, so
this guard is deliberately limited to the transaction boundaries.

Frames use experimental EtherType 0x88b5, both source and destination equal to
the selected interface's MAC, random 64-bit nonce, sequence, declared length,
deterministic full payload, and CRC32. The receiver is bound to that interface
and EtherType, enables PACKET_IGNORE_OUTGOING, and independently accepts ONLY
PACKET_HOST with the expected interface/protocol. It checks exact full-frame
bytes plus CRC32. A local outgoing socket copy, a stale sequence, a reflected
frame from another run or corrupted payload cannot satisfy the test.

Exit codes: 0 = all expected HOST frames matched and guards stayed valid;
1 = packet timeout/mismatch; 2 = setup/guard/system error; 130 = interrupted.
These codes do not claim normal external-cable throughput or link stability.
A pass exercises the PHY's internal digital return path including RGMII; it
is not an acceptance test for the copper/analog path or all PHY features.

Restore
-------
Always clear the temporary loopback and restore the recorded link settings,
BMCR, advertising, CTRL1000 and any page change, then restart normal negotiation
if it was originally enabled. Verify readback and actual carrier/speed. Do not
blindly restore with `advertise 0x2f`: it omits Pause and Asym_Pause that were
present in the original 0x602f mask, introducing a separate test variable.
Self-clearing BMCR_RESET/ANRESTART bits are not persistent settings to replay.
No delay-tap, PLL, proprietary PHY page or permanent configuration change is
part of this test. A program killed with SIGKILL or power loss cannot execute
its own cleanup; use the independent bounded controller for setup/restoration.

Host-only verification
----------------------
`--selftest` verifies standard CRC32(123456789)=cbf43926; a correct incoming
frame passes; PACKET_OUTGOING, changed payload, changed payload with recomputed
CRC, stale sequence and short frame all fail. It opens no sockets or devices.
The host selftest and target cross-build were completed without board access.

TCP 上传完整性检查（主机工具）
----------------------------
tcp-upload-check.py 在构建主机启动有界接收端，参考文件可为公开的随机测试
payload；不接收命令，不写入上传文件。默认只绑定127.0.0.1，板卡测试需显式
指定构建主机LAN地址。示例：
  python3 tests/diagnostics/tcp-upload-check.py reference.bin --bind BUILD_HOST --count 4
板卡将同一已校验的 reference.bin 放到/tmp后运行：
  nc BUILD_HOST 8879 < /tmp/reference.bin
每次服务端对收到的预期字节数与SHA256进行校验，返回JSON并关闭连接；重复
4次后退出。OpenWrt精简BusyBox的nc不支持-w，因此超时由接收端控制。
传输阶段有整体超时，等待连接也有超时；默认90秒，可用--timeout调整。
结果只证明预期N字节完整到达，不检查随后是否还有额外尾部，不是线速测试。

本轮实机执行了普通TCP双向校验和短暂PHY状态探测。AF_PACKET工具只完成
编译与主机输入判别自测，尚未在板上执行包回环；不能据其存在宣称回环通过。
