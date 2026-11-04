#!/usr/bin/env python3
"""Host-compile the actual callback bodies; no hardware or MMIO is accessed."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
KERNEL = next((ROOT / 'openwrt/build_dir').glob('target-*/linux-sunxi_cortexa7/linux-6.12.113'))

def function(source, signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

core = (KERNEL / 'drivers/power/reset/reboot-mode.c').read_text()
rtc = (ROOT / 'port/target/linux/sunxi/files/drivers/power/reset/sun8i-t153-reboot-mode.c').read_text()
wdt = (KERNEL / 'drivers/watchdog/sunxi_wdt.c').read_text()
helpers = '\n'.join([
    function(core, 'static bool get_reboot_mode_magic'),
    function(core, 'static int reboot_mode_notify'),
    function(rtc, 'static int t153_reboot_mode_write'),
    function(wdt, 'static void sunxi_wdt_set_output'),
    function(wdt, 'static int sunxi_wdt_restart'),
])
prelude = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <setjmp.h>
typedef uint32_t u32;
#define __iomem
#define T153_RTC_FEL_MAGIC 0x5aa5a55aU
#define NOTIFY_DONE 0
#define container_of(p,t,m) ((t *)((char *)(p)-offsetof(t,m)))
struct list_head { struct list_head *next; };
struct mode_info { const char *mode; u32 magic; struct list_head list; };
#define list_for_each_entry(pos, head, member) \
 for (struct list_head *it=(head)->next; it!=(head) && ((pos)=container_of(it,struct mode_info,member),1); it=it->next)
struct notifier_block { int unused; };
struct reboot_mode_driver {
 struct list_head head;
 struct notifier_block reboot_notifier;
 int (*write)(struct reboot_mode_driver *, unsigned int);
 void *dev;
};
struct t153_reboot_mode { struct reboot_mode_driver reboot; void *request; };
struct sunxi_wdt_reg { unsigned char wdt_ctrl, wdt_cfg, wdt_mode, wdt_out_cfg, wdt_timeout_shift, wdt_reset_mask, wdt_reset_val; u32 wdt_out_mask, wdt_out_val, wdt_key_val; bool wdt_restart_single_shot; };
struct sunxi_wdt_dev { void *wdt_base; const struct sunxi_wdt_reg *wdt_regs; };
struct watchdog_device { struct sunxi_wdt_dev *priv; };
#define watchdog_get_drvdata(wdt) ((wdt)->priv)
#define WDT_TIMEOUT_MASK 0xf
#define WDT_MODE_EN 1
#define WDT_CTRL_RELOAD 0x14af
static jmp_buf watchdog_escape;
static unsigned watchdog_ticks;
static void mdelay(unsigned n) { (void)n; if (++watchdog_ticks == 4) longjmp(watchdog_escape,1); }
static unsigned writes, reads, delays, errors, block_writes;
static u32 cell, observed;
static void writel(u32 value, void *p) {
 writes++;
 if (writes > block_writes) *(u32 *)p=value;
}
static u32 readl(void *p) { reads++; return *(u32 *)p; }
static void udelay(unsigned n) { delays+=n; }
#define dev_err(...) (errors++)
static int record(struct reboot_mode_driver *r,unsigned int magic) { (void)r; observed=magic; writes++;return 0; }
static void reset_counters(void) { writes=reads=delays=errors=block_writes=0; }
'''
main = r'''
int main(void) {
 struct mode_info modes[] = {{.mode="normal",.magic=0},{.mode="efex",.magic=T153_RTC_FEL_MAGIC}};
 struct reboot_mode_driver reboot={.write=record};
 reboot.head.next=&modes[0].list;modes[0].list.next=&modes[1].list;modes[1].list.next=&reboot.head;
 observed=0xdeadbeef; reset_counters();
 reboot_mode_notify(&reboot.reboot_notifier,0,NULL);
 assert(writes==1 && observed==0); /* explicit zero must clear, old core fails */
 reboot_mode_notify(&reboot.reboot_notifier,0,"efex");assert(writes==2 && observed==T153_RTC_FEL_MAGIC);
 reboot_mode_notify(&reboot.reboot_notifier,0,"unknown");assert(writes==2); /* unmatched must not write zero */
 reboot_mode_notify(&reboot.reboot_notifier,0,"normal");assert(writes==3 && observed==0);
 struct t153_reboot_mode rtc={.request=&cell};
 reset_counters();cell=T153_RTC_FEL_MAGIC;
 assert(t153_reboot_mode_write(&rtc.reboot,0)==0 && cell==0 && writes==1);
 reset_counters();block_writes=3;
 assert(t153_reboot_mode_write(&rtc.reboot,T153_RTC_FEL_MAGIC)==0 && cell==T153_RTC_FEL_MAGIC && writes==4 && delays==3);
 reset_counters();
 assert(t153_reboot_mode_write(&rtc.reboot,0x5a)==-EINVAL && writes==0);
 reset_counters();block_writes=1000;cell=0;
 assert(t153_reboot_mode_write(&rtc.reboot,T153_RTC_FEL_MAGIC)==-ETIMEDOUT);
 assert(writes==1000 && reads==1000 && delays==1000 && errors==1);
 u32 regs[8]={0};
 struct sunxi_wdt_reg variant={.wdt_out_cfg=0x18,.wdt_out_mask=0xfff,.wdt_out_val=0x3f,.wdt_key_val=0x16aa0000};
 struct sunxi_wdt_dev wdt={.wdt_base=regs,.wdt_regs=&variant};
 reset_counters();regs[6]=0x0000f555;sunxi_wdt_set_output(&wdt);
 assert(regs[6]==0x16aaf03f && writes==1 && reads==1);
 reset_counters();variant.wdt_out_mask=0;sunxi_wdt_set_output(&wdt);assert(writes==0 && reads==0);
 variant=(struct sunxi_wdt_reg){.wdt_ctrl=0x0c,.wdt_cfg=0x10,.wdt_mode=0x14,.wdt_out_cfg=0x18,.wdt_timeout_shift=4,.wdt_reset_mask=3,.wdt_reset_val=1,.wdt_out_mask=0xfff,.wdt_out_val=0x3f,.wdt_key_val=0x16aa0000,.wdt_restart_single_shot=true};
 struct watchdog_device device={.priv=&wdt};
 reset_counters();memset(regs,0,sizeof(regs));watchdog_ticks=0;
 if (!setjmp(watchdog_escape)) sunxi_wdt_restart(&device,0,NULL);
 assert(writes==4 && reads==3); /* OUT, CFG, MODE, CTRL once; no loop writes */
 assert(regs[3]==0x14af && regs[4]==0x16aa0001 && regs[5]==0x16aa0001 && regs[6]==0x16aa003f);
 reset_counters();variant.wdt_out_mask=0;variant.wdt_restart_single_shot=false;watchdog_ticks=0;
 if (!setjmp(watchdog_escape)) sunxi_wdt_restart(&device,0,NULL);
 assert(writes==6 && reads==5); /* legacy behavior still rewrites MODE */
 puts("PASS: mode selection; RTC retry/timeout; T153 single-shot restart; legacy restart unchanged");
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='t153-reboot-test-') as tmp:
    source=Path(tmp)/'test.c'; binary=Path(tmp)/'test'
    source.write_text(prelude+helpers+main)
    subprocess.run(['cc','-std=gnu11','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-fsanitize=undefined','-o',str(binary),str(source)],check=True)
    subprocess.run([str(binary)],check=True)
