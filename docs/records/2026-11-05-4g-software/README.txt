2026-11-05 EC20/ML302 软件配置核对
==================================
evidence.json绑定所查Linux 6.12.113驱动源文件和四个UCI示例的SHA256。
config-check.log为构建负责人执行defconfig后的实际16项选择保留结果。
四个UCI例子使用已构建的ARM目标uci，经qemu-arm-static在临时目录解析并合并，
确认原LAN/WAN接口保留且cellular自动启动关闭；没有访问板卡或调制解调器。
新增驱动为现有qcserial与rndis_host，不是自制私有USB驱动；未追加宽泛USB ID。
后续板级验证已确认用户接入EC20的USB枚举(2c7c:0125)、option串口和qmi_wwan
网卡/控制节点，以及QMI和AT只读控制通道。当前online但not_registered；随后
AT+CPIN?返回+CME ERROR:10（SIM not inserted），模块未检测到SIM。该结论来自
AT明确响应，不能由QMI的Not supported或空对象推断。CGMM=EC20F，
CGMR=EC20CEHDLGR08A03M1G。没有PIN尝试、持久设置、IMEI/ICCID查询或拨号。
ML302实机、SIM注册、拨号、数据传输和重插恢复未验证。板测由root操作；
此软件配置审核任务未直接访问板卡。原始日志归档状态见evidence.json。
软件包编译状态以evidence.json的package_build字段为准。
具体操作、模式边界和配置：../../4G-EC20-ML302.txt。
