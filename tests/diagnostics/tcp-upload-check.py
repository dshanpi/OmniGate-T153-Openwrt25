#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Bounded TCP receiver for board-to-host payload verification; no shell commands."""
import argparse
import hashlib
import json
from pathlib import Path
import socket
import time
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('reference', type=Path)
p.add_argument('--bind', default='127.0.0.1')
p.add_argument('--port', type=int, default=8879)
p.add_argument('--count', type=int, default=4)
p.add_argument('--timeout', type=float, default=90)
a = p.parse_args()
if not (1 <= a.count <= 16 and 1 <= a.timeout <= 120):
    p.error('count must be 1..16; timeout must be 1..120 seconds')
size = a.reference.stat().st_size
if not (1 <= size <= 64 * 1024 * 1024):
    p.error('reference must be 1 byte..64 MiB')
with a.reference.open('rb') as source:
    expected = hashlib.file_digest(source, 'sha256').hexdigest()
with socket.socket() as server:
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((a.bind, a.port))
    server.listen(1)
    server.settimeout(a.timeout)
    print(json.dumps({'ready': True, 'bytes': size, 'sha256': expected}), flush=True)
    for seq in range(a.count):
        conn, _ = server.accept()
        with conn:
            conn.settimeout(a.timeout)
            start = time.monotonic()
            digest = hashlib.sha256()
            received = 0
            while received < size:
                remaining = a.timeout - (time.monotonic() - start)
                if remaining <= 0:
                    raise TimeoutError('upload deadline exceeded')
                conn.settimeout(remaining)
                chunk = conn.recv(min(size - received, 65536))
                if not chunk:
                    break
                received += len(chunk)
                digest.update(chunk)
            seconds = time.monotonic() - start
            result = {'round': seq + 1, 'bytes': received, 'sha256': digest.hexdigest(),
                      'seconds': round(seconds, 6), 'ok': received == size and digest.hexdigest() == expected}
            print(json.dumps(result), flush=True)
            conn.sendall((json.dumps(result) + '\n').encode())
            if not result['ok']:
                raise SystemExit(1)
