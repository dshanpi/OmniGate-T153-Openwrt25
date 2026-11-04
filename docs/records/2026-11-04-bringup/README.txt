2026-11-04 T153 1GiB bring-up evidence
====================================
This is a milestone record, not final full-platform acceptance.
See hardware-status.json for the image-bound observed results and remaining items.
Serial logs remove CR/ANSI display escapes only; original and published hashes
are recorded in serial-log-provenance.json. FES status includes selected result
fields and excludes local host executable/firmware paths.
Earlier software-only JSON records retain the status they had when generated;
they must not override later image-bound hardware observations.
The first flash's two bit differences remain historical evidence and do not
invalidate the later exact readback, nor establish the original fault cause.

Additional network/storage logs preserve serial CR/ANSI formatting; private host
addresses were replaced with role placeholders. Their source and published hashes
are in network-storage-provenance.json. The 100 Mbps DHCP/HTTP test used a runtime
PHY RX-delay change on the installed 6ea image. It does not prove the newer
persistent rgmii-rxid/native reboot image booted. Gigabit remained unstable.
The board/debugger was then disconnected; recovery power GPIO failed.
