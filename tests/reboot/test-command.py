#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compile real changed C paths with inert syscall/ubus/process mocks.

No reboot, signal, exec, device or board action is permitted by the harness.
"""
from pathlib import Path
import json
import re
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
BUILD=next((ROOT/'openwrt/build_dir').glob('target-arm_cortex-a7*'))
PROCD=next((BUILD/'procd-default').glob('procd-*'))
BUSYBOX=next((BUILD/'busybox-default').glob('busybox-*'))


def function(source,name):
    match=re.search(r'^(?:static )?(?:int|void) '+name+r'\([^;]*?\)\n\{',source,re.M)
    if not match:
        raise AssertionError('function missing: '+name)
    pos=source.index('{',match.start());depth=1;end=pos+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[match.start():end]+'\n'


LIBBB=r'''
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#define ENABLE_FEATURE_WTMP 0
#define ENABLE_FEATURE_WAIT_FOR_INIT 0
#define ENABLE_HALT 1
#define ENABLE_POWEROFF 1
#define ENABLE_REBOOT 1
#define ENABLE_LINUXRC 0
#define ENABLE_FEATURE_CLEAN_UP 0
#define ENABLE_FEATURE_CALL_TELINIT 0
#define CONFIG_TELINIT_PATH ""
#define MAIN_EXTERNALLY_VISIBLE
#define UNUSED_PARAM __attribute__((unused))
typedef signed char smallint;
extern const char *applet_name;
unsigned getopt32(char **argv,const char *options,...);
char *xasprintf(const char *format,...);
void bb_show_usage(void) __attribute__((noreturn));
void bb_perror_msg_and_die(const char *format,...) __attribute__((noreturn));
void bb_perror_nomsg_and_die(void) __attribute__((noreturn));
pid_t *find_pid_by_name(const char *name);
unsigned mock_sleep(unsigned n);
void mock_sync(void);
int mock_kill(pid_t p,int s);
int mock_reboot(int event);
long mock_syscall(long number,...);
int mock_execl(const char *path,const char *arg,...);
#define sleep mock_sleep
#define sync mock_sync
#define kill mock_kill
#define reboot mock_reboot
#define syscall mock_syscall
#define execl mock_execl
#define execlp mock_execl
'''
BUSYBOX_MOCK=r'''
#include "libbb.h"
#include <sys/syscall.h>
#include <linux/reboot.h>
const char *applet_name;
unsigned getopt32(char **argv,const char *options,...)
{
    int argc=0,c,flags=0;va_list ap;int *delay;
    (void)options;while(argv[argc])argc++;va_start(ap,options);delay=va_arg(ap,int *);va_end(ap);
    optind=1;opterr=0;
    while((c=getopt(argc,argv,"d:nfwi"))!=-1){
        switch(c){case 'd':flags|=1;*delay=atoi(optarg);break;case 'n':flags|=2;break;
        case 'f':flags|=4;break;case 'w':flags|=8;break;case 'i':flags|=16;break;default:bb_show_usage();}
    }
    return flags;
}
char *xasprintf(const char *format,...){char *s;va_list ap;va_start(ap,format);if(vasprintf(&s,format,ap)<0)exit(70);va_end(ap);return s;}
void bb_show_usage(void){exit(64);}
void bb_perror_msg_and_die(const char *format,...){(void)format;exit(65);}
void bb_perror_nomsg_and_die(void){exit(65);}
pid_t *find_pid_by_name(const char *name){static pid_t p[1];(void)name;return p;}
unsigned mock_sleep(unsigned n){(void)n;return 0;}
void mock_sync(void){puts("sync");}
int mock_kill(pid_t p,int s){printf("kill:%d:%d\n",p,s);return 0;}
int mock_reboot(int e){printf("reboot:%x\n",e);return 0;}
long mock_syscall(long number,...){va_list ap;int m1,m2,cmd;const char *arg;va_start(ap,number);
    m1=va_arg(ap,int);m2=va_arg(ap,int);cmd=va_arg(ap,int);arg=va_arg(ap,const char *);va_end(ap);
    if(number!=SYS_reboot || (unsigned)m1!=(unsigned)LINUX_REBOOT_MAGIC1 || (unsigned)m2!=(unsigned)LINUX_REBOOT_MAGIC2 || (unsigned)cmd!=(unsigned)LINUX_REBOOT_CMD_RESTART2)exit(71);
    printf("restart2:%s\n",arg);return 0;}
int mock_execl(const char *path,const char *arg,...){va_list ap;printf("exec:%s|%s",path,arg);va_start(ap,arg);
    while((arg=va_arg(ap,const char *)))printf("|%s",arg);va_end(ap);puts("");exit(0);}
int halt_main(int argc,char **argv);
int main(int argc,char **argv){if(argc<2)return 72;applet_name=argv[1];return halt_main(argc-1,argv+1);}
'''
PROCD_MOCK=r'''
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <linux/reboot.h>
#define DEBUG(...) ((void)0)
#define STATE_SHUTDOWN 5
static int state,reboot_event,shutdown_calls;
static char reboot_command[256];
static void state_enter(void){shutdown_calls++;}
static int mock_reboot(int e){printf("reboot:%x\n",e);return 0;}
static long mock_syscall(long number,...){va_list ap;int m1,m2,cmd;const char *arg;va_start(ap,number);
 m1=va_arg(ap,int);m2=va_arg(ap,int);cmd=va_arg(ap,int);arg=va_arg(ap,const char *);va_end(ap);
 if(number!=SYS_reboot || (unsigned)m1!=(unsigned)LINUX_REBOOT_MAGIC1 || (unsigned)m2!=(unsigned)LINUX_REBOOT_MAGIC2 || (unsigned)cmd!=(unsigned)LINUX_REBOOT_CMD_RESTART2)exit(71);
 printf("restart2:%s\n",arg);return 0;}
#define reboot mock_reboot
#define syscall mock_syscall
#define BLOBMSG_TYPE_STRING 3
#define UBUS_STATUS_INVALID_ARGUMENT 2
struct ubus_context { int dummy; };struct ubus_object { int dummy; };struct ubus_request_data { int dummy; };
struct blobmsg_policy { const char *name;int type; };
struct blob_attr { const char *name;int type;char *string;struct blob_attr *items;int count; };
#define blobmsg_for_each_attr(cur,msg,rem) for((rem)=0;(rem)<(msg)->count && ((cur)=&(msg)->items[rem],1);(rem)++)
static const char *blobmsg_name(struct blob_attr *b){return b->name;}
static int blobmsg_type(struct blob_attr *b){return b->type;}
static char *blobmsg_get_string(struct blob_attr *b){return b->string;}
static void *blob_data(struct blob_attr *b){return b;}
static int blob_len(struct blob_attr *b){return b->count;}
static int blobmsg_parse(const struct blobmsg_policy *p,int count,struct blob_attr **tb,void *raw,int len){
 struct blob_attr *b=raw;int i,j;(void)len;
 for(i=0;i<count;i++)for(j=0;j<b->count;j++)if(!strcmp(p[i].name,b->items[j].name)&&p[i].type==b->items[j].type)tb[i]=&b->items[j];return 0;}
'''
PROCD_MAIN=r'''
int main(int argc,char **argv){
 char mode[300]="efex";int rc;struct blob_attr item={"command",BLOBMSG_TYPE_STRING,mode,NULL,0};
 struct blob_attr msg={NULL,0,NULL,&item,1};
 if(argc!=2)return 72;
 if(!strcmp(argv[1],"normal"))rc=system_reboot(NULL,NULL,NULL,NULL,NULL);
 else {
  if(!strcmp(argv[1],"empty"))mode[0]=0;
  if(!strcmp(argv[1],"long")){memset(mode,'x',256);mode[256]=0;}
  if(!strcmp(argv[1],"wrong-type"))item.type=9;
  rc=system_reboot(NULL,NULL,NULL,NULL,&msg);
 }
 printf("result:%d:shutdown:%d\n",rc,shutdown_calls);
 if(rc){if(shutdown_calls || state)return 73;return 0;}
 if(shutdown_calls!=1 || state!=STATE_SHUTDOWN)return 74;
 strcpy(mode,"changed-after-ubus-reply");
 if(!strcmp(argv[1],"repeat")){procd_shutdown(RB_AUTOBOOT);if(shutdown_calls!=1)return 75;}
 return perform_reboot();
}
'''


class RebootCommandTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory(prefix='t153-reboot-test-');cls.path=Path(cls.temp.name)
        (cls.path/'libbb.h').write_text(LIBBB)
        (cls.path/'halt.c').write_text((BUSYBOX/'init/halt.c').read_text())
        (cls.path/'reboot.h').write_text((BUSYBOX/'init/reboot.h').read_text())
        (cls.path/'busybox-mock.c').write_text(BUSYBOX_MOCK)
        flags=['cc','-std=gnu11','-Wall','-Wextra','-Wno-unused-parameter','-Wno-misleading-indentation','-Werror']
        subprocess.run(flags+['-I',str(cls.path),str(cls.path/'halt.c'),str(cls.path/'busybox-mock.c'),'-o',str(cls.path/'busybox-test')],check=True)
        state=(PROCD/'state.c').read_text();system=(PROCD/'system.c').read_text()
        policy=system[system.index('enum {\n\tREBOOT_COMMAND,'):system.index('static int system_reboot')]
        content=PROCD_MOCK+function(state,'perform_reboot')+function(state,'procd_shutdown_command')+function(state,'procd_shutdown')+policy+function(system,'system_reboot')+PROCD_MAIN
        (cls.path/'procd-test.c').write_text(content)
        subprocess.run(flags+[str(cls.path/'procd-test.c'),'-o',str(cls.path/'procd-test')],check=True)
        cls.state_source=state

    @classmethod
    def tearDownClass(cls):cls.temp.cleanup()

    def run_program(self,name,*args):
        return subprocess.run([str(self.path/name),*args],capture_output=True,text=True)

    def test_regular_busybox_reboot_unchanged(self):
        r=self.run_program('busybox-test','reboot');self.assertEqual(r.returncode,0);self.assertIn('kill:1:15',r.stdout);self.assertNotIn('restart2',r.stdout)

    def test_mode_uses_real_ubus_with_json_and_no_forced_reboot(self):
        r=self.run_program('busybox-test','reboot','efex');self.assertEqual(r.returncode,0)
        self.assertIn('exec:/bin/ubus|ubus|call|system|reboot|{"command":"efex"}',r.stdout)
        self.assertNotIn('kill:',r.stdout);self.assertNotIn('restart2:',r.stdout)

    def test_explicit_force_uses_restart2(self):
        r=self.run_program('busybox-test','reboot','-f','efex');self.assertEqual(r.returncode,0);self.assertIn('restart2:efex',r.stdout)

    def test_invalid_busybox_arguments_do_not_act(self):
        for args in [('reboot','efex','extra'),('poweroff','efex'),('reboot',''),('reboot','x'*256),('reboot','bad"mode'),('reboot','bad mode')]:
            with self.subTest(args=args):
                r=self.run_program('busybox-test',*args);self.assertEqual(r.returncode,64);self.assertEqual(r.stdout,'')

    def test_wtmp_only_never_requests_shutdown(self):
        r=self.run_program('busybox-test','reboot','-w','efex');self.assertEqual(r.returncode,0);self.assertEqual(r.stdout,'')

    def test_procd_retains_mode_after_message_lifetime(self):
        r=self.run_program('procd-test','mode');self.assertEqual(r.returncode,0);self.assertIn('result:0:shutdown:1',r.stdout);self.assertIn('restart2:efex',r.stdout)

    def test_procd_regular_reboot_and_repeated_request(self):
        r=self.run_program('procd-test','normal');self.assertEqual(r.returncode,0);self.assertIn('reboot:1234567',r.stdout)
        r=self.run_program('procd-test','repeat');self.assertEqual(r.returncode,0);self.assertIn('restart2:efex',r.stdout)

    def test_procd_rejects_invalid_command_before_shutdown(self):
        for mode in ['empty','long','wrong-type']:
            with self.subTest(mode=mode):
                r=self.run_program('procd-test',mode);self.assertEqual(r.returncode,0);self.assertEqual(r.stdout,'result:2:shutdown:0\n')

    def test_existing_shutdown_sequence_still_precedes_final_syscall(self):
        body=function(self.state_source,'state_enter')
        for operation in ['procd_inittab_run("shutdown")','kill(-1, SIGTERM)','kill(-1, SIGKILL)','perform_halt()']:
            self.assertIn(operation,body)
        self.assertNotIn('syscall(',body)
        self.assertNotIn('syscall(',function((PROCD/'system.c').read_text(),'system_reboot'))


if __name__=='__main__':
    unittest.main()
