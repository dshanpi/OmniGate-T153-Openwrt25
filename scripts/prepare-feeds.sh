#!/bin/sh
# Pinned archives are shared; extracted feed sources remain per-checkout.
set -eu
port_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
openwrt_dir=$(CDPATH= cd -- "${1:?OpenWrt source directory required}" && pwd)
cache_dir=${FEED_CACHE:-"$port_dir/feed-sources"}
archive_dir=${FEED_DOWNLOAD_CACHE:-"$port_dir/downloads/feeds"}
mkdir -p "$cache_dir" "$archive_dir"
cache_dir=$(CDPATH= cd -- "$cache_dir" && pwd)
archive_dir=$(CDPATH= cd -- "$archive_dir" && pwd)
check_archive() {
    if ! printf '%s  %s\n' "$1" "$2" | sha256sum -c -; then
        echo "Feed archive SHA256 mismatch; refusing input: $2" >&2
        return 1
    fi
}
conf="$openwrt_dir/feeds.conf.t153"
: > "$conf"
while read -r feed revision sha256; do
    [ -n "$feed" ] || continue
    dest="$cache_dir/$feed-$revision"
    archive="$archive_dir/$feed-$revision.tar.gz"
    legacy="$cache_dir/$feed-$revision.tar.gz"
    # Migrate only a verified old archive; never share the extracted source tree.
    if [ ! -e "$archive" ] && [ -f "$legacy" ]; then
        check_archive "$sha256" "$legacy"
        if ! ln "$legacy" "$archive" 2>/dev/null; then
            cp -p "$legacy" "$archive"
        fi
    fi
    if [ -e "$archive" ]; then
        check_archive "$sha256" "$archive"
    else
        archive_part=$(mktemp "$archive_dir/.$feed-$revision.XXXXXX.part")
        if ! curl -fL --retry 4 --connect-timeout 30 \
                "https://codeload.github.com/openwrt/$feed/tar.gz/$revision" -o "$archive_part"; then
            echo "Failed to download pinned feed archive: $feed $revision" >&2
            rm -f -- "$archive_part"
            exit 1
        fi
        if ! check_archive "$sha256" "$archive_part"; then
            rm -f -- "$archive_part"
            exit 1
        fi
        if ! tar -tzf "$archive_part" >/dev/null; then
            echo "Invalid feed archive: $feed $revision" >&2
            rm -f -- "$archive_part"
            exit 1
        fi
        mv "$archive_part" "$archive"
    fi
    if [ -f "$dest/.t153-complete" ]; then
        if [ "$(cat "$dest/.t153-complete")" != "$revision" ]; then
            echo "Feed source marker differs from locked revision: $dest" >&2
            exit 1
        fi
    else
        if [ -e "$dest" ]; then
            echo "Refusing existing unmarked feed directory: $dest" >&2
            exit 1
        fi
        extract_dir=$(mktemp -d "$cache_dir/.$feed-$revision.XXXXXX")
        tar -xzf "$archive" --strip-components=1 -C "$extract_dir"
        printf '%s\n' "$revision" > "$extract_dir/.t153-complete"
        mv "$extract_dir" "$dest"
    fi
    printf 'src-link %s %s\n' "$feed" "$dest" >> "$conf"
done < "$port_dir/configs/feeds.lock"
if [ -f "$openwrt_dir/feeds.conf" ] && ! cmp -s "$conf" "$openwrt_dir/feeds.conf"; then
    cp -p "$openwrt_dir/feeds.conf" "$openwrt_dir/feeds.conf.before-t153.$(date +%Y%m%d%H%M%S)"
fi
mv "$conf" "$openwrt_dir/feeds.conf"
