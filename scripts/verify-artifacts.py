#!/usr/bin/env python3
"""Inspect the actual OpenWrt FIT, embedded cpio, configuration and rootfs."""
from pathlib import Path
import gzip, hashlib, json, lzma, struct, subprocess, sys, tarfile, zlib


def require(condition, message):
    if not condition:
        raise SystemExit('FAIL: '+message)


def fdt_nodes(data):
    require(len(data)>=40,'FDT header truncated')
    magic,total,off,strings=struct.unpack_from('>4I',data)
    require(magic==0xd00dfeed and total<=len(data),'FDT magic/size')
    nodes={};stack=[]
    while off<total:
        token,=struct.unpack_from('>I',data,off);off+=4
        if token==1:
            end=data.index(b'\0',off,total)
            stack.append(data[off:end].decode());off=(end+4)&~3
            nodes['/'.join(stack) or '/']={}
        elif token==2:
            require(bool(stack),'FDT unmatched END_NODE');stack.pop()
        elif token==3:
            length,nameoff=struct.unpack_from('>2I',data,off);off+=8
            require(off+length<=total,'FDT property bounds')
            start=strings+nameoff;end=data.index(b'\0',start,total)
            name=data[start:end].decode()
            nodes['/'.join(stack) or '/'][name]=data[off:off+length]
            off=(off+length+3)&~3
        elif token==4:
            pass
        elif token==9:
            require(not stack,'FDT unclosed nodes');return nodes
        else:
            raise SystemExit(f'FAIL: FDT token {token}')
    raise SystemExit('FAIL: FDT missing END')


def cpio_entries(data):
    result={};off=0
    while off+110<=len(data):
        require(data[off:off+6] in [b'070701',b'070702'],'newc cpio header')
        fields=[int(data[off+6+i*8:off+14+i*8],16) for i in range(13)]
        size=fields[6];namesize=fields[11];off+=110
        name=data[off:off+namesize-1].decode();off=(off+namesize+3)&~3
        if name=='TRAILER!!!':return result
        require(off+size<=len(data),'cpio file bounds')
        result[name.removeprefix('./').lstrip('/')]=data[off:off+size]
        off=(off+size+3)&~3
    raise SystemExit('FAIL: missing cpio trailer')


def text(value):
    return value.rstrip(b'\0').decode()


def check_root(files,label):
    require(b'dshanpi,t153mx-omnigate' in files.get('usr/sbin/t153-report',b''),label+' board report tool')
    require(b't153-6c57f7cd04' in files.get('etc/openwrt_release',b''),label+' pinned OpenWrt version')
    network=files['etc/board.d/02_network'].decode()
    upgrade=files['lib/upgrade/platform.sh'].decode()
    require('192.168.153.1' in network and 'dshanpi,t153mx-omnigate' in network,label+' network')
    require('device/type' in network and 'macaddr_generate_from_mmc_cid "${mmc_path##*/}"' in network,label+' eMMC identity selection')
    for name in ['platform_check_image','platform_copy_config','platform_do_upgrade']:
        body=upgrade.split(name+'() {',1)[1].split('\n}',1)[0]
        require('dshanpi,t153mx-omnigate)' in body and 'return 1' in body,label+' upgrade guard '+name)
    modules=[name for name in files if name.endswith('.ko')]
    for mod in ['m_can','m_can_sun8i_t153','qmi_wwan','cdc_mbim','option']:
        matches=[name for name in modules if Path(name).stem==mod]
        require(len(matches)==1,label+' missing module '+mod)
        elf=files[matches[0]]
        require(elf[:6]==b'\x7fELF\x01\x01' and struct.unpack_from('<H',elf,18)[0]==40,label+' ARM module '+mod)
    require(any(b'm_can_sun8i_t153' in content for name,content in files.items() if name.startswith('etc/modules.d/')),label+' CAN autoload')
    for binary in ['candump','cansend','canfdtest','uqmi','umbim']:
        require(any(Path(name).name==binary for name in files),label+' missing '+binary)
    require(any(name.startswith('www/luci-static/') for name in files),label+' LuCI assets')


def main():
    port=Path(__file__).resolve().parents[1]
    owrt=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else port/'openwrt'
    subprocess.run([sys.executable,str(port/'scripts/check-config.py'),str(port/'configs/t153mx-omnigate.config'),str(owrt/'.config')],check=True)
    out=owrt/'bin/targets/sunxi/cortexa7'
    fits=list(out.glob('*dshanpi_t153mx-omnigate-initramfs-kernel.bin'))
    require(len(fits)==1,'exactly one T153 initramfs FIT must exist')
    fit=fits[0];nodes=fdt_nodes(fit.read_bytes())
    cfg=nodes['/configurations'];chosen=nodes['/configurations/'+text(cfg['default'])]
    kernel_path='/images/'+text(chosen['kernel']);dt_path='/images/'+text(chosen['fdt'])
    kernel=nodes[kernel_path];dt=nodes[dt_path]
    require(text(kernel['arch'])=='arm' and text(kernel['os'])=='linux','ARM Linux FIT')
    require(text(kernel['compression'])=='gzip','FIT gzip kernel')
    require(struct.unpack('>I',kernel['load'])[0]==0x48008000,'kernel load address')
    require(kernel['load']==kernel['entry'],'kernel entry equals load')
    for path in [kernel_path,dt_path]:
        payload=nodes[path]['data'];hashes=0
        for child,props in nodes.items():
            if child.startswith(path+'/hash'):
                algo=text(props['algo'])
                actual=struct.pack('>I',zlib.crc32(payload)&0xffffffff) if algo=='crc32' else hashlib.new(algo,payload).digest()
                require(actual==props['value'],'FIT hash '+child);hashes+=1
        require(hashes>=2,'FIT missing hashes '+path)
    zimage=gzip.decompress(kernel['data'])
    require(struct.unpack_from('<I',zimage,0x24)[0]==0x016f2818,'ARM zImage header')
    require(len(zimage)<0x2000000,'zImage exceeds factory BOOTM decompression limit')
    require(0x50000000+fit.stat().st_size<0x58000000,'FIT exceeds staging address budget')
    board=fdt_nodes(dt['data'])
    require(b'dshanpi,t153mx-omnigate\0' in board['/']['compatible'],'T153 board compatible')
    require(struct.unpack('>II',board['/memory@40000000']['reg'])==(0x40000000,0x20000000),'512MiB DT memory')
    for name,address,size in [('remoteproc@42100000',0x42100000,0x4c5000),('secure-monitor@42900000',0x42900000,0x100000),('remoteproc@44000000',0x44000000,0x200000)]:
        require(struct.unpack('>II',board['/reserved-memory/'+name]['reg'])==(address,size),'firmware reserved '+name)
    linuxdirs=list(owrt.glob('build_dir/target-*/linux-sunxi_cortexa7/linux-6.12.113'))
    require(len(linuxdirs)==1,'final OpenWrt Linux 6.12.113 directory')
    linux=linuxdirs[0];kdir=linux.parent
    require(zimage==(kdir/'zImage-initramfs').read_bytes(),'FIT payload equals OpenWrt initramfs zImage')
    kconfig=(linux/'.config').read_text()
    for symbol in ['SUN8I_T153_CCU','PINCTRL_SUN8I_T153','DWMAC_SUN8I_T153','PHY_SUN8I_T153_USB','MMC_SUNXI','SERIAL_8250_DW','SERIAL_8250_CONSOLE','REALTEK_PHY','SMP','ARM_GIC','ARM_ARCH_TIMER','ARM_PSCI_FW','ARM_PATCH_PHYS_VIRT','AUTO_ZRELADDR','BLK_DEV_INITRD','USE_OF']:
        require('CONFIG_'+symbol+'=y\n' in kconfig,'kernel config '+symbol)
    for symbol in ['CAN_M_CAN','CAN_M_CAN_SUN8I_T153']:
        require('CONFIG_'+symbol+'=m\n' in kconfig,'kernel module config '+symbol)
    require('6.12.113' in (linux/'include/config/kernel.release').read_text(),'kernel release')
    # The complete uncompressed kernel must remain below the staged FIT.
    raw=(kdir/'vmlinux-initramfs').read_bytes()
    require(0x48008000+len(raw)+len(zimage)+0x800000<0x50000000,'uncompressed kernel and relocation margin')
    archives=[p for p in (linux/'usr').glob('initramfs_data.cpio*') if p.suffix in ['.cpio','.gz','.xz','.lzma']]
    require(len(archives)==1,'one embedded cpio archive')
    payload=archives[0].read_bytes()
    if archives[0].suffix=='.gz':payload=gzip.decompress(payload)
    elif archives[0].suffix in ['.xz','.lzma']:payload=lzma.decompress(payload)
    require((linux/'usr/initramfs_inc_data').read_bytes() in raw,'checked initramfs is embedded in actual kernel')
    require(b'Linux version 6.12.113' in raw,'actual kernel version banner')
    files=cpio_entries(payload)
    require(b'export INITRAMFS=1' in files.get('init',b''),'RAM-only init entry')
    check_root(files,'initramfs')
    root_tar=list(out.glob('*dshanpi_t153mx-omnigate-rootfs.tar.gz'))
    require(len(root_tar)==1,'one rootfs tar')
    with tarfile.open(root_tar[0],'r:gz') as archive:
        root_files={m.name.removeprefix('./').lstrip('/'):archive.extractfile(m).read() for m in archive if m.isfile()}
    check_root(root_files,'rootfs tar')
    manifests=list(out.glob('*.manifest'));require(len(manifests)==1,'package manifest')
    packages={line.split()[0] for line in manifests[0].read_text().splitlines() if line}
    for pkg in ['luci-ssl','luci-proto-qmi','luci-proto-mbim','kmod-can-t153','canutils-candump','canutils-cansend','canutils-canfdtest','uqmi','umbim']:
        require(pkg in packages,'manifest package '+pkg)
    ext4s=list(out.glob('*dshanpi_t153mx-omnigate*rootfs.ext4.gz'))
    require(len(ext4s)==1,'one T153 ext4 image')
    with gzip.open(ext4s[0],'rb') as fp:
        fp.seek(1024+56);require(fp.read(2)==b'\x53\xef','ext4 superblock')
    require(not list(out.glob('*dshanpi_t153mx-omnigate*sdcard*')),'must not publish unimplemented SD installer')
    checksums=out/'sha256sums'
    require(checksums.exists(),'OpenWrt checksums')
    for line in checksums.read_text().splitlines():
        expected,name=line.split(maxsplit=1);path=out/name.lstrip('*')
        require(path.is_file(),'checksum target '+name)
        with path.open('rb') as fp:actual=hashlib.file_digest(fp,'sha256').hexdigest()
        require(actual==expected,'SHA256 '+name)
    # Export fallback payloads from the checked FIT, never from the smoke build.
    export=port/'artifacts';export.mkdir(exist_ok=True)
    (export/'t153-omnigate-initramfs.zImage').write_bytes(zimage)
    (export/'t153-omnigate.dtb').write_bytes(dt['data'])
    report={'status':'software checks passed; hardware NOT tested','kernel':'6.12.113','fit':str(fit),'fit_bytes':fit.stat().st_size,'zimage_bytes':len(zimage),'uncompressed_kernel_bytes':len(raw),'kernel_load':'0x48008000','fit_staging':'0x50000000','initial_linux_memory_ceiling_MiB':384,'packages':len(packages)}
    (port/'logs/artifact-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
    print('PASS: actual FIT hashes, payload, memory, kernel config, embedded initramfs, rootfs, modules, manifest, SHA256')

if __name__=='__main__':main()
