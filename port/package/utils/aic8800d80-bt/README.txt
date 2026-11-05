AIC8800D80 UART Bluetooth helper for T153
=======================================

This package initializes the controller over UART7 and attaches it to the
mainline Linux HCI UART H4 driver. It does not contain a vendor kernel Bluetooth
driver. The AIC SDIO BSP/FDRV must first upload the AIC8800D80 Wi-Fi and Bluetooth
ADID/patch/table firmware. Those device firmware blobs are separate from this
source-built GPL-2.0-or-later helper; see the aic8800-sdio firmware notice.

Hardware and DT requirements
----------------------------
- UART7 PB11/PB12/PB13/PB14, 1,500,000 baud, 8N1, hardware RTS/CTS; /dev/ttyS7.
- A generic rfkill-gpio node with label t153-aic8800d80-bt, radio-type bluetooth,
  shutdown-gpios on PB0. Its polarity must match the fitted AIC module.
  The supplied SDK drives PB0 low when Bluetooth is unblocked; equivalent
  generic rfkill-gpio wiring uses GPIO_ACTIVE_LOW. See HARDWARE-NOTES.txt.
- PJ11 GPIO output held high to keep the device awake.
- PB4 host-wake interrupts and suspend/wake are not implemented in this initial
  integration. Do not enable low-power operation until that path is implemented
  and validated. Device power consumption is higher while wake is held high.

The init service first loads aic8800_fdrv, hci_uart and rfkill_gpio, then finds
only the rfkill device with the specified label and Bluetooth type. The helper
claims the UART exclusively, blocks Bluetooth for 100ms, unblocks it, waits 500ms, and
initializes the controller. Every HCI transaction has a three-second deadline;
responses must have a matching opcode and success status before proceeding.

Vendor initialization sequence
------------------------------
0x0c03 HCI Reset
0xfc70 Write BD_ADDR (six bytes in reversed HCI byte order)
0xfc4d Write 104-byte AON parameter block, explicitly little-endian
0xfc50 Set LP level = 0 (active)
0xfc51 Set power-control-slave = 1
0xfc52 Set CPU power-off = 0

Command IDs, AON parameter values, 1.5M UART setting and command order follow:
T153 Tina SDK buildroot/buildroot-202205/package/bluez5_utils/
012-add-aw869b-hciattach-support.patch
Author: luoyuyi <luoyuyi@allwinnertech.com>, 2021-08-24.
Patch source commit header: 0b46f0afe6c311c8abeda9dfc6c8defa6950a817.
The new code independently implements I/O and serialization. The original SDK
uses LP level 2 and CPU power-off 1 with its private btlpm driver; this package
intentionally uses 0 and 0 because that low-power handshake is not integrated.
The original 104-byte AON ABI includes two final zero padding bytes; no host
structure layout or endianness is assumed by the encoder.

Address selection
-----------------
The default address=auto hashes the device-tree serial-number, SoC serial_number,
MMC CID, or unique /etc/machine-id using a versioned FNV-1a identifier hash and sets the
locally administered/unicast bits. This is not an official assigned Bluetooth
OUI or a privacy mechanism. The board identifier must be unique. If MMC CID is used, replacing that storage
changes the derived address. All-zero and
missing identifiers are rejected. No common fallback address is used.
If the image has no unique board identity, configure an explicitly assigned
unique address, for example (replace the sample with your assigned value):
  uci set aic8800d80-bt.main.address='02:xx:xx:xx:xx:xx'
  uci commit aic8800d80-bt
  /etc/init.d/aic8800d80-bt restart
Do not clone a fixed /etc/machine-id across boards.

Lifetime and verification
-------------------------
The helper remains in the foreground under procd to keep the HCI line discipline
open. Stop sends SIGTERM, restores N_TTY, and blocks Bluetooth via rfkill. Rapid
initialization failures are retried up to five times and logged by logread.
Stop this service before manually unloading aic8800_fdrv/aic8800_bsp. WLAN and
Bluetooth share a firmware/power lifetime; wireless interface down does not
mean it is safe to unload the shared modules while Bluetooth is running.

Runtime checks (board validation required):
  logread -e aic8800d80
  cat /sys/class/rfkill/rfkill*/name
  hciconfig -a
  bluetoothctl list
  bluetoothctl show
  bluetoothctl power on
  bluetoothctl scan on
Then test a real device pairing/connection, Wi-Fi and Bluetooth concurrently,
service stop/start, and reboot. Controller registration alone is not sufficient
proof of radio operation. No real-board validation is claimed by this package.

Host tests (no board required):
  cc -std=c11 -Wall -Wextra -Werror tests/test-helper.c -o /tmp/aic-bt-test
  /tmp/aic-bt-test
Tests compare the complete 104-byte payload, reject malformed addresses, and
simulate HCI command success, mismatch, error, truncated response and timeout.
