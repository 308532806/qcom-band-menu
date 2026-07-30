/*
 * qcom-band-menu-v3.2.c
 * Interactive Qualcomm QMI NAS forcing UI for rooted ARM64 Android.
 *
 * Based on qcom-band-menu-v2-mode-debug.c, extended with independent NR-SA
 * and NR-NSA band locking (see notes below).
 *
 * Launch flags:
 *   -verbose      start with verbose TX/RX/TLV dumps on (default: off).
 *                 Can also be toggled at runtime, see the "verbose" command.
 *
 * Usage:
 *   qcom-band-menu-v3.2 [-verbose]
 *
 * Commands:
 *   sim 1 | sim 2
 *   rat auto | rat gsm,wcdma,lte,nr
 *   gsm 850,900,1800,1900
 *   wcdma 1,2,4,5,6,8,19
 *   lte 1,2,3,66,71
 *   nr 1,3,28,41,78          (sets the combined NR band mask -- affects
 *                              whichever of SA/NSA the modem is currently
 *                              using, per TLV 0x2B)
 *   sa 1,3,28,41,78          (sets ONLY the NR-SA band mask, TLV 0x2F)
 *   nsa 1,3,28,41,78         (sets ONLY the NR-NSA band mask, TLV 0x30)
 *   mode sa | mode nsa | mode both
 *   verbose on | verbose off   (toggle at runtime; off by default unless
 *                                launched with -verbose. Dumps raw TX/RX
 *                                bytes and decoded TLVs for every command.)
 *   hardware
 *   refresh
 *   exit
 *
 * Independent SA/NSA band locking:
 *   TLV 0x2C/0x2D are the ids the device uses to REPORT the current NR-SA /
 *   NR-NSA band state on a GET reply (query() has parsed these correctly
 *   all along -- see the "NR-SA"/"NR-NSA" lines in the status display).
 *   They are NOT valid write ids: an earlier revision of this tool reused
 *   them on a SET request and the modem rejected it (result=0x0001,
 *   error=0x0001 -- effectively "malformed/unrecognized TLV").
 *
 *   This device already has a proven GET-id/SET-id split for one field:
 *   TLV_EXT_LTE_GET (0x23) vs TLV_EXT_LTE_SET (0x24) for the extended LTE
 *   mask. The same split applies to NR-SA/NR-NSA. Cross-referencing
 *   nr-nsa-check.c/ratctl_dynamic.c (which independently reverse-engineered
 *   a working NR-SA/NR-NSA *SET* against the same modem from live strace
 *   captures) gives the correct write ids: 0x2F for NR-SA and 0x30 for
 *   NR-NSA. This file now uses TLV_NR_SA_SET (0x2F) / TLV_NR_NSA_SET (0x30)
 *   for the "sa"/"nsa" commands, while query() keeps reading GET replies
 *   from TLV_NR_SA_GET (0x2C) / TLV_NR_NSA_GET (0x2D) as before.
 *
 *   cmd_nr() still sends only TLV_DURATION (0x17) plus the one 64-byte NR
 *   mask being set -- the same proven incremental pattern every other
 *   setter in this file uses (cmd_lte/cmd_gsm/cmd_wcdma/cmd_mode never send
 *   a full-state snapshot either). If the modem still rejects this with the
 *   corrected ids, the next thing to try -- based on nr-nsa-check.c's notes
 *   that its device wants the complete LTE+SA+NSA state in one call -- is
 *   sending the companion TLVs (0x12 legacy, 0x16 net-sel, 0x1f, 0x25,
 *   0x24 LTE, the other NR domain's current mask, 0x36 capability, 0x37)
 *   alongside 0x2F/0x30 in a single request. Re-run with "verbose on" (the
 *   default) and share the TX/RX dump if that's needed.
 *
 * NR MODE shown as "unknown" until set once -- fixed:
 *   query() parses every other field out of GET replies (RAT, legacy,
 *   LTE, NR-SA, NR-NSA) but never looked for a live "current NR mode"
 *   value there; nr_mode[]/nr_mode_known[] were only ever populated
 *   locally inside cmd_mode() right after a successful SET, so the display
 *   only ever reflected "what I last told it to do", not "what it actually
 *   reports right now" -- and was wrong on first launch or after any
 *   command other than "mode".
 *
 *   First attempt: assumed GET reused TLV_NR_MODE (0x2E), the same id the
 *   SET side uses. Disproved by a capture: on GET, id 0x2E is a 64-byte
 *   field that never changed value across any capture, including
 *   immediately after a successful "mode both" SET -- a static capability
 *   blob, not live mode (same pattern as nr-nsa-check.c's always-identical
 *   TLV 0x36).
 *
 *   Confirmed by a second capture cycling through "mode sa" / "mode nsa" /
 *   "mode both", each followed immediately by a GET: id 0x2B (the same id
 *   the "nr" command uses on SET for its 64-byte combined band mask) is
 *   reused on GET for a 4-byte current-mode report, with value byte 0
 *   equal to 0x02/0x01/0x00 respectively for sa/nsa/both -- exactly
 *   matching cmd_mode()'s own 0=both/1=nsa/2=sa encoding. query() now
 *   parses this (guarded to length==4, so it can never collide with the
 *   64-byte combined-mask meaning), so NR MODE is correct from the very
 *   first query, before any "mode" command has ever been issued.
 *
 * Build:
 * clang --target=aarch64-linux-gnu -fuse-ld=lld -O2 -nostdlib -static \
 *   -fno-stack-protector -fno-builtin -Wl,-e,_start \
 *   -Wl,--build-id=none -o qcom-band-menu-v3.2 qcom-band-menu-v3.2.c
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;
typedef long s64;

enum { SYS_close=57,SYS_read=63,SYS_write=64,SYS_exit=93,SYS_nanosleep=101,SYS_clock_gettime=113,
       SYS_socket=198,SYS_connect=203,SYS_sendto=206,SYS_recvfrom=207,SYS_setsockopt=208 };
enum { AF_QIPCRTR=42,SOCK_DGRAM=2,SOL_SOCKET=1,SO_RCVTIMEO=20,CLOCK_MONOTONIC=1 };
#define QRTR_CTRL_NODE 1u
#define QRTR_PORT_CTRL 0xFFFFFFFEu
#define NAS_SERVICE 3u
#define DMS_SERVICE 2u
#define DMS_MSG_GET_BANDS 0x0045u
#define NAS_PACKED_INSTANCE 1u
#define QRTR_TYPE_NEW_LOOKUP 0x0Au
#define QRTR_TYPE_NEW_SERVER 0x04u
#define MSG_SET 0x0033u
#define MSG_GET 0x0034u
#define MSG_BIND 0x0045u
#define TLV_RESULT 0x02u
#define TLV_MODE 0x11u
#define TLV_LEGACY 0x12u
#define TLV_LTE 0x15u
#define TLV_DURATION 0x17u
#define TLV_EXT_LTE_GET 0x23u
#define TLV_EXT_LTE_SET 0x24u
#define TLV_NR_COMBINED 0x2Bu
#define TLV_NR_SA_GET 0x2Cu   /* read-back id, confirmed working: query() parses this from GET replies */
#define TLV_NR_NSA_GET 0x2Du  /* read-back id, confirmed working: query() parses this from GET replies */
#define TLV_NR_SA_SET 0x2Fu   /* write id for an independent NR-SA lock -- see notes below */
#define TLV_NR_NSA_SET 0x30u  /* write id for an independent NR-NSA lock -- see notes below */
#define TLV_NR_MODE 0x2Eu

struct sockaddr_qrtr{u16 family,pad;u32 node,port;};
struct qrtr_ctrl_pkt{u32 command,service,instance,node,port;};
struct timeval64{s64 sec,usec;};
struct timespec64{s64 sec,nsec;};
struct state{s64 fd;u32 node,port;int sim;u16 rat;u8 legacy[8],lte[8],extlte[32],sa[64],nsa[64];int valid;u8 hw_legacy[8],hw_lte[32],hw_nr[64];int hw_valid;int nr_mode[2],nr_mode_known[2];char status[160];int verbose;int show_hardware;};

static inline s64 sc1(s64 n,s64 a){register s64 x8 __asm__("x8")=n;register s64 x0 __asm__("x0")=a;__asm__ volatile("svc 0":"+r"(x0):"r"(x8):"memory");return x0;}
static inline s64 sc2(s64 n,s64 a,s64 b){register s64 x8 __asm__("x8")=n;register s64 x0 __asm__("x0")=a;register s64 x1 __asm__("x1")=b;__asm__ volatile("svc 0":"+r"(x0):"r"(x1),"r"(x8):"memory");return x0;}
static inline s64 sc3(s64 n,s64 a,s64 b,s64 c){register s64 x8 __asm__("x8")=n;register s64 x0 __asm__("x0")=a;register s64 x1 __asm__("x1")=b;register s64 x2 __asm__("x2")=c;__asm__ volatile("svc 0":"+r"(x0):"r"(x1),"r"(x2),"r"(x8):"memory");return x0;}
static inline s64 sc5(s64 n,s64 a,s64 b,s64 c,s64 d,s64 e){register s64 x8 __asm__("x8")=n;register s64 x0 __asm__("x0")=a;register s64 x1 __asm__("x1")=b;register s64 x2 __asm__("x2")=c;register s64 x3 __asm__("x3")=d;register s64 x4 __asm__("x4")=e;__asm__ volatile("svc 0":"+r"(x0):"r"(x1),"r"(x2),"r"(x3),"r"(x4),"r"(x8):"memory");return x0;}
static inline s64 sc6(s64 n,s64 a,s64 b,s64 c,s64 d,s64 e,s64 f){register s64 x8 __asm__("x8")=n;register s64 x0 __asm__("x0")=a;register s64 x1 __asm__("x1")=b;register s64 x2 __asm__("x2")=c;register s64 x3 __asm__("x3")=d;register s64 x4 __asm__("x4")=e;register s64 x5 __asm__("x5")=f;__asm__ volatile("svc 0":"+r"(x0):"r"(x1),"r"(x2),"r"(x3),"r"(x4),"r"(x5),"r"(x8):"memory");return x0;}

static u64 slen(const char*s){u64 n=0;while(s[n])n++;return n;}
static void out(const char*s){sc3(SYS_write,1,(s64)s,(s64)slen(s));}
static void zero(void*p,u64 n){u8*b=p;while(n--)*b++=0;}
static void copy(void*d,const void*s,u64 n){u8*dd=d;const u8*ss=s;while(n--)*dd++=*ss++;}
static int eq(const char*a,const char*b){while(*a&&*b&&*a==*b){a++;b++;}return *a==*b;}
static int starts(const char*s,const char*p){while(*p)if(*s++!=*p++)return 0;return 1;}
static u16 le16(const u8*p){return (u16)p[0]|((u16)p[1]<<8);}
static u64 le64(const u8*p){return (u64)p[0]|((u64)p[1]<<8)|((u64)p[2]<<16)|((u64)p[3]<<24)|((u64)p[4]<<32)|((u64)p[5]<<40)|((u64)p[6]<<48)|((u64)p[7]<<56);}
static void put16(u8*p,u16 v){p[0]=(u8)v;p[1]=(u8)(v>>8);}
static void outnum(u32 v){char b[10];int i=0,j;if(!v){out("0");return;}while(v){b[i++]=(char)('0'+v%10);v/=10;}for(j=i-1;j>=0;j--)sc3(SYS_write,1,(s64)&b[j],1);}
static void setstatus(struct state*s,const char*x){u64 i=0;while(x[i]&&i+1<sizeof(s->status)){s->status[i]=x[i];i++;}s->status[i]=0;}

/* ── Verbose/diagnostic helpers ──────────────────────────────────────────
 * Everything below prints raw wire bytes and decoded TLVs for SET (0x0033)
 * and GET (0x0034) transactions. Off by default -- start with "-verbose"
 * on the command line, or use the "verbose on"/"verbose off" commands at
 * runtime, to see the full TX/RX dump when a command is rejected or a GET
 * reply is missing a TLV that was expected. */
static void out_hex8(u8 v){static const char h[]="0123456789ABCDEF";char b[2];b[0]=h[(v>>4)&0xf];b[1]=h[v&0xf];sc3(SYS_write,1,(s64)b,2);}
static void out_hex16(u16 v){out_hex8((u8)(v>>8));out_hex8((u8)v);}
static void dump_bytes(const char*label,const u8*p,u32 n){u32 i;out(label);for(i=0;i<n;i++){out_hex8(p[i]);out(i+1==n?"\n":" ");}if(!n)out("\n");}

/* Best-effort human label for the TLV ids this tool itself sends/reads.
 * Anything else prints as "unknown" -- that's fine, the raw id is always
 * printed alongside it. */
static const char*tlv_name(u8 id){
 switch(id){
  case TLV_RESULT:return "RESULT";
  case TLV_MODE:return "RAT_MODE";
  case TLV_LEGACY:return "LEGACY(GSM/WCDMA)";
  case TLV_LTE:return "LTE(legacy,8B)";
  case TLV_DURATION:return "DURATION/APPLY";
  case TLV_EXT_LTE_GET:return "LTE_EXT(get,32B)";
  case TLV_EXT_LTE_SET:return "LTE_EXT(set,32B)";
  case TLV_NR_COMBINED:return "NR_COMBINED(0x2B)";
  case TLV_NR_SA_GET:return "NR_SA_GET(0x2C)";
  case TLV_NR_NSA_GET:return "NR_NSA_GET(0x2D)";
  case TLV_NR_SA_SET:return "NR_SA_SET(0x2F)";
  case TLV_NR_NSA_SET:return "NR_NSA_SET(0x30)";
  case TLV_NR_MODE:return "NR_MODE(0x2E)";
  default:return "unknown";
 }
}

/* Finds the TLV_RESULT (0x02) TLV in a response and extracts result/error.
 * Returns 1 if found (regardless of success/failure), 0 if the TLV is
 * missing or the response is malformed. */
static int parse_result(const u8*r,u32 n,u16*result,u16*error){
 u32 p=7,e;if(n<7)return 0;e=7u+le16(r+5);if(e>n)e=n;
 while(p+3<=e){
  u8 id=r[p];u16 l=le16(r+p+1);const u8*v=r+p+3;
  if(p+3u+l>e)return 0;
  if(id==TLV_RESULT&&l>=4){*result=le16(v);*error=le16(v+2);return 1;}
  p+=3u+l;
 }
 return 0;
}

/* Walks every TLV in a request or response buffer and prints id, length,
 * and (for short values) the raw hex, decoding TLV_RESULT specially. */
static void dump_tlvs(const u8*r,u32 n){
 u32 p=7,e;
 if(n<7){out("  (too short to contain a QMI header)\n");return;}
 e=7u+le16(r+5);
 out("  header: flag=0x");out_hex8(r[0]);out(" txid=0x");out_hex16(le16(r+1));
 out(" msgid=0x");out_hex16(le16(r+3));out(" payload_len=");outnum(le16(r+5));
 out(" total_bytes=");outnum(n);out("\n");
 if(e>n){out("  WARNING: declared payload_len runs past the actual buffer, truncating.\n");e=n;}
 while(p+3<=e){
  u8 id=r[p];u16 l=le16(r+p+1);const u8*v=r+p+3;u32 i,shown;
  if(p+3u+l>e){out("  [TLV declares more bytes than remain -- truncated/malformed]\n");break;}
  out("  TLV id=0x");out_hex8(id);out(" (");out(tlv_name(id));out(") len=");outnum(l);
  if(id==TLV_RESULT&&l>=4){
   u16 res=le16(v),err=le16(v+2);
   out(" result=0x");out_hex16(res);out(" error=0x");out_hex16(err);
   out(res==0?" [SUCCESS]":" [FAILURE]");
  }
  out(" value=");
  shown=l<20?l:20;
  for(i=0;i<shown;i++){out_hex8(v[i]);if(i+1<shown)out(" ");}
  if(l>shown)out(" ...");
  out("\n");
  p+=3u+l;
 }
}
static u16 txid(void){struct timespec64 t;if(sc2(SYS_clock_gettime,CLOCK_MONOTONIC,(s64)&t)<0)return 1;return (u16)((u64)t.nsec^(u64)t.sec^((u64)t.nsec>>16));}

/* Sleeps for the given number of milliseconds. Used after a SET command,
 * since the device's NAS state (queried right back via GET) can lag a
 * moment behind an accepted SET -- without this gap, an immediate
 * post-command query can still read the pre-change values, which is why
 * "refresh" run manually a moment later used to show the correct state
 * when the automatic post-command query did not. Retries once on a signal
 * interruption (ret==-EINTR / -4) using the kernel-updated remaining time. */
static void sleep_ms(u64 ms){
 struct timespec64 req,rem;
 req.sec=(s64)(ms/1000u);
 req.nsec=(s64)((ms%1000u)*1000000u);
 if(sc2(SYS_nanosleep,(s64)&req,(s64)&rem)==-4){
  (void)sc2(SYS_nanosleep,(s64)&rem,0);
 }
}

static int discover_service(u32 service,u32*node,u32*port){
 struct qrtr_ctrl_pkt q,r;struct sockaddr_qrtr a;struct timeval64 tv={2,0};s64 fd,n;int t;
 fd=sc3(SYS_socket,AF_QIPCRTR,SOCK_DGRAM,0);if(fd<0)return 0;
 if(sc5(SYS_setsockopt,fd,SOL_SOCKET,SO_RCVTIMEO,(s64)&tv,sizeof(tv))<0){sc1(SYS_close,fd);return 0;}
 a.family=AF_QIPCRTR;a.pad=0;a.node=QRTR_CTRL_NODE;a.port=QRTR_PORT_CTRL;
 q.command=QRTR_TYPE_NEW_LOOKUP;q.service=service;q.instance=NAS_PACKED_INSTANCE;q.node=0;q.port=0;
 for(t=0;t<3;t++){n=sc6(SYS_sendto,fd,(s64)&q,sizeof(q),0,(s64)&a,sizeof(a));if(n!=(s64)sizeof(q))continue;for(;;){n=sc6(SYS_recvfrom,fd,(s64)&r,sizeof(r),0,0,0);if(n<0)break;if(n==(s64)sizeof(r)&&r.command==QRTR_TYPE_NEW_SERVER&&r.service==service&&r.instance==NAS_PACKED_INSTANCE&&r.port){*node=r.node;*port=r.port;sc1(SYS_close,fd);return 1;}}}
 sc1(SYS_close,fd);return 0;
}

static int open_nas(struct state*s){struct sockaddr_qrtr a;struct timeval64 tv={3,0};if(!discover_service(NAS_SERVICE,&s->node,&s->port)){setstatus(s,"NAS discovery failed.");return 0;}s->fd=sc3(SYS_socket,AF_QIPCRTR,SOCK_DGRAM,0);if(s->fd<0){setstatus(s,"NAS socket failed.");return 0;}if(sc5(SYS_setsockopt,s->fd,SOL_SOCKET,SO_RCVTIMEO,(s64)&tv,sizeof(tv))<0){sc1(SYS_close,s->fd);return 0;}a.family=AF_QIPCRTR;a.pad=0;a.node=s->node;a.port=s->port;if(sc3(SYS_connect,s->fd,(s64)&a,sizeof(a))<0){sc1(SYS_close,s->fd);return 0;}return 1;}

static int exchange(struct state*s,u16 msg,const u8*pl,u16 plen,u8*rsp,u32 cap,u32*rn){
 u8 req[640];u16 t=txid();s64 n;int skipped=0;int v=s->verbose&&(msg==MSG_SET||msg==MSG_GET);
 if(!t)t=1;if(7u+plen>sizeof(req))return 0;
 req[0]=0;put16(req+1,t);put16(req+3,msg);put16(req+5,plen);if(plen)copy(req+7,pl,plen);
 if(v){
  out("\n----- VERBOSE: outgoing SET request -----\n");
  dump_bytes("  raw TX: ",req,7u+plen);
  dump_tlvs(req,7u+plen);
 }
 n=sc6(SYS_sendto,s->fd,(s64)req,7u+plen,0,0,0);
 if(n!=(s64)(7u+plen)){if(v){out("  sendto() failed or short write, ret=");outnum((u32)(n<0?0:n));out("\n");}return 0;}
 /* A NAS socket may receive indications or a delayed response first. Keep
    reading until the response matching this transaction and message arrives. */
 for(;;){
  n=sc6(SYS_recvfrom,s->fd,(s64)rsp,cap,0,0,0);
  if(n<0){if(v)out("  recvfrom() failed / timed out waiting for a reply.\n");return 0;}
  if(v){
   out("----- VERBOSE: incoming packet (");outnum((u32)n);out(" bytes) -----\n");
   dump_bytes("  raw RX: ",rsp,(u32)n);
  }
  if(n>=7&&rsp[0]==2&&le16(rsp+1)==t&&le16(rsp+3)==msg){
   *rn=(u32)n;
   if(v){out("  ^ matches this transaction (txid=0x");out_hex16(t);out(", msgid=0x");out_hex16(msg);out(")\n");dump_tlvs(rsp,(u32)n);}
   return 1;
  }
  if(v)out("  (does not match this transaction/txid/msgid -- likely an unrelated indication, skipping)\n");
  if(++skipped>=16){if(v)out("  gave up after 16 non-matching packets.\n");return 0;}
 }
}
static int result_ok(const u8*r,u32 n){u16 res=0,err=0;if(!parse_result(r,n,&res,&err))return 0;return res==0;}
static int bind_sim(struct state*s,int sim){u8 p[4]={1,1,0,0},r[128];u32 n;p[3]=(u8)(sim-1);if(!exchange(s,MSG_BIND,p,4,r,sizeof(r),&n)||!result_ok(r,n)){setstatus(s,"SIM bind failed.");return 0;}s->sim=sim;return 1;}
static int query(struct state*s){u8 r[2048];u32 n,p,e;zero(s->legacy,8);zero(s->lte,8);zero(s->extlte,32);zero(s->sa,64);zero(s->nsa,64);s->rat=0;if(!exchange(s,MSG_GET,0,0,r,sizeof(r),&n)||!result_ok(r,n)){s->valid=0;setstatus(s,"State query failed.");return 0;}e=7u+le16(r+5);if(e>n)e=n;for(p=7;p+3<=e;){u8 id=r[p];u16 l=le16(r+p+1);const u8*v=r+p+3;if(p+3u+l>e)break;if(id==TLV_MODE&&l>=2)s->rat=le16(v);else if(id==TLV_LEGACY&&l==8)copy(s->legacy,v,8);else if(id==TLV_LTE&&l==8)copy(s->lte,v,8);else if(id==TLV_EXT_LTE_GET&&l==32)copy(s->extlte,v,32);else if(id==TLV_NR_SA_GET&&l==64)copy(s->sa,v,64);else if(id==TLV_NR_NSA_GET&&l==64)copy(s->nsa,v,64);
 /* Confirmed by a live capture (mode sa/nsa/both, each followed by a GET):
    on GET, id 0x2B ("NR_COMBINED" -- the same id the "nr" command uses on
    SET for a 64-byte combined band mask) is reused for a 4-byte current
    NR-mode report: value byte 0 was 0x02 after "mode sa", 0x01 after
    "mode nsa", 0x00 after "mode both" -- exactly the 0=both/1=nsa/2=sa
    encoding cmd_mode() already uses locally. Length must be 4 to avoid
    ever colliding with the 64-byte combined-mask meaning. */
 else if(id==TLV_NR_COMBINED&&l==4){
  u8 mv=v[0];
  if(mv<=2&&s->sim>=1&&s->sim<=2){s->nr_mode[s->sim-1]=(int)mv;s->nr_mode_known[s->sim-1]=1;}
 }
 p+=3u+l;}s->valid=1;return 1;}
static void mset(u8*m,u32 b);
static int mhas(const u8*m,u32 b);
static void pmask(const u8*m,u32 max,const char*pre);
static void pgsm(const u8*p);
static void pwcdma(const u8*p);
static void pgsm_effective(const struct state*s);
static void pwcdma_effective(const struct state*s);
static int wbit(u32 b);
static void set64(u8*p,u64 v);
static int dms_exchange(u32 node,u32 port,u8*rsp,u32 cap,u32*rn){
 struct sockaddr_qrtr a;struct timeval64 tv={3,0};u8 req[7];s64 fd,n;u16 t=txid();int skipped=0;
 fd=sc3(SYS_socket,AF_QIPCRTR,SOCK_DGRAM,0);if(fd<0)return 0;
 if(sc5(SYS_setsockopt,fd,SOL_SOCKET,SO_RCVTIMEO,(s64)&tv,sizeof(tv))<0){sc1(SYS_close,fd);return 0;}
 a.family=AF_QIPCRTR;a.pad=0;a.node=node;a.port=port;
 if(sc3(SYS_connect,fd,(s64)&a,sizeof(a))<0){sc1(SYS_close,fd);return 0;}
 if(!t)t=1;req[0]=0;put16(req+1,t);put16(req+3,DMS_MSG_GET_BANDS);put16(req+5,0);
 n=sc6(SYS_sendto,fd,(s64)req,7,0,0,0);if(n!=7){sc1(SYS_close,fd);return 0;}
 for(;;){
  n=sc6(SYS_recvfrom,fd,(s64)rsp,cap,0,0,0);if(n<0){sc1(SYS_close,fd);return 0;}
  if(n>=7&&rsp[0]==2&&le16(rsp+1)==t&&le16(rsp+3)==DMS_MSG_GET_BANDS){*rn=(u32)n;sc1(SYS_close,fd);return 1;}
  if(++skipped>=16){sc1(SYS_close,fd);return 0;}
 }
}
static int query_hardware(struct state*s){
 u32 node,port,n,p,e;u8 r[2048];
 zero(s->hw_legacy,8);zero(s->hw_lte,32);zero(s->hw_nr,64);s->hw_valid=0;
 if(!discover_service(DMS_SERVICE,&node,&port)){setstatus(s,"Hardware-band service discovery failed.");return 0;}
 if(!dms_exchange(node,port,r,sizeof(r),&n)||!result_ok(r,n)){setstatus(s,"Hardware-band query failed.");return 0;}
 e=7u+le16(r+5);if(e>n)e=n;
 for(p=7;p+3<=e;){
  u8 id=r[p];u16 l=le16(r+p+1);const u8*v=r+p+3;u32 i,count;
  if(p+3u+l>e)break;
  if(id==0x01&&l>=8)copy(s->hw_legacy,v,8);
  else if(id==0x10&&l>=8)copy(s->hw_lte,v,8);
  else if(id==0x12&&l>=2){count=le16(v);for(i=0;i<count&&2u+2u*i+1u<l;i++){u32 b=le16(v+2+2*i);if(b>=1&&b<=256)mset(s->hw_lte,b);}}
  else if(id==0x13&&l>=2){count=le16(v);for(i=0;i<count&&2u+2u*i+1u<l;i++){u32 b=le16(v+2+2*i);if(b>=1&&b<=512)mset(s->hw_nr,b);}}
  p+=3u+l;
 }
 s->hw_valid=1;return 1;
}
static void print_hardware(struct state*s){
 if(!s->hw_valid&&!query_hardware(s)){out("\nHARDWARE SUPPORTED BANDS: unavailable\n");return;}
 out("\nHARDWARE SUPPORTED BANDS:\nGSM: ");pgsm(s->hw_legacy);
 out("WCDMA: ");pwcdma(s->hw_legacy);
 out("LTE: ");pmask(s->hw_lte,256,"B");
 out("NR-NSA: ");pmask(s->hw_nr,512,"n");
 out("NR-SA: ");pmask(s->hw_nr,512,"n");
}
static int hw_gsm_has(const struct state*s,u32 b){u64 m=le64(s->hw_legacy);if(b==850)return(m&(1ULL<<19))!=0;if(b==900)return(m&((1ULL<<8)|(1ULL<<9)))!=0;if(b==1800)return(m&(1ULL<<7))!=0;if(b==1900)return(m&(1ULL<<21))!=0;return 0;}
static int hw_wcdma_has(const struct state*s,u32 b){int bit=wbit(b);return bit>=0&&(le64(s->hw_legacy)&(1ULL<<bit))!=0;}
static void print_rejected(const char*tech,const u32*v,int c,const u8*mask,u32 max,const char*pre){int i,f=1;out("Unsupported ");out(tech);out(" band(s): ");for(i=0;i<c;i++)if(v[i]>max||!mhas(mask,v[i])){if(!f)out(",");out(pre);outnum(v[i]);f=0;}out("\nSupported ");out(tech);out(" bands: ");pmask(mask,max,pre);}
static int addtlv(u8*p,int pos,u8 id,const u8*v,u16 l){int i;p[pos++]=id;put16(p+pos,l);pos+=2;for(i=0;i<l;i++)p[pos++]=v[i];return pos;}
static int setter(struct state*s,const u8*p,u16 l){
 u8 r[256];u32 n;u16 res=0,err=0;
 if(!exchange(s,MSG_SET,p,l,r,sizeof(r),&n)){setstatus(s,"Command failed: no reply.");return 0;}
 if(!parse_result(r,n,&res,&err)||res!=0){
  char msg[96];u64 i=0;const char*pre="Command rejected: result=0x";
  while(pre[i]&&i+1<sizeof(msg)){msg[i]=pre[i];i++;}
  /* result/error already dumped in full by exchange() above when verbose;
     this just puts a short version in the status line too. */
  msg[i]=0;out(msg);out_hex16(res);out(" error=0x");out_hex16(err);out("\n");
  setstatus(s,"Command rejected by modem -- see VERBOSE dump above.");
  return 0;
 }
 setstatus(s,"Command accepted.");
 return 1;
}

static int sep(char c){return c==','||c==' '||c=='\t';}
static int puint(const char*s,u32*v){u32 n=0;int d=0;while(*s){if(*s<'0'||*s>'9')return 0;n=n*10u+(u32)(*s-'0');if(n>10000)return 0;s++;d++;}*v=n;return d>0;}
static int plist(char*s,u32*v,int cap,u32 min,u32 max){int c=0;char*p=s,*a;u32 x;while(*p){while(*p&&sep(*p))p++;if(!*p)break;a=p;while(*p&&!sep(*p))p++;if(*p)*p++=0;if(!puint(a,&x)||x<min||x>max||c>=cap)return -1;v[c++]=x;}return c;}
static void mset(u8*m,u32 b){u32 x=b-1;m[x/8]|=(u8)(1u<<(x%8));}
static int mhas(const u8*m,u32 b){u32 x=b-1;return (m[x/8]&(u8)(1u<<(x%8)))!=0;}
static void pmask(const u8*m,u32 max,const char*pre){u32 b;int f=1;for(b=1;b<=max;b++)if(mhas(m,b)){if(!f)out(",");out(pre);outnum(b);f=0;}if(f)out("none");out("\n");}
static void pgsm(const u8*p){u64 m=le64(p);int f=1;if(m&(1ULL<<19)){out("850");f=0;}if(m&((1ULL<<8)|(1ULL<<9))){if(!f)out(",");out("900");f=0;}if(m&(1ULL<<7)){if(!f)out(",");out("1800");f=0;}if(m&(1ULL<<21)){if(!f)out(",");out("1900");f=0;}if(f)out("none");out("\n");}
static void pwcdma(const u8*p){u64 m=le64(p);int f=1;
#define PW(bit,b) do{if(m&(1ULL<<(bit))){if(!f)out(",");out("B");outnum(b);f=0;}}while(0)
 PW(22,1);PW(23,2);PW(24,3);PW(25,4);PW(26,5);PW(27,6);PW(48,7);PW(49,8);PW(50,9);PW(51,10);PW(52,11);PW(53,12);PW(54,13);PW(55,14);PW(56,15);PW(57,16);PW(58,17);PW(59,18);PW(60,19);
#undef PW
 if(f)out("none");out("\n");}
static void pgsm_effective(const struct state*s){
 u8 x[8];int i;
 if(!s->hw_valid){pgsm(s->legacy);return;}
 for(i=0;i<8;i++)x[i]=(u8)(s->legacy[i]&s->hw_legacy[i]);
 pgsm(x);
}
static void pwcdma_effective(const struct state*s){
 u8 x[8];int i;
 if(!s->hw_valid){pwcdma(s->legacy);return;}
 for(i=0;i<8;i++)x[i]=(u8)(s->legacy[i]&s->hw_legacy[i]);
 pwcdma(x);
}
static void prat(u16 m){int f=1;if(m==0xFF){out("AUTO\n");return;}
#define PR(bit,n) do{if(m&(bit)){if(!f)out(",");out(n);f=0;}}while(0)
 PR(4,"GSM");PR(8,"WCDMA");PR(16,"LTE");PR(64,"NR");
#undef PR
 if(f)out("none/unknown");out("\n");}
static void pnrmode(struct state*s){int i=s->sim-1;if(i<0||i>1||!s->nr_mode_known[i]){out("unknown\n");return;}if(s->nr_mode[i]==0)out("SA/NSA\n");else if(s->nr_mode[i]==1)out("NSA\n");else if(s->nr_mode[i]==2)out("SA\n");else out("unknown\n");}

static void draw_state(struct state*s){
 out("\n===============================\nCURRENT BANDS (SIM ");outnum((u32)s->sim);out(")\n");
 if(s->valid){
  out("RAT: ");prat(s->rat);
  out("GSM: ");pgsm_effective(s);
  out("WCDMA: ");pwcdma_effective(s);
  out("LTE: ");pmask(s->extlte,256,"B");
  out("NR-NSA: ");pmask(s->nsa,512,"n");
  out("NR-SA: ");pmask(s->sa,512,"n");
  out("NR MODE: ");pnrmode(s);
 }else{
  out("RAT: unavailable\nGSM: unavailable\nWCDMA: unavailable\nLTE: unavailable\nNR-NSA: unavailable\nNR-SA: unavailable\nNR MODE: unavailable\n");
 }
 if(s->show_hardware){
  out("\nHARDWARE SUPPORTED BANDS\n");
  if(s->hw_valid){
   out("GSM: ");pgsm(s->hw_legacy);
   out("WCDMA: ");pwcdma(s->hw_legacy);
   out("LTE: ");pmask(s->hw_lte,256,"B");
   out("NR-NSA: ");pmask(s->hw_nr,512,"n");
   out("NR-SA: ");pmask(s->hw_nr,512,"n");
  }else out("unavailable\n");
 }
 out("Apply settings for: SIM");outnum((u32)s->sim);out("\n");
 if(s->status[0]&&!eq(s->status,"Ready.")){out("Message: ");out(s->status);out("\n");}
 out("===============================\nInput: ");
 /* The hardware section is intentionally one-shot. The `hardware` command
    enables it for the next redraw only; clearing the flag here prevents it
    from appearing after every later command. */
 s->show_hardware=0;
}

static void draw_initial(struct state*s){
 out("===============================\n Qualcomm QMI Bandlock V3.2\n===============================\n");
 out("USAGE EXAMPLES\nsim 1\nsim 2\nrat auto\nrat gsm,wcdma,lte,nr\nlte 1,2,3,4,5\nlte all\nlte none\nnr 1,2,3,4,5\nsa 1,2,3,4,5\nnsa 1,2,3,4,5\nmode sa\nmode nsa\nmode both\nwcdma 1,5,8,19\ngsm 850,900,1800,1900\nlte all nr 1,2,38 rat lte,nr\nverbose on\nverbose off\nrefresh\nhardware\nreset\nexit\n");
 if(s->verbose)out("Verbose mode is ON (started with -verbose): every command prints the raw TX/RX\nbytes and decoded TLVs. Use 'verbose off' to quiet it.\n");
 else out("Verbose mode is OFF (default). Use 'verbose on', or relaunch with -verbose\n");
 draw_state(s);
}
/* Read a complete command line. ADB shell on Windows may deliver stdin one
 * character (or a small fragment) at a time instead of one canonical line.
 * The old implementation treated each fragment as a complete command, causing
 * the UI to redraw after every keypress. */
static s64 readline(char*b,u64 cap){
 u64 used=0;
 if(cap<2)return -1;
 for(;;){
  char c;
  s64 n=sc3(SYS_read,0,(s64)&c,1);
  if(n<0){b[used]=0;return used?(s64)used:n;}
  if(n==0){b[used]=0;return (s64)used;}
  if(c=='\r'||c=='\n'){
   out("\n");
   b[used]=0;
   return (s64)used;
  }
  if(c==8||c==127){
   if(used){used--;out("\b \b");}
   continue;
  }
  if(c>=' '&&c!=127){
   if(used+1<cap){
    b[used++]=c;
    (void)sc3(SYS_write,1,(s64)&c,1);
   }
  }
 }
}

static int cmd_rat(struct state*s,char*a){u8 p[16],d=1,m[2];int pos=0;u16 mask=0;if(eq(a,"auto"))mask=0xFF;else{char*x=a,*z;while(*x){while(*x&&sep(*x))x++;if(!*x)break;z=x;while(*x&&!sep(*x))x++;if(*x)*x++=0;if(eq(z,"gsm"))mask|=4;else if(eq(z,"wcdma")||eq(z,"umts"))mask|=8;else if(eq(z,"lte"))mask|=16;else if(eq(z,"nr")||eq(z,"nr5g"))mask|=64;else{setstatus(s,"Invalid RAT token.");return 0;}}if(!mask)return 0;}m[0]=(u8)mask;m[1]=(u8)(mask>>8);pos=addtlv(p,pos,TLV_DURATION,&d,1);pos=addtlv(p,pos,TLV_MODE,m,2);return setter(s,p,(u16)pos);}
static int cmd_lte_hardware(struct state*s){
 u8 l[8],p[48],d=1;int pos=0,i,has_high=0,any=0;
 if(!s->hw_valid&&!query_hardware(s))return 0;
 for(i=0;i<32;i++)if(s->hw_lte[i]){any=1;if(i>=8)has_high=1;}
 if(!any){setstatus(s,"No hardware-supported LTE bands were reported.");return 0;}
 copy(l,s->hw_lte,8);
 pos=addtlv(p,pos,TLV_DURATION,&d,1);
 if(has_high)pos=addtlv(p,pos,TLV_EXT_LTE_SET,s->hw_lte,32);
 else pos=addtlv(p,pos,TLV_LTE,l,8);
 return setter(s,p,(u16)pos);
}
static int cmd_nr_hardware(struct state*s,u8 id){
 u8 p[80],d=1;int pos=0,i,any=0;
 if(!s->hw_valid&&!query_hardware(s))return 0;
 for(i=0;i<64;i++)if(s->hw_nr[i]){any=1;break;}
 if(!any){setstatus(s,"No hardware-supported NR bands were reported.");return 0;}
 pos=addtlv(p,pos,TLV_DURATION,&d,1);
 pos=addtlv(p,pos,id,s->hw_nr,64);
 return setter(s,p,(u16)pos);
}
static int cmd_gsm_hardware(struct state*s){
 u64 m,hm;u8 x[8],p[20],d=1;int pos=0;
 if(!s->valid)return 0;
 if(!s->hw_valid&&!query_hardware(s))return 0;
 hm=le64(s->hw_legacy);
 m=le64(s->legacy)&~((1ULL<<7)|(1ULL<<8)|(1ULL<<9)|(1ULL<<19)|(1ULL<<21));
 m|=hm&((1ULL<<7)|(1ULL<<8)|(1ULL<<9)|(1ULL<<19)|(1ULL<<21));
 set64(x,m);pos=addtlv(p,pos,TLV_DURATION,&d,1);pos=addtlv(p,pos,TLV_LEGACY,x,8);
 return setter(s,p,(u16)pos);
}
static int cmd_wcdma_hardware(struct state*s){
 u64 m,hm,wm=0;u8 x[8],p[20],d=1;int pos=0,b,i;
 if(!s->valid)return 0;
 if(!s->hw_valid&&!query_hardware(s))return 0;
 for(i=1;i<=19;i++){b=wbit((u32)i);wm|=1ULL<<b;}
 hm=le64(s->hw_legacy);m=(le64(s->legacy)&~wm)|(hm&wm);
 set64(x,m);pos=addtlv(p,pos,TLV_DURATION,&d,1);pos=addtlv(p,pos,TLV_LEGACY,x,8);
 return setter(s,p,(u16)pos);
}

static int cmd_lte(struct state*s,char*a){
 u32 v[128];int c,i,pos=0,bad=0,has_high=0;u8 l[8],e[32],p[64],d=1;
 if(eq(a,"hardware")||eq(a,"all"))return cmd_lte_hardware(s);
 if(eq(a,"none")){
  zero(l,8);pos=addtlv(p,pos,TLV_DURATION,&d,1);pos=addtlv(p,pos,TLV_LTE,l,8);
  return setter(s,p,(u16)pos);
 }
 c=plist(a,v,128,1,256);
 if(c<=0){setstatus(s,"Invalid LTE list.");return 0;}
 if(!s->hw_valid&&!query_hardware(s))return 0;
 for(i=0;i<c;i++){
  if(!mhas(s->hw_lte,v[i]))bad=1;
  if(v[i]>64)has_high=1;
 }
 if(bad){print_rejected("LTE",v,c,s->hw_lte,256,"B");setstatus(s,"LTE command not sent.");return 0;}
 zero(l,8);zero(e,32);
 for(i=0;i<c;i++){
  mset(e,v[i]);
  if(v[i]<=64)mset(l,v[i]);
 }
 pos=addtlv(p,pos,TLV_DURATION,&d,1);
 /* Qualcomm firmware differs here. The tested modem rejects a request that
    carries both legacy LTE TLV 0x15 and extended LTE TLV 0x24 together.
    Use the known-good legacy-only packet for B1-B64. If any requested band is
    above B64, use the extended bitmap alone, which can also represent low bands. */
 if(has_high)pos=addtlv(p,pos,TLV_EXT_LTE_SET,e,32);
 else pos=addtlv(p,pos,TLV_LTE,l,8);
 return setter(s,p,(u16)pos);
}
static int cmd_nr(struct state*s,char*a,u8 id){u32 v[128];int c,i,pos=0,bad=0;u8 m[64],p[80],d=1;if(eq(a,"hardware")||eq(a,"all"))return cmd_nr_hardware(s,id);if(eq(a,"none")){zero(m,64);pos=addtlv(p,pos,TLV_DURATION,&d,1);pos=addtlv(p,pos,id,m,64);return setter(s,p,(u16)pos);}c=plist(a,v,128,1,512);if(c<=0){setstatus(s,"Invalid NR list.");return 0;}if(!s->hw_valid&&!query_hardware(s))return 0;for(i=0;i<c;i++)if(!mhas(s->hw_nr,v[i]))bad=1;if(bad){print_rejected("NR",v,c,s->hw_nr,512,"n");setstatus(s,"NR command not sent.");return 0;}zero(m,64);for(i=0;i<c;i++)mset(m,v[i]);pos=addtlv(p,pos,TLV_DURATION,&d,1);pos=addtlv(p,pos,id,m,64);return setter(s,p,(u16)pos);}
static void set64(u8*p,u64 v){int i;for(i=0;i<8;i++)p[i]=(u8)(v>>(8*i));}
static int cmd_gsm(struct state*s,char*a){u32 v[16];int c,i,pos=0,bad=0;u64 m;u8 x[8],p[20],d=1;if(eq(a,"hardware")||eq(a,"all"))return cmd_gsm_hardware(s);if(!s->valid)return 0;if(eq(a,"none")){m=le64(s->legacy)&~((1ULL<<7)|(1ULL<<8)|(1ULL<<9)|(1ULL<<19)|(1ULL<<21));set64(x,m);pos=addtlv(p,pos,TLV_DURATION,&d,1);pos=addtlv(p,pos,TLV_LEGACY,x,8);return setter(s,p,(u16)pos);}c=plist(a,v,16,1,2000);if(c<=0){setstatus(s,"Invalid GSM list.");return 0;}if(!s->hw_valid&&!query_hardware(s))return 0;for(i=0;i<c;i++)if(!hw_gsm_has(s,v[i]))bad=1;if(bad){out("Unsupported GSM band(s): ");{int f=1;for(i=0;i<c;i++)if(!hw_gsm_has(s,v[i])){if(!f)out(",");outnum(v[i]);f=0;}}out("\nSupported GSM bands: ");pgsm(s->hw_legacy);setstatus(s,"GSM command not sent.");return 0;}m=le64(s->legacy)&~((1ULL<<7)|(1ULL<<8)|(1ULL<<9)|(1ULL<<19)|(1ULL<<21));for(i=0;i<c;i++){if(v[i]==850)m|=1ULL<<19;else if(v[i]==900)m|=(1ULL<<8)|(1ULL<<9);else if(v[i]==1800)m|=1ULL<<7;else if(v[i]==1900)m|=1ULL<<21;}set64(x,m);pos=addtlv(p,pos,TLV_DURATION,&d,1);pos=addtlv(p,pos,TLV_LEGACY,x,8);return setter(s,p,(u16)pos);}
static int wbit(u32 b){switch(b){case 1:return 22;case 2:return 23;case 3:return 24;case 4:return 25;case 5:return 26;case 6:return 27;case 7:return 48;case 8:return 49;case 9:return 50;case 10:return 51;case 11:return 52;case 12:return 53;case 13:return 54;case 14:return 55;case 15:return 56;case 16:return 57;case 17:return 58;case 18:return 59;case 19:return 60;default:return -1;}}
static int cmd_wcdma(struct state*s,char*a){u32 v[32];int c,i,b,pos=0,bad=0;u64 m,clr=0;u8 x[8],p[20],d=1;if(eq(a,"hardware")||eq(a,"all"))return cmd_wcdma_hardware(s);if(!s->valid)return 0;if(eq(a,"none")){for(i=1;i<=19;i++){b=wbit((u32)i);clr|=1ULL<<b;}m=le64(s->legacy)&~clr;set64(x,m);pos=addtlv(p,pos,TLV_DURATION,&d,1);pos=addtlv(p,pos,TLV_LEGACY,x,8);return setter(s,p,(u16)pos);}c=plist(a,v,32,1,19);if(c<=0){setstatus(s,"Invalid WCDMA list.");return 0;}if(!s->hw_valid&&!query_hardware(s))return 0;for(i=0;i<c;i++)if(!hw_wcdma_has(s,v[i]))bad=1;if(bad){out("Unsupported WCDMA band(s): ");{int f=1;for(i=0;i<c;i++)if(!hw_wcdma_has(s,v[i])){if(!f)out(",");out("B");outnum(v[i]);f=0;}}out("\nSupported WCDMA bands: ");pwcdma(s->hw_legacy);setstatus(s,"WCDMA command not sent.");return 0;}for(i=1;i<=19;i++){b=wbit((u32)i);clr|=1ULL<<b;}m=le64(s->legacy)&~clr;for(i=0;i<c;i++){b=wbit(v[i]);m|=1ULL<<b;}set64(x,m);pos=addtlv(p,pos,TLV_DURATION,&d,1);pos=addtlv(p,pos,TLV_LEGACY,x,8);return setter(s,p,(u16)pos);}


/* Restore every band family to the modem-reported hardware capability for
 * the currently bound SIM. GSM and WCDMA share TLV 0x12, so they must be
 * restored together in one legacy-mask write; issuing the two old helpers
 * back-to-back would let the second write rebuild from stale cached state
 * and accidentally undo the first. LTE and the three NR domains remain
 * independent SET transactions, matching the proven incremental setters. */
static int cmd_reset(struct state*s){
 u8 legacy[8],p[80],d=1;int pos,i,has_lte_high=0,any_lte=0,any_nr=0;
 if(!s->hw_valid&&!query_hardware(s))return 0;

 /* GSM + WCDMA: the DMS legacy capability mask already contains both. */
 copy(legacy,s->hw_legacy,8);
 pos=0;pos=addtlv(p,pos,TLV_DURATION,&d,1);pos=addtlv(p,pos,TLV_LEGACY,legacy,8);
 if(!setter(s,p,(u16)pos))return 0;

 /* LTE: use legacy 0x15 only when every supported band fits B1-B64;
    otherwise use the extended 0x24 bitmap alone. */
 for(i=0;i<32;i++)if(s->hw_lte[i]){any_lte=1;if(i>=8)has_lte_high=1;}
 if(!any_lte){setstatus(s,"Reset failed: no hardware LTE bands reported.");return 0;}
 pos=0;pos=addtlv(p,pos,TLV_DURATION,&d,1);
 if(has_lte_high)pos=addtlv(p,pos,TLV_EXT_LTE_SET,s->hw_lte,32);
 else pos=addtlv(p,pos,TLV_LTE,s->hw_lte,8);
 if(!setter(s,p,(u16)pos))return 0;

 for(i=0;i<64;i++)if(s->hw_nr[i]){any_nr=1;break;}
 if(!any_nr){setstatus(s,"Reset failed: no hardware NR bands reported.");return 0;}

 /* Restore combined NR, independent SA, and independent NSA masks. */
 pos=0;pos=addtlv(p,pos,TLV_DURATION,&d,1);pos=addtlv(p,pos,TLV_NR_COMBINED,s->hw_nr,64);
 if(!setter(s,p,(u16)pos))return 0;
 pos=0;pos=addtlv(p,pos,TLV_DURATION,&d,1);pos=addtlv(p,pos,TLV_NR_SA_SET,s->hw_nr,64);
 if(!setter(s,p,(u16)pos))return 0;
 pos=0;pos=addtlv(p,pos,TLV_DURATION,&d,1);pos=addtlv(p,pos,TLV_NR_NSA_SET,s->hw_nr,64);
 if(!setter(s,p,(u16)pos))return 0;

 setstatus(s,"All band masks restored to hardware-supported bands.");
 return 1;
}


/* Proven mode-only setter from the original menu. Deliberately sends no LTE
 * or NR band masks: duration (0x17) + NR operating mode (0x2E) only. */
static int cmd_mode(struct state*s,char*a){
 u8 p[20],d=1,m[4]={0,0,0,0};int pos=0,value;
 if(eq(a,"both"))value=0;
 else if(eq(a,"nsa"))value=1;
 else if(eq(a,"sa"))value=2;
 else{setstatus(s,"Use mode sa, mode nsa, or mode both.");return 0;}
 m[0]=(u8)value;
 pos=addtlv(p,pos,TLV_DURATION,&d,1);
 pos=addtlv(p,pos,TLV_NR_MODE,m,4);
 if(!setter(s,p,(u16)pos))return 0;
 if(s->sim>=1&&s->sim<=2){s->nr_mode[s->sim-1]=value;s->nr_mode_known[s->sim-1]=1;}
 return 1;
}

static int reopen_bound(struct state*s,int sim){
 if(s->fd>=0){sc1(SYS_close,s->fd);s->fd=-1;}
 if(!open_nas(s))return 0;
 if(!bind_sim(s,sim))return 0;
 return 1;
}

static int wordeq(const char*s,u32 n,const char*w){u32 i=0;while(w[i]&&i<n&&s[i]==w[i])i++;return i==n&&w[i]==0;}
static int command_word(const char*s,u32 n){
 return wordeq(s,n,"sim")||wordeq(s,n,"rat")||wordeq(s,n,"lte")||
        wordeq(s,n,"nr")||wordeq(s,n,"nsa")||wordeq(s,n,"sa")||
        wordeq(s,n,"mode")||wordeq(s,n,"gsm")||wordeq(s,n,"wcdma")||
        wordeq(s,n,"verbose")||wordeq(s,n,"refresh")||
        wordeq(s,n,"hardware")||wordeq(s,n,"reset")||wordeq(s,n,"exit");
}
static int command_needs_arg(const char*s,u32 n){
 return !(wordeq(s,n,"refresh")||wordeq(s,n,"hardware")||wordeq(s,n,"reset")||wordeq(s,n,"exit"));
}

/* Execute exactly one already-separated command.
 * Return: -1 exit, 0 failure, 1 success/no delayed GET needed,
 *         2 successful modem SET (caller performs one shared delay+GET). */
static int process_one(struct state*s,char*l){
 char*a;int ok;
 if(eq(l,"exit"))return -1;
 if(eq(l,"hardware")){s->show_hardware=1;if(!query_hardware(s))return 0;setstatus(s,"Ready.");return 1;}
 if(eq(l,"reset"))return cmd_reset(s)?2:0;
 if(starts(l,"verbose ")){
  a=l+8;
  if(eq(a,"on")){s->verbose=1;setstatus(s,"Verbose mode on: full TX/RX bytes and decoded TLVs will print for every set command.");}
  else if(eq(a,"off")){s->verbose=0;setstatus(s,"Verbose mode off.");}
  else{setstatus(s,"Use verbose on or verbose off.");return 0;}
  return 1;
 }
 if(eq(l,"refresh")){
  int sim=s->sim;
  if(!reopen_bound(s,sim)){s->valid=0;return 0;}
  if(!query(s))return 0;
  setstatus(s,"State refreshed.");
  return 1;
 }
 if(starts(l,"sim ")){
  a=l+4;
  if(eq(a,"1")||eq(a,"2")){
   int sim=a[0]-'0';
   if(reopen_bound(s,sim)){
    if(query(s))setstatus(s,sim==1?"Now applying settings to SIM1.":"Now applying settings to SIM2.");
    else return 0;
   }else{s->valid=0;return 0;}
  }else{setstatus(s,"Use sim 1 or sim 2.");return 0;}
  return 1;
 }
 if(starts(l,"rat "))ok=cmd_rat(s,l+4);
 else if(starts(l,"lte "))ok=cmd_lte(s,l+4);
 else if(starts(l,"nr "))ok=cmd_nr(s,l+3,TLV_NR_COMBINED);
 else if(starts(l,"nsa "))ok=cmd_nr(s,l+4,TLV_NR_NSA_SET);
 else if(starts(l,"sa "))ok=cmd_nr(s,l+3,TLV_NR_SA_SET);
 else if(starts(l,"mode "))ok=cmd_mode(s,l+5);
 else if(starts(l,"gsm "))ok=cmd_gsm(s,l+4);
 else if(starts(l,"wcdma "))ok=cmd_wcdma(s,l+6);
 else{setstatus(s,"Unknown command.");return 0;}
 return ok?2:0;
}

/* Supports both:
 *   lte all nr 1,2,38 rat lte,nr
 *   lte all; nr 1,2,38; rat lte,nr
 *
 * A command argument ends at ';' or immediately before the next recognized
 * command keyword. This still permits spaced lists such as "lte 1, 3, 7"
 * because numeric tokens are not command keywords. All modem SETs execute in
 * order, followed by one shared one-second delay and one final state query. */
static int process(struct state*s,char*l){
 u32 pos=0,len=(u32)slen(l);int did_set=0;
 while(pos<len){
  u32 cs,ce,as,ae,i,j;char one[512];int r;
  while(pos<len&&(l[pos]==' '||l[pos]=='\t'||l[pos]==';'))pos++;
  if(pos>=len)break;
  cs=pos;while(pos<len&&l[pos]!=' '&&l[pos]!='\t'&&l[pos]!=';')pos++;ce=pos;
  if(!command_word(l+cs,ce-cs)){setstatus(s,"Unknown command in chain.");return 1;}
  if(!command_needs_arg(l+cs,ce-cs)){
   i=0;for(j=cs;j<ce&&i+1<sizeof(one);j++)one[i++]=l[j];one[i]=0;
  }else{
   while(pos<len&&(l[pos]==' '||l[pos]=='\t'))pos++;
   if(pos>=len||l[pos]==';'){setstatus(s,"Missing command argument.");return 1;}
   as=pos;ae=len;
   for(i=pos;i<len;i++){
    if(l[i]==';'){ae=i;break;}
    if(l[i]==' '||l[i]=='\t'){
     u32 k=i;while(k<len&&(l[k]==' '||l[k]=='\t'))k++;
     if(k<len&&l[k]!=';'){
      u32 we=k;while(we<len&&l[we]!=' '&&l[we]!='\t'&&l[we]!=';')we++;
      if(command_word(l+k,we-k)){ae=i;break;}
     }
    }
   }
   while(ae>as&&(l[ae-1]==' '||l[ae-1]=='\t'))ae--;
   i=0;for(j=cs;j<ce&&i+1<sizeof(one);j++)one[i++]=l[j];
   if(i+1<sizeof(one))one[i++]=' ';
   for(j=as;j<ae&&i+1<sizeof(one);j++)one[i++]=l[j];one[i]=0;
   pos=ae;
  }
  r=process_one(s,one);
  if(r<0)return -1;
  if(r==0)return 1; /* stop the chain on the first failed command */
  if(r==2)did_set=1;
 }
 if(did_set){sleep_ms(1000);query(s);}
 return 1;
}

static int run(int argc,char**argv){
 struct state s;char line[512];int i,verbose_flag=0;
 for(i=1;i<argc;i++)if(eq(argv[i],"-verbose"))verbose_flag=1;
 zero(&s,sizeof(s));s.fd=-1;s.sim=1;s.verbose=verbose_flag;setstatus(&s,"Starting...");if(!open_nas(&s)){draw_initial(&s);return 2;}if(!bind_sim(&s,1)){draw_initial(&s);sc1(SYS_close,s.fd);return 3;}query(&s);(void)query_hardware(&s);setstatus(&s,"Ready.");draw_initial(&s);for(;;){if(readline(line,sizeof(line))<0)break;if(!line[0]){out("Input: ");continue;}if(process(&s,line)<0)break;draw_state(&s);}out("\nQualcomm QMI Bandlock V3.2 closed.\n");if(s.fd>=0)sc1(SYS_close,s.fd);return 0;
}
void c_start(u64*stack){int rc;int argc=(int)stack[0];char**argv=(char**)&stack[1];rc=run(argc,argv);sc1(SYS_exit,rc);for(;;){}}
__asm__(".global _start\n.type _start,%function\n_start:\nmov x0,sp\nbl c_start\nb .\n");