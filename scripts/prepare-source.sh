#!/bin/sh
set -eu
port_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
revision=6c57f7cd04028d370f9360de71f5d59412046194
sha256=3444d6132465dc4c8442d6f3c946752c7d71701c6f0f52529aa2b0ecb32e2efe
openwrt_dir=${1:-"$port_dir/openwrt"}
if [ -e "$openwrt_dir" ]; then
    if [ -f "$openwrt_dir/.upstream-commit" ] && [ "$(cat "$openwrt_dir/.upstream-commit")" = "$revision" ]; then
        printf '%s\n' "$revision" > "$openwrt_dir/version"
        printf '%s\n' "Using existing pinned source: $openwrt_dir"
        exit 0
    fi
    echo "Destination exists without expected revision marker: $openwrt_dir" >&2
    exit 1
fi
mkdir -p "$port_dir/downloads"
archive="$port_dir/downloads/openwrt-6c57f7c.tar.gz"
if [ ! -f "$archive" ]; then
    curl -fL --retry 4 --connect-timeout 30 "https://codeload.github.com/openwrt/openwrt/tar.gz/$revision" -o "$archive.part"
    printf '%s  %s\n' "$sha256" "$archive.part" | sha256sum -c -
    mv "$archive.part" "$archive"
fi
printf '%s  %s\n' "$sha256" "$archive" | sha256sum -c -
mkdir -p "$openwrt_dir"
tar -xzf "$archive" --strip-components=1 -C "$openwrt_dir"
printf '%s\n' "$revision" > "$openwrt_dir/.upstream-commit"

# Source archives have no Git metadata; APK needs a valid hexadecimal revision.
printf '%s\n' "$revision" > "$openwrt_dir/version"
