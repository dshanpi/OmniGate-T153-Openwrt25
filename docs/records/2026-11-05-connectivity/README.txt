T153 connectivity bring-up, 2026-11-05

Initial RAM FIT: f6259f6145e9fb0ddb713805b683f3d3dbb8d99dcdf9c225d7562313906c8c6d
Built incrementally from the 58-option board profile, Linux 6.12.113 / wireless backports 6.18.55.
Not a clean rebuild, and this initial image is not the final flashed candidate.

Initial RAM observations:
- CPUs 0-3 online.
- SDIO actual clock 25 MHz; Wi-Fi scan succeeded with seven BSS records.
- netifd/hostapd enabled a temporary WPA2 AP at 5 GHz channel 36, 80 MHz.
  No client association or throughput test was performed. Test config was reverted.
- EC20 option/QMI binding and online mode succeeded; no SIM was detected in prior AT probe.
- WAN carrier_changes=1 through uptime 16 minutes; 16 MiB download SHA256 matched; RX/TX errors zero.
  This does not resolve the previous unexplained three link drops near EC20 insertion.
- Bluetooth failed intermittently at HCI opcode 0x0c52 (245-byte H4 frame).
  One complete initialization succeeded; repeated cold SDIO recovery reproduced timeout.
- A standard TTY ioctl reported FIFO=16, baud=1500000, CRTSCTS=true, zero RX framing/overrun errors.
  Whole-frame EIR probe failed; 32-byte/1ms chunks passed after separate SDIO reset.
  These are pre-fix diagnostics, not Bluetooth acceptance.

Privacy: public text logs redact MAC/Bluetooth addresses. Nearby SSID/BSSID discovery output is not archived.
The raw btsnoop capture is retained only in ignored logs/, SHA256 bb7dd34416a111bfb0ec3e35c746f462937c2cecc8cec69d2e28a8f659b93f6b.
