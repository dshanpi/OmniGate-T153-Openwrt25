/* SPDX-License-Identifier: GPL-2.0-only
 * Read-only PHY guards plus AF_PACKET traffic for an already-enabled loopback.
 * Does NOT change PHY/MAC registers, link settings, taps, IP, or interface flags.
 */
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if_packet.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define TYPE 0x88b5
#define MAX_FRAME 1514
#define RTL8211F_ID 0x001cc916U
static volatile sig_atomic_t interrupted;
static void on_signal(int sig) { (void)sig; interrupted = 1; }
static uint64_t millis(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts)) abort();
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
static uint32_t crc32_buf(const uint8_t *p, size_t n)
{
    uint32_t c = ~0U;
    while (n--) {
        c ^= *p++;
        for (int b = 0; b < 8; b++)
            c = (c >> 1) ^ (0xedb88320U & (0U - (c & 1U)));
    }
    return ~c;
}
static void put32(uint8_t *p, uint32_t v)
{
    v = htonl(v); memcpy(p, &v, sizeof(v));
}
static void make_frame(uint8_t *p, size_t n, const uint8_t mac[6],
                       const uint8_t nonce[8], uint32_t seq)
{
    memcpy(p, mac, 6); memcpy(p + 6, mac, 6);
    p[12] = TYPE >> 8; p[13] = TYPE & 255;
    memcpy(p + 14, "T153PHY1", 8);
    memcpy(p + 22, nonce, 8); put32(p + 30, seq); put32(p + 34, n);
    for (size_t i = 38; i < n - 4; i++)
        p[i] = (uint8_t)(i * 73U + seq * 19U + nonce[i & 7]);
    put32(p + n - 4, crc32_buf(p, n - 4));
}
static int valid_frame(const uint8_t *p, size_t n, const uint8_t *expected,
                       size_t expected_n, const struct sockaddr_ll *from,
                       int ifindex)
{
    uint32_t stored;
    if (from->sll_ifindex != ifindex || from->sll_pkttype != PACKET_HOST ||
        from->sll_protocol != htons(TYPE) || n != expected_n || n < 64)
        return 0;
    memcpy(&stored, p + n - 4, sizeof(stored));
    if (ntohl(stored) != crc32_buf(p, n - 4)) return 0;
    /* Exact compare includes both MACs, magic, nonce, sequence, length, payload. */
    return memcmp(p, expected, n) == 0;
}
static int read_num(const char *name, const char *entry)
{
    char path[128]; int n = -1;
    snprintf(path, sizeof(path), "/sys/class/net/%s/%s", name, entry);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    if (fscanf(f, "%d", &n) != 1) n = -1;
    fclose(f); return n;
}
static int mdio_read(int fd, const char *name, int phy, int reg)
{
    struct ifreq ifr = {0}; struct mii_ioctl_data m = {0};
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", name);
    m.phy_id = phy; m.reg_num = reg;
    memcpy(&ifr.ifr_data, &m, sizeof(m));
    if (ioctl(fd, SIOCGMIIREG, &ifr)) return -1;
    memcpy(&m, &ifr.ifr_data, sizeof(m));
    return m.val_out;
}
/* Actual stmmac/dwmac4 dump starts at GMAC_CONFIG, native u32.  Query
 * only at the transaction boundaries, not repeatedly while moving packets.
 * The ethtool driver collects a full register dump internally even for len=4.
 */
static int mac_guard(int fd, const char *name)
{
    struct ifreq ifr = {0};
    struct ethtool_regs *regs = calloc(1, sizeof(*regs) + sizeof(uint32_t));
    uint32_t config;
    if (!regs) return 0;
    regs->cmd = ETHTOOL_GREGS; regs->len = sizeof(config);
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", name);
    ifr.ifr_data = (char *)regs;
    if (ioctl(fd, SIOCETHTOOL, &ifr) || regs->len != sizeof(config)) {
        free(regs); fputs("MAC register guard unavailable\n", stderr); return 0;
    }
    memcpy(&config, regs->data, sizeof(config)); free(regs);
    /* PS15=0,FES14=0 => 1G; DM13=1; LM12=0; TX1/RX0 enabled. */
    int ok = (config & 0xf003U) == 0x2003U;
    printf("mac_guard config=%08x 1g_full_tx_rx_external_path=%d\n", config, ok);
    return ok;
}
static int guard(int fd, const char *name, int phy, int verbose)
{
    int page = mdio_read(fd, name, phy, 31);
    int bmcr = mdio_read(fd, name, phy, MII_BMCR);
    int carrier = read_num(name, "carrier"), speed = read_num(name, "speed");
    char path[128], duplex[16] = "unknown";
    snprintf(path, sizeof(path), "/sys/class/net/%s/duplex", name);
    FILE *f = fopen(path, "r");
    if (f) { if (fscanf(f, "%15s", duplex) != 1) strcpy(duplex, "unknown"); fclose(f); }
    int ok = (page == 0 || page == 0xa42) && bmcr >= 0 && (bmcr & (BMCR_LOOPBACK | BMCR_SPEED1000 | BMCR_FULLDPLX)) == 0x4140 &&
        !(bmcr & (BMCR_RESET | BMCR_ANENABLE | BMCR_PDOWN | BMCR_ISOLATE | BMCR_SPEED100)) &&
        carrier == 1 && speed == 1000 && (!strcmp(duplex, "full") || !strcmp(duplex, "Full"));
    if (verbose || !ok)
        printf("guard page=%04x bmcr=%04x carrier=%d speed=%d duplex=%s ok=%d\n", page, bmcr, carrier, speed, duplex, ok);
    return ok;
}
static int selftest(void)
{
    uint8_t p[MAX_FRAME], e[MAX_FRAME], mac[6] = {2,1,2,3,4,5}, nonce[8] = {1,2,3,4,5,6,7,8};
    struct sockaddr_ll from = {.sll_ifindex=7, .sll_pkttype=PACKET_HOST, .sll_protocol=htons(TYPE)};
    if (crc32_buf((const uint8_t *)"123456789", 9) != 0xcbf43926U) return 1;
    make_frame(e, sizeof(e), mac, nonce, 42); memcpy(p, e, sizeof(p));
    if (!valid_frame(p, sizeof(p), e, sizeof(e), &from, 7)) return 1;
    from.sll_pkttype = PACKET_OUTGOING;
    if (valid_frame(p, sizeof(p), e, sizeof(e), &from, 7)) return 1;
    from.sll_pkttype = PACKET_HOST; p[50] ^= 1;
    if (valid_frame(p, sizeof(p), e, sizeof(e), &from, 7)) return 1;
    put32(p + sizeof(p) - 4, crc32_buf(p, sizeof(p) - 4));
    if (valid_frame(p, sizeof(p), e, sizeof(e), &from, 7)) return 1;
    make_frame(p, sizeof(p), mac, nonce, 43);
    if (valid_frame(p, sizeof(p), e, sizeof(e), &from, 7)) return 1;
    if (valid_frame(e, sizeof(e)-1, e, sizeof(e), &from, 7)) return 1;
    puts("selftest PASS: CRC32, incoming match, outgoing rejection, corruption, sequence, length");
    return 0;
}
int main(int argc, char **argv)
{
    int ctl=-1, rx=-1, tx=-1, result=2, phy, idx, one=1;
    unsigned sent=0, received=0, ignored=0, outgoing=0, timedout=0;
    unsigned count=16, timeout_ms=100;
    uint8_t expected[MAX_FRAME], buffer[MAX_FRAME+128], mac[6], nonce[8];
    struct ifreq ifr={0}; struct sockaddr_ll bindaddr={0}, dest={0};
    if (argc == 2 && !strcmp(argv[1], "--selftest")) return selftest();
    if (argc < 2 || argc > 4) { fprintf(stderr,"usage: %s IFACE [frames=16] [timeout_ms=100]\n",argv[0]); return 2; }
    if (strchr(argv[1], '/') || strlen(argv[1]) >= IFNAMSIZ) return 2;
    if (argc > 2) { char *end; unsigned long v=strtoul(argv[2],&end,10); if (*end || !v || v>128) return 2; count=v; }
    if (argc > 3) { char *end; unsigned long v=strtoul(argv[3],&end,10); if (*end || v<10 || v>500) return 2; timeout_ms=v; }
    setvbuf(stdout, NULL, _IOLBF, 0);
    signal(SIGINT,on_signal); signal(SIGTERM,on_signal); signal(SIGHUP,on_signal);
    ctl=socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0); if (ctl<0) goto error;
    snprintf(ifr.ifr_name,sizeof(ifr.ifr_name),"%s",argv[1]);
    if (ioctl(ctl,SIOCGIFINDEX,&ifr)) goto error; idx=ifr.ifr_ifindex;
    if (ioctl(ctl,SIOCGIFFLAGS,&ifr)) goto error;
    if (!(ifr.ifr_flags & IFF_UP)) { fputs("interface is down\n",stderr); goto done; }
    if (ioctl(ctl,SIOCGIFHWADDR,&ifr)) goto error; memcpy(mac,ifr.ifr_hwaddr.sa_data,6);
    if (ioctl(ctl,SIOCGMIIPHY,&ifr)) goto error;
    struct mii_ioctl_data mii; memcpy(&mii, &ifr.ifr_data, sizeof(mii)); phy=mii.phy_id;
    int id1=mdio_read(ctl,argv[1],phy,MII_PHYSID1), id2=mdio_read(ctl,argv[1],phy,MII_PHYSID2);
    if (id1<0 || id2<0 || (((uint32_t)id1<<16)|(uint32_t)id2)!=RTL8211F_ID) { fputs("unexpected PHY; no packets sent\n",stderr); goto done; }
    if (!guard(ctl,argv[1],phy,1) || !mac_guard(ctl,argv[1])) goto done;
    if (getrandom(nonce,sizeof(nonce),0)!=(ssize_t)sizeof(nonce)) goto error;
    rx=socket(AF_PACKET,SOCK_RAW|SOCK_NONBLOCK|SOCK_CLOEXEC,htons(TYPE)); if(rx<0) goto error;
    tx=socket(AF_PACKET,SOCK_RAW|SOCK_NONBLOCK|SOCK_CLOEXEC,0); if(tx<0) goto error;
    if (setsockopt(rx,SOL_PACKET,PACKET_IGNORE_OUTGOING,&one,sizeof(one))) goto error;
    bindaddr.sll_family=AF_PACKET; bindaddr.sll_protocol=htons(TYPE); bindaddr.sll_ifindex=idx;
    if(bind(rx,(struct sockaddr*)&bindaddr,sizeof(bindaddr))) goto error;
    dest=bindaddr; dest.sll_halen=6; memcpy(dest.sll_addr,mac,6);
    printf("test iface=%s phy=%d ethertype=%04x mac=%02x:%02x:%02x:%02x:%02x:%02x frames=%u timeout_ms=%u nonce=",argv[1],phy,TYPE,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],count,timeout_ms);
    for (unsigned i=0;i<sizeof(nonce);i++) printf("%02x",nonce[i]); puts("");
    result=1;
    for (unsigned seq=0;seq<count && !interrupted;seq++) {
        size_t n=(seq&1)?1514:64; int matched=0;
        if(!guard(ctl,argv[1],phy,0)) { result=2; break; }
        make_frame(expected,n,mac,nonce,seq);
        ssize_t wrote=sendto(tx,expected,n,0,(struct sockaddr*)&dest,sizeof(dest));
        if(wrote!=(ssize_t)n) goto error;
        sent++;
        uint64_t deadline=millis()+timeout_ms;
        while(!interrupted && millis()<deadline) {
            struct pollfd pfd={.fd=rx,.events=POLLIN};
            uint64_t now=millis(); if(now>=deadline) break;
            int remain=(int)(deadline-now);
            int ready=poll(&pfd,1,remain); if(ready<0) { if(errno==EINTR) continue; goto error; }
            if(!ready) break;
            struct sockaddr_ll from={0}; socklen_t fromlen=sizeof(from);
            ssize_t got=recvfrom(rx,buffer,sizeof(buffer),MSG_TRUNC,(struct sockaddr*)&from,&fromlen);
            if(got<0) { if(errno==EAGAIN || errno==EINTR) continue; goto error; }
            if(from.sll_pkttype==PACKET_OUTGOING) outgoing++;
            if(got<=(ssize_t)sizeof(buffer) && valid_frame(buffer,got,expected,n,&from,idx)) { received++; matched=1; break; }
            ignored++;
        }
        if(!matched) timedout++;
        printf("frame seq=%u bytes=%zu result=%s\n",seq,n,matched?"HOST_CRC_MATCH":"TIMEOUT");
    }
    if(!guard(ctl,argv[1],phy,1) || !mac_guard(ctl,argv[1])) result=2;
    else if(!interrupted && sent==count && received==count) result=0;
    if(interrupted) result=130;
    printf("summary sent=%u received_host_crc_match=%u timedout=%u ignored=%u outgoing_seen=%u result=%d\n",sent,received,timedout,ignored,outgoing,result);
    goto done;
error:
    perror("diagnostic"); result=2;
done:
    if(tx>=0)close(tx); if(rx>=0)close(rx); if(ctl>=0)close(ctl);
    puts("PHY/MAC settings were not changed; caller must restore its loopback setup.");
    return result;
}
