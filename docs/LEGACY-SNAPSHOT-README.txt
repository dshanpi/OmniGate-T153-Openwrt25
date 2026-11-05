连接功能测试版（2026-11-05）
==========================
此连接功能候选已实机测试；尚未创建对应 Release。当前正在集成官方 v25.12.6 原生板级补丁，见 docs/OPENWRT-25.12.6-NATIVE.txt。
本版基于四核Cortex-A7，加入AIC8800D80 Wi-Fi/蓝牙及EC20/ML302软件支持。
这是实验性测试版：WAN千兆掉线/降100M仍未解决；首轮RAM测试另有一次用户
确认非人工的意外重启，根因未明。不能把编译、烧录或短测通过当作长期稳定。

本次交付文件：
  t153-openwrt25-source-1g-smp-disk.img.gz（解压后292MiB整盘）
  t153-fes-ddr933-ram-loader.img（仅驻RAM，不能写入eMMC代替源码启动链）
  t153-disk-1g-smp-manifest.json、delivery-manifest.json、SHA256SUMS
  t153-openwrt25-port.tar.gz、展开配置、FES说明与驱动固定来源归档
raw SHA256：7f2485c12409c205255ba64a57f44d26c62084c1be04aa484f582b963e884ea5
gzip SHA256：d24559f236674a1d5b5eb5ec40e0dd44e2261c773429615042aab53bcfd15940
FES loader SHA256：0d32d52139bc969f7f90665d32203ff07366ed0a31139bda607d89edbcd12873
烧录使用下方完整FEL/FES步骤，并以本节hash及同版manifest为准；下方旧hash
仅用于相应历史版本。不要把rootfs、FIT或bootarea单独当作整盘烧录。

软件与实机状态：
- 配置58/58、AIC片段13/13、4G片段16/16；本轮完整增量构建exit 0。
  内核6.12.113、无线backports 6.18.55；未声称另一次干净工具链重建。
- CPU0–3在线，eMMC候选的逐核绑定/并发计算/共享原子数据3秒短测通过。
- AIC8800D80：SDIO实际25MHz，RAM及eMMC Wi-Fi扫描通过；RAM中OpenWrt
  WPA2热点可启动于5GHz/80MHz。客户端认证、连接和吞吐尚未验证。
- UART7补齐64字节FIFO/硬件流控；蓝牙自动HCI UP，完整245字节命令10/10
  通过，BlueZ LE扫描收到广播。连接、配对、音频和低功耗尚未验证。
  r2在拆UART前同步关闭HCI；bluetoothd保持运行时三轮停/启均无超时。
  BlueZ 5.83自带btmgmt存在初始化缺陷，使用已验证的bluetoothctl，详见AIC说明。
- EC20：2c7c:0125、ttyUSB0–3、wwan0/cdc-wdm0、QMI及AT均实测正常。
  AT+CPIN?报告未检测到SIM，尚未拨号或测试流量。ML302驱动/协议包编译
  通过，尚未上板；UCI示例默认不自动拨号，也不替换LAN/WAN。
- 最终r2整盘已FES写入并通过烧录器校验；整盘独立读回SHA一致，eMMC启动且四核在线。
  前一r1候选已经完整读回并从eMMC启动、普通重启，过程记录独立保留。

尚未解决的稳定性问题：
- RTL8211F复位释放等待150ms修正了启动早期能力缓存异常，但集成RAM仍多次
  掉线/降100M。RGMII延迟对照、清除EEE广告，以及保留PHY-mode EEE均未
  根治。未把试验参数固化到镜像。用户确认原厂Tina在同板、同线、同对端、
  同一EC20下稳定千兆，不能简单归因EC20插入或供电。
- SDK现有bImage/vmlinux未启用Realtek专用驱动，走Generic PHY；其SHA尚未
  与用户当时稳定运行的Tina镜像核对，未编入的realtek.c不能当作已执行路径。
- 首轮RAM在多次SDIO恢复/EIR诊断后意外重启，用户确认没有操作板子；未捕获
  明确panic原因，不能排除异常，也不能直接归因为SMP、DDR、AIC或供电。
- r2烧录曾在FES重连阶段失败（未写盘）；保留串口的诊断尝试又被串口独占
  预检拒绝。关闭调试串口、确认FEL后按原流程重试成功，所有尝试均留档。

实现、构建、烧录与测试记录：
  docs/AIC8800D80.txt；docs/4G-EC20-ML302.txt
  docs/WAN-RTL8211F-RESET.txt；docs/records/2026-11-05-wan-reset/
  docs/records/2026-11-05-connectivity/（运行记录与最终交付结果）
  docs/records/2026-11-05-connectivity-final-r2-software/（构建与配置）
  docs/records/2026-11-05-uart-flow-software/（UART补丁/DT检查）
  docs/records/2026-11-05-bt-shutdown-software/（关闭路径与启停验证）
AIC主机驱动是外部GPL源码，radio固件是二进制；FES也有厂商二进制边界，
完整说明见下方“源码和FES的边界”。持久DDR/SPL/U-Boot/内核来自源码构建。

四核 Cortex-A7 实机验证预览（2026-11-05）
-----------------------------------
源码已实现CPU0–3的PSCI启动；四核镜像已FES烧录并完成整盘SHA读回。
Linux实机online=0-3，逐核绑定并行计算/共享数据一致性短测通过；空闲后及普通
重启后复测也通过。源码SPL软件回FEL已由USB枚举验证，随后再次FES烧录并
启动成功，该轮最终运行四核OpenWrt。仍属实验性移植。
下载：https://github.com/dshanpi/t153-openwrt/releases/tag/smp-preview-20261105
选用t153-openwrt25-source-1g-smp-disk.img.gz，搭配同页的
 t153-fes-ddr933-ram-loader.img。四核构建与烧录差异见下方“四核候选使用方法”。
实现、寄存器来源、限制：docs/T153-SMP.txt。
软件证据：docs/records/2026-11-05-smp-software/。
实机证据：docs/records/2026-11-05-smp-bringup/。

已上板的CPU0基线（回退版本）
---------------------------
软件验证预览版下载：https://github.com/dshanpi/t153-openwrt/releases/tag/software-20261104
CPU0基线整盘文件：t153-openwrt25-source-1g-disk.img.gz；同时下载
t153-fes-ddr933-ram-loader.img。SHA256和直接烧录步骤见下文。

2026-11-04软件复跑结果：已从GitHub固定提交f16dc94在独立目录完成完整
OpenWrt/工具链/内核/U-Boot编译、FES组包及17阶段检查，全部exit 0。
本次固件raw SHA256为42df6e8f2755e362eb82c7121671855984cd68c65fea037ac0d3bbcd87e6aad8。
展开配置、来源、工具版本及日志见docs/records/2026-11-04-software-rebuild/
（从docs内阅读时去掉前缀docs/）。该次软件复跑未进行烧录/启动验证；
后续实机结果单独记录。
下文提到尚未完成的“最终全流程”包含硬件验收，不能据软件构建通过宣称已完成。

T153MX OmniGate / OpenWrt 25.12 主线适配
======================================

实验性本地适配，尚未合并OpenWrt/Linux上游。源码基线为官方OpenWrt
openwrt-25.12固定提交6c57f7cd04028d370f9360de71f5d59412046194，
Linux 6.12.113、U-Boot 2026.07。没有使用Tina 5.10内核替代主线内核。

已验证的实机里程碑（2026-11-05，CPU0基线）
-----------------------------------------
全源码1GiB DDR4 SPL → 主线U-Boot → Linux 6.12.113 → eMMC ext4 → OpenWrt。
本次已安装的292MiB镜像raw SHA256：
42df6e8f2755e362eb82c7121671855984cd68c65fea037ac0d3bbcd87e6aad8。
FES写入、校验和post-reboot成功，U-Boot读回整盘SHA精确匹配。
持久rgmii-rxid无需临时调PHY，WAN自动取得DHCP；本次观察1000Mbps Full，
网关5次ping全通。启动时出现过一次重新协商，不能据短测宣称千兆长期稳定。
原生watchdog重启已观察到，但旧SPL没有消费RTC请求，reboot efex仍从eMMC
启动，未进入FEL。新四核镜像已补SPL早期处理，并另行实测成功进入FEL。
证据：docs/records/2026-11-05-bringup/。
11/4首次存储/网络测试和历史故障保留于docs/records/2026-11-04-bringup/。

当前限制
--------
- 物理DDR1GiB，Linux地址窗口仍384MiB。
- 四核已完成启动、调度和共享数据一致性短测；没有完成长期负载/温控验证。
  更早WAN排查缓存中的两次重启，用户已确认是手动操作，保持历史记录。
  后续首轮连接功能RAM另有一次用户确认未操作板子的意外重启，串口未捕获
  明确panic原因；此异常尚未解释，不能宣称长期稳定，亦不能直接归因为SMP。
- 四核仅实现PSCI 0.1 CPU_ON；CPU热插拔、休眠、多核kexec不支持。
- WAN最新对照见docs/WAN-RTL8211F-RESET.txt：150ms修正早期能力缓存异常，
  但后续两轮集成RAM均复现掉线/降100M；临时关闭标准EEE广告后也再次掉线。
  千兆稳定性问题尚未解决，不能将人工重协商的down与自然掉线混为一谈。
  更早延迟/降速排查保留于docs/records/2026-11-05-wan/，不覆盖最新结果。
- DDR/存储长期可靠性、eth0 LAN及CAN实际通信仍需验收。
- AIC8800D80已加入外部GPL SDIO驱动、radio二进制固件和源码蓝牙初始化
  服务；Wi-Fi扫描和RAM热点启动通过，关联/数据传输未验收。UART7流控已修复，
  蓝牙HCI注册/LE发现通过，连接/配对/音频未验收；btmgmt缺陷见AIC文档。
- EC20已确认USB枚举和QMI/AT响应；模块未检测到SIM，尚未注册/拨号/测流量。
  ML302以及两个模块的重插恢复尚未实测；详见docs/4G-EC20-ML302.txt。
- USB0 gadget、CPU DVFS、eMMC高速模式未启用/验证。
- 新SPL的软件回FEL修复已实机成功枚举；旧CPU0固件仍需硬件进入FEL。
- sysupgrade三处拒绝T153写盘；使用完整磁盘FES流程，不通过LuCI升级。
- 11/4完整软件构建通过且对应CPU0镜像11/5已烧录；已发布的
  smp-preview-20261105复用该内核和rootfs，仅重新编译启动链，其构建/烧录/
  运行已分别留证。此历史构建范围不适用于本次连接功能候选，候选状态见开头。

源码和FES的边界
---------------
最终写入eMMC的DDR初始化、SPL、U-Boot和Linux/OpenWrt来自源码构建，
不链接厂商DDR对象，也不依赖厂商monitor。
本次新增的AIC8800D80主机驱动来自固定版本的外部GPL源码，不是Linux上游
驱动；独立Wi-Fi/蓝牙radio固件为厂商二进制，无对应源码。其来源与许可边界
见docs/AIC8800D80.txt及包内FIRMWARE-NOTICE，不能称无线部分全部由源码构建。
临时FES RAM loader仍含用户提供的厂商FES1 DDR和U-Boot二进制；它只用于
烧录，提取/组包/参数更新工具为本仓库Python源码。不能称整个FES链全源码。
原厂SDK镜像不上传；Git源码历史不收录厂商payload。Release另提供本板
933MHz RAM loader和manifest，保留厂商二进制属性及LicenseRef-NOASSERTION，
不会因此变成开源FES实现。固定hash与来源边界见docs/FES-REPRODUCE.txt。
T113参考工程的两条烧录路径及T153对应实现见docs/FLASH-SOURCE.txt。

构建（Ubuntu 24.04）
-------------------
依赖安装、固定来源及详细步骤：docs/BUILD-REPRODUCE.txt。
四核DT检查还需libfdt-dev（已列入该文档安装命令）。
在干净源码checkout根目录运行：
  sh scripts/bootstrap.sh --check-only
  bash scripts/build-release.sh --jobs 4 --smp
不加--smp时生成CPU0回退配置。
同时准备四核镜像和FES RAM loader：
  bash scripts/build-release.sh --jobs 4 --smp --with-fes --factory-image /path/to/factory.img
统一入口编译OpenWrt工具链/内核/选包和1GiB U-Boot，运行软件回归、打包并校验。
默认拒绝旧build_dir/staging/tmp/bin/.config；仅开发增量可加--incremental。
下载缓存可共享，构建对象不可共享，见docs/REBUILD-CACHE.txt。
正常持久固件构建不依赖Tina；仅FES组包需要用户提供固定hash的原厂镜像。
scripts/generate-port.py只供移植开发，不是正常复现步骤。

产物与烧录
----------
artifacts/t153-openwrt25-source-1g-disk.img及.img.gz为本板完整292MiB镜像。
t153-disk-1g-manifest.json绑定本次产物大小、布局和SHA，不用历史hash代替。
它包含源码SPL/U-Boot、32MiB FAT boot和256MiB ext4，覆盖eMMC前292MiB，
不沿用原厂A/B分区。rootfs.ext4.gz、FIT及bootarea不能单独当作整盘烧录。
旧512MiB profile、RAM-chainload及默认FEL cpio为独立实验路径，不用于此板。

完整烧录步骤：docs/FLASH-SOURCE.txt。
无私人会话的MCP下载/烧录/轮询示例：host-tools/README.txt。
实机FES后的整盘SHA、Linux启动、网络和外设必须与本次产物分别验收。
默认LAN地址192.168.153.1/24；WAN DHCP；从eMMC CID派生MAC。

四核候选使用方法
----------------
下载smp-preview-20261105发布页中的以下文件：
  t153-openwrt25-source-1g-smp-disk.img.gz
  t153-disk-1g-smp-manifest.json
  t153-fes-ddr933-ram-loader.img
  t153-fes-ddr933-ram-loader.manifest.json
  SHA256SUMS
核对下载附件：sha256sum -c SHA256SUMS --ignore-missing
解压：gzip -dk t153-openwrt25-source-1g-smp-disk.img.gz
raw大小306184192字节，SHA256：
  ad406a8489440386e0039bbd0a3655df2a475087dc3f9736eef5b1291b7177f6
.gz SHA256：
  e11a46b7e7a2837db645f51f07c16e045faf2a071c15d9f8f62d839a5333a775
FES SHA256仍为下方0d32d521...，不使用源码的-smp-fel.bin替代FES loader。

遵循下方同一套1–5步烧录操作，将磁盘下载名/firmwarePath/读回预期SHA
全部改为本节四核文件和值；FES loaderPath及raw/command/verify/reboot不变。
不要把CPU0的42df...读回值用于本四核候选。旧固件仍需硬件进入FEL。

四核启动后应检查：
  cat /sys/devices/system/cpu/online           期望0-3
  grep '^processor' /proc/cpuinfo             期望4项
  cat /proc/interrupts                       期望CPU0–CPU3列
把同页t153-smp-smoke经网络复制到/tmp并按manifest核对SHA后：
  chmod 755 /tmp/t153-smp-smoke
  /tmp/t153-smp-smoke
  echo $?
要求4行CPU PASS、共享计数一致、RESULT PASS以及退出码0；仅显示4个CPU节点
不算并行验证。空闲后再测、普通重启后再测；同时回归eMMC、WAN和软件回FEL。
源码与判据：tests/smp/RUNTIME-SMOKE.txt。CPU0负对照会正确失败；四核镜像
首次启动、空闲后、正常重启后正向结果均为PASS，见实机记录。软件构建manifest
保留hardware_verified=false，表示该离线构建流程不访问板卡；实际成功证据
另以本raw SHA绑定，不能由manifest中的cpu_count=4单独推断硬件成功。

FEL/FES烧录步骤（本板1GiB DDR4 / eMMC）
---------------------------------------
发布页：https://github.com/dshanpi/t153-openwrt/releases/tag/software-20261104
准备同一Release中的以下文件：
  t153-openwrt25-source-1g-disk.img.gz       完整主线磁盘固件
  t153-fes-ddr933-ram-loader.img            933MHz临时FES引导
  t153-disk-1g-manifest.json                磁盘布局及组件hash
  t153-fes-ddr933-ram-loader.manifest.json   FES来源、参数及payload hash
  SHA256SUMS                              Release附件校验值
FES引导只驻留RAM，不能单独当作eMMC系统镜像；完整磁盘内已包含源码DDR/SPL、
U-Boot、内核和rootfs。不要用t153-fes-ram-loader.img（旧1066MHz）或physical0。

1. 下载、解压并校验
   Linux下将下载的文件放在同一目录，执行：
     sha256sum t153-openwrt25-source-1g-disk.img.gz t153-fes-ddr933-ram-loader.img
     gzip -dk t153-openwrt25-source-1g-disk.img.gz
     sha256sum t153-openwrt25-source-1g-disk.img
   Windows可用7-Zip解压.gz；PowerShell用Get-FileHash -Algorithm SHA256核对文件。
   解压后的.img应为306184192字节（292MiB），固定SHA256为：
     42df6e8f2755e362eb82c7121671855984cd68c65fea037ac0d3bbcd87e6aad8
   .img.gz SHA256：
     76291858f55c74084c795e253d20e102ba5c85a3b66804f5b60a64968a1c7dce
   FES引导应为1154048字节，SHA256：
     0d32d52139bc969f7f90665d32203ff07366ed0a31139bda607d89edbcd12873
   下面的Allwinner流程使用解压后的.img；不要套用其它芯片的压缩包参数。

2. 进入FEL并确认目标
   保留需要的旧系统数据，移除可能带相同PARTUUID的SD卡。写入会覆盖eMMC
   前292MiB和原厂分区表，不保留原厂A/B布局。
   使用板卡的FEL硬件入口进入BootROM FEL，USB数据口连烧录主机。
   常见方式是按住FEL键后复位/重新上电，具体按键以本板硬件定义为准。
   MCP客户端连接已绑定本板的LYNX端点（本次环境http://127.0.0.1:8771/mcp），
   正常完成initialize/initialized，然后依次调用：
     lynx_workbench_status {}
     lynx_flash_capabilities {}
     lynx_scan_flash_devices {}
   确认目标绑定正确，T153的SoC ID为0x1922，且raw/command/verify/reboot受支持。
   通道显示名称不等于实际芯片；扫描未发现FEL时不要启动烧录任务。

3. 让烧录工具独占串口并准备两个本地绝对路径
   关闭其它串口终端；若MCP已开串口，用lynx_serial_close关闭自己的句柄。
   烧录预检会使用本板串口读取eMMC几何信息，任务成功之前不要自行open/read。
   若文件已在LYNX所在电脑，直接使用校验过的.img和FES引导绝对路径。
   若文件在Linux构建服务器，分别调用lynx_download_firmware：
     {"remotePath":"<ARTIFACTS_ABSOLUTE_PATH>/t153-fes-ddr933-ram-loader.img"}
     {"remotePath":"<ARTIFACTS_ABSOLUTE_PATH>/t153-openwrt25-source-1g-disk.img"}
   多项目端点需额外填写正确projectId；单项目设备端点可按服务能力省略。
   每次下载后用lynx_workbench_task_status {"taskId":"<实际返回的ID>"}
   轮询到success，核对返回的大小/SHA，保存实际localPath，不能推测缓存路径。
   用lynx_inspect_firmware {"image_path":"<FES_LOCAL_PATH>"}确认临时loader。

4. 执行整盘烧录
   调用lynx_start_flash，arguments为：
     {
       "firmwarePath": "<DISK_LOCAL_PATH，解压后的.img绝对路径>",
       "confirmDestructive": true,
       "options": {
         "mode": "command",
         "verify": true,
         "postFlashAction": "reboot",
         "vendorOptions": {
           "raw": true,
           "loaderPath": "<FES_LOCAL_PATH，933MHz引导绝对路径>"
         }
       }
     }
   loader保留stock +40960扇区映射，由当前OpenixCLI raw路径补偿；禁止再叠加
   physical0，否则会重复抵消地址。只向本板eMMC使用上面的raw/command流程。
   轮询同一个taskId，不重复start_flash。失败或取消时保存状态并停止。
   成功应同时有state=success、exitCode=0、verifyState=success、
   writtenBytes=committedBytes=306184192；postFlashState也应报告重启成功。
   transferredBytes或progressPct=100本身不等于校验成功。

5. 启动与读回验证
   烧录任务成功并释放串口后，再以115200、8N1打开本板UART0，保存SPL/U-Boot
   和Linux日志。FES post-reboot是软件重启，不是物理断电测试。
   若要整盘SHA复核，在主线U-Boot三秒倒计时中断；先确认编号1为本板eMMC：
     mmc dev 1
     if mmc read 48000000 0 92000; then hash sha256 48000000 12400000; fi
   92000为598016扇区，12400000为292MiB，均为十六进制。结果须等于上面的raw SHA。
   然后run t153boot启动；确认源码SPL/U-Boot、Linux、eMMC ext4根分区和OpenWrt
   shell，核对本轮网络及重启功能。默认LAN 192.168.153.1/24，WAN DHCP。
   必须把实际烧录/启动记录绑定该镜像hash，不能用旧镜像成功替代本轮验收。

完整JSON-RPC请求及响应判据见host-tools/mcp-fes-workflow.example.json；
细节见docs/FLASH-SOURCE.txt。新四核镜像的软件回FEL已验证，可在真实Linux
shell下通过MCP调用：
  lynx_allwinner_enter_fel {
    "method":"serial_shell_reboot_efex", "handle":"<实际串口句柄>",
    "confirmDestructive":true, "preserveSerialMonitor":true
  }
要求state=fel-device-online且felOnline=true，并记录SPL的
RTC EFEX request cleared; entering BootROM FEL。失败时不自动重试。
成功后关闭自己的串口句柄，再按第3–5步烧录；不要同时启动串口读者。
旧CPU0基线不具备这份SPL修复，不能套用其成功结论。本README不要求手工写
寄存器、跳BootROM或绕过工具限制。

目录
----
port/：Linux/OpenWrt及U-Boot源码覆盖层；configs/：配置与固定来源。
scripts/：准备、构建、组包、检查；tests/：软件回归；host-tools/：通用MCP示例。
docs/：架构、烧录、限制及精选实机记录。下载、上游展开树、编译对象、日志和
固件产物均不加入源码历史。源码和编译只在Linux服务器，不在Windows复制工程。
架构：docs/ARCHITECTURE.txt；状态：docs/STATUS.txt；检查：docs/VALIDATION.txt。

目标源码仓库：https://github.com/dshanpi/t153-openwrt。
独立干净完整编译由该仓库固定提交复跑；软件与实机状态分别见构建记录和docs/STATUS.txt。
