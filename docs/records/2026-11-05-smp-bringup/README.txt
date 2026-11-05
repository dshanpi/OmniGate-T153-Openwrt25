2026-11-05 T153 四核实机记录
===========================
镜像ad406a8489440386e0039bbd0a3655df2a475087dc3f9736eef5b1291b7177f6。
源码SPL/DDR/PSCI/U-Boot + Linux6.12.113/OpenWrt25.12，物理DDR1GiB，Linux384MiB。

1. 从用户硬件进入FEL开始，933MHz FES写292MiB、verify/post-reboot成功。
2. U-Boot从eMMC读回整盘，SHA精确等于上述镜像。
3. Linux显示4核启动、online0-3、4项processor，四列timer/IPI，Err0。
4. 逐核绑定2秒并行计算/共享数据一致性PASS；空闲5秒后复测PASS。
5. 普通sync/reboot后源码DDR重新初始化，4核上线，第三次短测PASS。
6. 官方MCP固定reboot efex入口成功；新SPL清RTC标志并进入BootROM，USB FEL online。
7. 同一镜像第2次FES写入/校验/重启成功，再次启动4核OpenWrt，最后释放串口。
   最终capture含用户手动cat /proc/cpuinfo，观察到0/1/2/3；第2次重刷后没有
   再运行自动并行短测，前三次的运行日志保留在独立文件。

WAN自动DHCP；首轮1000Full网关3/3 ping。多次链路抖动导致首个wget失败，
后续下载的测试程序SHA匹配；普通重启及最终启动最终降100MbpsFull。
千兆稳定性问题未解决，不把恢复后的成功覆盖失败，也不据短测证明长期可靠。

本次是FES软件重启/普通watchdog重启，不是物理断电再上电验收。
离线构建manifest的hardware_verified=false保持原意；本记录独立绑定hash
报告实际实机结果。状态摘要runtime-validation.json，原始/公开日志hash及
脱敏规则provenance.json。CPU0基线和负对照另见../2026-11-05-bringup/。
