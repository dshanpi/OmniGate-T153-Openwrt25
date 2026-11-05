LYNX MCP 标准 FES 烧录请求示例
==============================

mcp-fes-workflow.example.json 保存本项目1GiB T153/eMMC流程的完整 JSON-RPC
请求；lynx-fes-tool-schemas.json 保存集成时观察到的六个工具输入schema。
这些文件是可审阅的请求数据，不会自动连接设备或执行烧录。
不含私人session、token、SSH密码或用户固件缓存目录，也不依赖私有helper。

前提
----
1. 由正常MCP客户端连接实际LYNX服务并完成initialize/initialized；协议版本、
   HTTP session及认证由客户端管理，不复制他人会话。示例endpoint为本机
   http://127.0.0.1:8771/mcp，只有确实运行该服务时才使用。
2. 调用当前服务tools/list和lynx_flash_capabilities，确认工具仍接受示例参数、
   目标是所需T153、raw/command/verify及reboot受支持。保存的schema不是
   服务当前能力的替代品。若接口或目标不匹配，停止并更新流程。
3. 板卡应已进入BootROM FEL；lynx_scan_flash_devices应只找到绑定的目标。
   本板实测raw chip=0x00192200/SoC=0x1922，不能凭旧通道名称判断芯片。
4. 先在Linux构建机完成打包和verify-only。使用1GiB profile的整盘镜像，
   不把rootfs、512MiB bootarea或FIT当作完整磁盘。

逐步调用
--------
把<REMOTE_ROOT>替换为本次Linux源码/产物目录的绝对路径。每条request是
一次tools/call请求；若框架直接提供工具调用，传相同name与arguments即可。
响应中的任务ID和LYNX本地缓存路径必须来自本轮真实返回值，不手工推测路径。
只有device-scoped服务且绑定恰好一个SDK项目时可省略projectId；其它情况
在两次download请求中显式填写正确项目ID。

烧录预检须独占本板串口：关闭其它串口终端及自己创建的MCP句柄，任务成功后
再open/read采集启动日志。提前占用串口会使存储几何预检失败。

顺序：capabilities → scan → 下载loader → 轮询下载 → 检查loader →
下载disk → 轮询下载 → start_flash → 轮询flash。
JSON中的check/require/save是客户端必须执行的检查规则，不是发送给工具的参数。
轮询每次使用新JSON-RPC id；按任务状态间隔轮询，不重复start_flash。
工具返回isError、任务失败/取消或hash/长度不匹配时停止，不自动重刷。

loader固定SHA256：
  0d32d52139bc969f7f90665d32203ff07366ed0a31139bda607d89edbcd12873
大小1154048字节。六个RAM-only角色为mbr-placeholder、sys-config、
board-config、dtb-config、fes1、uboot。禁止包含要持久烧录的厂商内核/根分区。
组包和六payload的SHA校验见docs/FES-REPRODUCE.txt及对应configs。

整盘SHA必须取本次t153-disk-1g-manifest.json的image.sha256，并与本轮
download结果核对；不要拿历史成功SHA代替新构建产物。现布局306184192字节，
598016个512字节扇区；若将来布局改变，先同步示例检查值和验收读回范围。
start_flash的confirmDestructive=true表示调用方已确认将覆盖选定eMMC前292MiB，
包括原厂分区表；它不是读取/预览参数。

本例使用raw=true、mode=command、verify=true及postFlashAction=reboot。
reboot是现有记录实际跑通的FES后续引导方式，尤其适合外部继电器不可用时；
必须先确认当前capabilities支持它。none只会留在FES，并不自动启动主线系统，
只有已经准备好可验证的后续引导/恢复方法时才改用none。FES post-reboot
不是新一轮物理断电测试，也不证明Linux原生reboot efex已经通过。

保持原厂FES U-Boot的+40960扇区映射，由当前host raw路径补偿。
禁止再叠加旧physical0补丁，否则会双重修正。详见FES-RAW-MAPPING-CORRECTION.txt。

完成判定
--------
任务必须同时state=success、exitCode=0、verifyState=success，且writtenBytes
和committedBytes等于306184192。保存原始最终任务状态和manifest/hash。
这只完成FES层写入/校验；还必须从源码SPL/U-Boot启动，在主线U-Boot
读取全部598016扇区并计算SHA256，与该镜像逐字节一致后，再验收Linux、
ext4根分区、配置持久化、网络和所需外设。不得用FES加法校验代替全盘SHA。
完整步骤及U-Boot命令见docs/FLASH-SOURCE.txt。

本例不包含未验证的kexec或私自绕过工具限制的串口命令。Linux的标准
reboot efex源码实现见docs/T153-LINUX-REBOOT.txt，必须先启动包含该内核、
DT和用户态的系统，并实测USB重新枚举为FEL后才能标为通过。
