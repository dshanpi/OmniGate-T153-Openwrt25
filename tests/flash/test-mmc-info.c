// SPDX-License-Identifier: GPL-2.0-only
/* Exercise the production EXT_CSD reader without any device access. */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/mmc/ioctl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

int mmc_info_main(int, char **);
static int mode, calls, opens, closes;
int __wrap_open(const char *path, int flags, ...)
{
    assert(!strcmp(path,"/dev/mmcblk0"));
    assert((flags & O_ACCMODE)==O_RDONLY && (flags & O_NOFOLLOW));
    opens++;
    if (mode==1) {errno=EACCES;return -1;}
    return 42;
}
int __wrap_fstat(int fd, struct stat *st)
{
    assert(fd==42);memset(st,0,sizeof(*st));
    st->st_mode=mode==2?S_IFREG:S_IFBLK;return 0;
}
int __wrap_close(int fd) {assert(fd==42);closes++;return 0;}
int __wrap_ioctl(int fd, unsigned long request, ...)
{
    va_list ap;va_start(ap,request);struct mmc_ioc_cmd *cmd=va_arg(ap,struct mmc_ioc_cmd *);va_end(ap);
    assert(fd==42 && request==MMC_IOC_CMD);
    assert(cmd->opcode==8 && cmd->arg==0 && cmd->write_flag==0);
    assert(cmd->flags==0x35 && cmd->blksz==512 && cmd->blocks==1);
    calls++;
    if (mode==3) {errno=EIO;return -1;}
    uint8_t *buf=(void *)(uintptr_t)cmd->data_ptr;
    buf[192]=mode==4?0:8;buf[179]=0x38;buf[177]=2;
    cmd->response[0]=mode==5?(1U<<22):0x900;
    return 0;
}
int main(void)
{
    char *good[]={"test","--device","/dev/mmcblk0",0};
    for(mode=0;mode<=5;mode++) {
        calls=opens=closes=0;
        int rc=mmc_info_main(3,good);
        assert((mode==0)==(rc==0));
        assert(opens==1 && closes==(mode!=1));
        assert(calls==(mode!=1 && mode!=2));
    }
    const char *bad[]={"/dev/sda","/dev/mmcblk0p1","/dev/mmcblk0boot0","/dev/mmcblk","/dev/mmcblk0/../sda"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++) {
        good[2]=(char *)bad[i];opens=0;
        assert(mmc_info_main(3,good)!=0 && opens==0);
    }
    puts("PASS: read-only CMD8, device/error gates; no real device opened");return 0;
}
