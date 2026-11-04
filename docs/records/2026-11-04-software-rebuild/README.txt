2026-11-04 独立完整软件构建记录
================================

结果：17个流水线阶段全部exit 0，软件构建/检查通过；本轮没有烧录或启动板卡。
用户已断开板卡/调试器并要求本轮完成软件整理验证后上传。
开始UTC：2026-11-04T13:33:48.126605+00:00
结束UTC：2026-11-04T13:59:47.540618+00:00
耗时：1559秒。

从GitHub仓库重新clone，构建固定提交：
f16dc94a5ad509d9a951d657864d3b6ea2aa94e0。
OpenWrt/内核/U-Boot与FES生产输入和后续记录时main保持一致，后续提交
只增加隔离研究、测试/记录工具及说明；validation-summary记录比较结果。
本次是新的远端构建目录，只共享经SHA验证的下载归档，未复用旧工具链、
build_dir、staging_dir、配置或目标文件。clean-start保留原始状态；其中唯一
untracked路径downloads是预设下载缓存软链接，详见build-inputs说明。

实际命令（仓库根目录）：
  bash scripts/build-release.sh --jobs 8 --with-fes --factory-image /path/to/factory.img
使用的原厂镜像SHA锁和FES输入见configs及FES-REPRODUCE.txt；它仅为RAM
烧录loader输入，未链接进持久SPL/U-Boot/Linux。未公开上传厂商payload。

产物：t153-openwrt25-source-1g-disk.img，306184192字节（292MiB）。
raw SHA256：42df6e8f2755e362eb82c7121671855984cd68c65fea037ac0d3bbcd87e6aad8
压缩文件10028573字节，SHA256：
76291858f55c74084c795e253d20e102ba5c85a3b66804f5b60a64968a1c7dce
本次固件尚未上板，不能用旧6ea18915...的实机通过记录替代。

配置与证据
----------
openwrt.config、kernel.config、uboot-source-1g.config为实际构建的完整配置。
kernel.config保存最后一次内核构建配置；普通FIT和initramfs的内容检查见
manifest及kexec-package-check.log。build-report绑定原始构建commit和镜像。
steps/保存各阶段完整小日志和退出状态；完整OpenWrt详细日志单独作为
Release附件openwrt-clean-build.log.gz，绝对工程路径替换为角色名，原始及
公开版本hash见full-log-provenance.json。所有源码输入hash见build-inputs。
host-environment保存主机依赖版本。测试日志中故意注入的训练/校验失败是
反例测试，最终状态以PASS和对应exit 0为准。

补充检查
--------
- 最新脚本分别编译运行512/1024MiB DDR ASan/UBSan模型；测试所用ddr.c/h
  与该次固件源码一致，两个容量成功和错误路径均通过。
- 引脚、时钟、LAN/WAN默认配置、重复apply和三处sysupgrade保护检查通过。
- 新工具链重新构建static kexec，并检查实际initramfs、已链接reboot驱动、
  BusyBox/procd、CPU0/no-PSCI/384MiB设备树，全部通过。没有执行kexec。

尚未完成：本次镜像烧录/启动、原生重启/回FEL、持久WAN rxid；CPU0/384MiB
限制仍在，四核候选未集成，千兆稳定性、LAN及其它外设/长期可靠性仍待验收。
