OpenWrt 25.12.6 原生镜像：2026-11-05 实机验证
================================================

本目录保存第二块 T153MX OmniGate 的本轮真实板验；物理 DDR4 为1GiB，
eMMC 为7.28GiB。它与 native-image 的独立软件证据分开，不改写软件记录
中的 hardware_verified=false。机器可读事实与12份压缩日志的字节数、
SHA256 均在 validation.json；本目录 README 是该记录的解释与索引。

一、绑定输入
------------
系统：OpenWrt25.12.6 / r33311-981c978eff / Linux6.12.112
完整 raw：306184192 字节，598016 个512字节扇区，SHA256：
  cd57d6970329f4a9255e7020ea3f7dfbc2c13040f7154563cbe43ba0d431a32e
压缩镜像：
  openwrt-sunxi-cortexa7-dshanpi_t153mx-omnigate-ext4-raw.img.gz
  16560656 字节，SHA256：
  aafef7198f0c5f68005471c30132b4a57334ae4d7e08c9ea5fcf9df5a68482ed
独立 FES loader：t153-fes-ddr933-ram-loader.img，1154048 字节，SHA256：
  0d32d52139bc969f7f90665d32203ff07366ed0a31139bda607d89edbcd12873

源码/配置/feeds：../2026-11-05-native-v25.12.6/
镜像结构与普通内核、ext4及重复生成软件证据：../2026-11-05-native-image/
完整构建、下载和烧录操作：../../OPENWRT-25.12.6-NATIVE.txt
本轮硬件结果仅绑定上述 raw，不延伸到后续修改或自定义配置的镜像。

二、通过项目
------------
1. 用户通过物理入口进入 FEL；本轮不是软件回FEL测试。FES任务
   flash-1793867316248792600 的 state/verify/post-reboot 均 success，
   exitCode=0，committed=306184192 字节。临时 loader 仍含厂商二进制，
   不能把持久启动链源码化扩展表述为全部烧录链源码化。
2. 首次 Linux 启动前，在源码 U-Boot 的 mmc1 eMMC user area 读回全部
   598016 扇区；SHA256 与 raw 完全一致。这是镜像覆盖区域的完整读回，
   不是整个7.28GiB eMMC。命令与输出保存在 native25126-readback.log.gz。
3. 源码 DDR训练通过，U-Boot2026.07 启动；Linux版本与上述绑定一致，
   /dev/root 为 ext4 rw，CPU online 为0-3。
4. 在 tmpfs 创建16MiB零文件，以 taskset -c 0/1/2/3 分别固定四个进程，
   并发 SHA256 均为：
     080acf35a507ac9849cfcba47dc2ad83e01b75663a516279c8b9d243b719643e
   此项是逐核并发短测，不是共享写一致性或长期压力测试。
5. 用户私有配置经传输后 SHA256 核对，再用 sysupgrade -r 恢复；随后
   普通软件重启通过。-r 是恢复配置，不代表支持 sysupgrade 固件刷写。
6. 恢复后的 Wi-Fi AP 在信道40 / 5200MHz / 80MHz启动；蓝牙 hci0
   UP RUNNING，HCI5.4，RX/TX errors=0。
7. 127.0.0.1 的 LuCI 请求返回 HTTP403、x-luci-login-required: yes 和
   用户名/密码表单，为正常未登录页。日志保留前两次不适用命令的失败：
   BusyBox nc 不支持 -w，且无 timeout；最后的普通 nc 管道请求成功。

三、未通过或未覆盖项目
----------------------
- WAN 再次出现从1Gbps降到100Mbps，尚未解决。不能把 PHY复位延时修正
  早期能力读取异常当作 WAN 稳定性修复。
- 本轮 eth0 carrier=0；Windows ASIX 不在当时 Get-NetAdapter 清单，
  外部 LAN 未重测，不推断具体物理原因。此前 snapshot 的 LAN成功记录
  保存在 ../2026-11-05-newboard/，不替代正式版外部 LAN 验收。
- Wi-Fi 客户端关联/吞吐、蓝牙配对/音频未测。
- option/qmi_wwan/cdc_mbim 已加载；本轮未接入 modem，无4G拨号/流量
  结果。驱动存在不等于 EC20/ML302 联网已通过。
- 物理断电冷启动、正式版软件回FEL、长期多核/DDR/存储/网络压力未测。
- Linux可见 RAM 仍为384MiB，CPU_OFF/热插拔/休眠/多核kexec不支持。
- sysupgrade/LuCI固件升级未启用，刷固件仍用完整raw/FES。
- 更早的一次 FES reconnect 失败发生在 eMMC写入之前，transferred=0；
  native25126_previous_status.json.gz 保留了失败状态，不与成功轮混用。

四、默认配置与隐私边界
----------------------
本轮恢复了用户专用配置及其 WAN管理例外；该例外只存在于测试板，发布
镜像仍以 LAN 192.168.153.1/24 为管理入口，默认禁止 WAN管理。Wi-Fi AP
运行结果发生在恢复配置后，不表示发布默认启用了同一热点。

本目录为脱敏后的验证资料。用户私有备份、密码、无线凭据等不归档、不
发布；配置恢复日志不包含私有备份本体。不要从本轮私人配置反向生成
公开默认配置。验证记录中的运行时地址只说明本次状态。

五、日志索引与校验
------------------
validation.json                         事实、输入SHA及所有.gz的大小/SHA
native25126_start.json.gz                本轮FES启动请求结果
native25126_status.json.gz               本轮FES最终状态
native25126_boot.log.gz                  FES后源码SPL/U-Boot启动
native25126-readback.log.gz              Linux首次启动前eMMC完整镜像区SHA
native25126-linux-boot.log.gz            本轮正式版Linux首次启动
native25126-runtime.log.gz               初始系统/CPU/根分区/外设状态
native25126-restored-reboot.log.gz       恢复配置后的软件重启
native25126-restored-runtime.log.gz      恢复后运行状态
native25126-cpu-test.log.gz              四核心固定亲和性并发SHA短测
native25126-http-test.log.gz             本机LuCI登录页面检查及失败尝试
native25126-final-health.log.gz          最后CPU/链路/驱动状态
native25126_previous_status.json.gz      较早FES未写入失败状态

在本目录可验证全部证据文件，任何不匹配都应停止引用该副本：

  python3 - <<'PYVERIFY'
  import gzip, hashlib, json
  from pathlib import Path
  record = json.loads(Path('validation.json').read_text())
  for name, expected in record['evidence'].items():
      data = Path(name).read_bytes()
      assert len(data) == expected['bytes'], name
      assert hashlib.sha256(data).hexdigest() == expected['sha256'], name
      gzip.decompress(data)
      print('OK', name)
  PYVERIFY

发布前已对本目录12份.gz全部核对大小/SHA并完成解压检查。编译通过、FES
写入通过、读回一致、启动通过、短测通过与长期稳定是不同层次的结论。
