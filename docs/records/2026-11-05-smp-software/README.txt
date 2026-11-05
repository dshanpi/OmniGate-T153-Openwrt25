2026-11-05 四核软件检查记录
===========================

这些文件记录源码/链接/DT/镜像的软件验证，不替代板上四核运行结果。
最终四核镜像 SHA256：
  ad406a8489440386e0039bbd0a3655df2a475087dc3f9736eef5b1291b7177f6
大小306184192字节；配置/镜像manifest在同目录。

使用11/4已完整构建的CPU0基准42df6e8f...中的同一内核/rootfs，仅替换源码
SPL/U-Boot区。新运行时代码当时未提交，software-report记录实际基commit
和运行时源文件SHA，后续发布commit请以最终仓库为准。没有在本轮重新编译
全部OpenWrt/toolchain，release一体入口的分支选择另用明确的构建替身测试。

SMP构建、最终ELF驻留、libfdt/UBSan、实际FEL请求C/UBSan、整盘验证通过。
CPU0独立输出目录编译也通过，确认无SMP monitor符号、默认配置不启用四核，
并验证早期FEL hook。未覆盖或重打包已上板的CPU0基准磁盘。

secure-disassembly和fel-request-disassembly均为本工程GPL源码构建输出。
fel-entry-evidence仅保存厂商参考的hash、地址及行为事实，不含厂商完整二进制
或完整反汇编。FES临时loader与持久源码bootchain的许可边界见FES-REPRODUCE。
