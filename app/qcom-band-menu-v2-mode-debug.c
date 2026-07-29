/*
 * qcom-band-menu-v2-mode-debug.c
 * Interactive Qualcomm QMI NAS forcing UI for rooted ARM64 Android.
 *
 * Commands:
 *   sim 1 | sim 2
 *   rat auto | rat gsm,wcdma,lte,nr
 *   gsm 850,900,1800,1900
 *   wcdma 1,2,4,5,6,8,19
 *   lte 1,2,3,66,71
 *   nr 1,3,28,41,78
 *   mode sa | mode nsa | mode both
 *   hardware
 *   refresh
 *   exit
 *
 * Build:
 * clang --target=aarch64-linux-gnu -fuse-ld=lld -O2 -nostdlib -static \
 *   -fno-stack-protector -fno-builtin -Wl,-e,_start \
 *   -Wl,--build-id=none -o qcom-band-menu-v2 qcom-band-menu-v2-mode-debug.c
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;
typedef long s64;

enum { SYS_close=57,SYS_read=63,SYS_write=64,SYS_exit=93,SYS_clock_gettime=113,
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
#define TLV_NR_SA 0x2Cu
#define TLV_NR_NSA 0x2Du
#define TLV_NR_MODE 0x2Eu

struct sockaddr_qrtr{u16 family,pad;u32 node,port;};
struct qrtr_ctrl_pkt{u32 command,service,instance,node,port;};
struct timeval64{s64 sec,usec;};
struct timespec64{s64 sec,nsec;};
struct state{s64 fd;u32 node,port;int sim;u16 rat;u8 legacy[8],lte[8],extlte[32],sa[64],nsa[64];int valid;u8 hw_legacy[8],hw_lte[32],hw_nr[64];int hw_valid;int nr_mode[2],nr_mode_known[2];char status[160];};

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
static u16 txid(void){struct timespec64 t;if(sc2(SYS_clock_gettime,CLOCK_MONOTONIC,(s64)&t)<0)return 1;return (u16)((u64)t.nsec^(u64)t.sec^((u64)t.nsec>>16));}

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
 u8 req[640];u16 t=txid();s64 n;int skipped=0;
 if(!t)t=1;if(7u+plen>sizeof(req))return 0;
 req[0]=0;put16(req+1,t);put16(req+3,msg);put16(req+5,plen);if(plen)copy(req+7,pl,plen);
 n=sc6(SYS_sendto,s->fd,(s64)req,7u+plen,0,0,0);if(n!=(s64)(7u+plen))return 0;
 /* A NAS socket may receive indications or a delayed response first. Keep
    reading until the response matching this transaction and message arrives. */
 for(;;){
  n=sc6(SYS_recvfrom,s->fd,(s64)rsp,cap,0,0,0);if(n<0)return 0;
  if(n>=7&&rsp[0]==2&&le16(rsp+1)==t&&le16(rsp+3)==msg){*rn=(u32)n;return 1;}
  if(++skipped>=16)return 0;
 }
}
static int result_ok(const u8*r,u32 n){u32 p=7,e;if(n<7)return 0;e=7u+le16(r+5);if(e>n)e=n;while(p+3<=e){u8 id=r[p];u16 l=le16(r+p+1);const u8*v=r+p+3;if(p+3u+l>e)return 0;if(id==TLV_RESULT&&l>=4)return le16(v)==0;p+=3u+l;}return 0;}
static int bind_sim(struct state*s,int sim){u8 p[4]={1,1,0,0},r[128];u32 n;p[3]=(u8)(sim-1);if(!exchange(s,MSG_BIND,p,4,r,sizeof(r),&n)||!result_ok(r,n)){setstatus(s,"SIM bind failed.");return 0;}s->sim=sim;return 1;}
static int query(struct state*s){u8 r[2048];u32 n,p,e;zero(s->legacy,8);zero(s->lte,8);zero(s->extlte,32);zero(s->sa,64);zero(s->nsa,64);s->rat=0;if(!exchange(s,MSG_GET,0,0,r,sizeof(r),&n)||!result_ok(r,n)){s->valid=0;setstatus(s,"State query failed.");return 0;}e=7u+le16(r+5);if(e>n)e=n;for(p=7;p+3<=e;){u8 id=r[p];u16 l=le16(r+p+1);const u8*v=r+p+3;if(p+3u+l>e)break;if(id==TLV_MODE&&l>=2)s->rat=le16(v);else if(id==TLV_LEGACY&&l==8)copy(s->legacy,v,8);else if(id==TLV_LTE&&l==8)copy(s->lte,v,8);else if(id==TLV_EXT_LTE_GET&&l==32)copy(s->extlte,v,32);else if(id==TLV_NR_SA&&l==64)copy(s->sa,v,64);else if(id==TLV_NR_NSA&&l==64)copy(s->nsa,v,64);else if(id==TLV_NR_MODE&&l>=4){int i=s->sim-1;if(i>=0&&i<=1){s->nr_mode[i]=v[0];s->nr_mode_known[i]=1;}}p+=3u+l;}s->valid=1;return 1;}
static void mset(u8*m,u32 b);
static int mhas(const u8*m,u32 b);
static void pmask(const u8*m,u32 max,const char*pre);
static void pgsm(const u8*p);
static void pwcdma(const u8*p);
static int wbit(u32 b);
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
static int setter(struct state*s,const u8*p,u16 l){u8 r[256];u32 n;if(!exchange(s,MSG_SET,p,l,r,sizeof(r),&n)){setstatus(s,"Command failed: no reply.");return 0;}if(!result_ok(r,n)){setstatus(s,"Command rejected by modem.");return 0;}setstatus(s,"Command accepted.");return 1;}

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
static void prat(u16 m){int f=1;if(m==0xFF){out("AUTO\n");return;}
#define PR(bit,n) do{if(m&(bit)){if(!f)out(",");out(n);f=0;}}while(0)
 PR(4,"GSM");PR(8,"WCDMA");PR(16,"LTE");PR(64,"NR");
#undef PR
 if(f)out("none/unknown");out("\n");}
static void pnrmode(struct state*s){int i=s->sim-1;if(i<0||i>1||!s->nr_mode_known[i]){out("unknown\n");return;}if(s->nr_mode[i]==0)out("BOTH\n");else if(s->nr_mode[i]==1)out("NSA\n");else if(s->nr_mode[i]==2)out("SA\n");else out("unknown\n");}

static void draw_state(struct state*s){
 out("\n===============================\nCURRENT BANDS (SIM ");outnum((u32)s->sim);out(")\n");
 if(s->valid){
  out("RAT: ");prat(s->rat);
  out("GSM: ");pgsm(s->legacy);
  out("WCDMA: ");pwcdma(s->legacy);
  out("LTE: ");pmask(s->extlte,256,"B");
  out("NR-NSA: ");pmask(s->nsa,512,"n");
  out("NR-SA: ");pmask(s->sa,512,"n");
  out("NR MODE: ");pnrmode(s);
 }else{
  out("RAT: unavailable\nGSM: unavailable\nWCDMA: unavailable\nLTE: unavailable\nNR-NSA: unavailable\nNR-SA: unavailable\nNR MODE: unavailable\n");
 }
 out("Apply settings for: SIM");outnum((u32)s->sim);out("\n");
 if(s->status[0]&&!eq(s->status,"Ready.")){out("Message: ");out(s->status);out("\n");}
 out("===============================\nInput: ");
}

static void draw_initial(struct state*s){
 out("===============================\n Qualcomm QMI Band Menu V2 Mode Debug\n===============================\n");
 out("USAGE EXAMPLES\nsim 1\nsim 2\nrat auto\nrat gsm,wcdma,lte,nr\nlte 1,2,3,4,5\nnr 1,2,3,4,5\nmode sa\nmode nsa\nmode both\nwcdma 1,5,8,19\ngsm 850,900,1800,1900\nrefresh\nexit\n");
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
static int cmd_lte(struct state*s,char*a){
 u32 v[128];int c,i,pos=0,bad=0,has_high=0;u8 l[8],e[32],p[64],d=1;
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
static int cmd_nr(struct state*s,char*a,u8 id){u32 v[128];int c,i,pos=0,bad=0;u8 m[64],p[80],d=1;c=plist(a,v,128,1,512);if(c<=0){setstatus(s,"Invalid NR list.");return 0;}if(!s->hw_valid&&!query_hardware(s))return 0;for(i=0;i<c;i++)if(!mhas(s->hw_nr,v[i]))bad=1;if(bad){print_rejected("NR",v,c,s->hw_nr,512,"n");setstatus(s,"NR command not sent.");return 0;}zero(m,64);for(i=0;i<c;i++)mset(m,v[i]);pos=addtlv(p,pos,TLV_DURATION,&d,1);pos=addtlv(p,pos,id,m,64);return setter(s,p,(u16)pos);}
static void set64(u8*p,u64 v){int i;for(i=0;i<8;i++)p[i]=(u8)(v>>(8*i));}
static int cmd_gsm(struct state*s,char*a){u32 v[16];int c,i,pos=0,bad=0;u64 m;u8 x[8],p[20],d=1;if(!s->valid)return 0;c=plist(a,v,16,1,2000);if(c<=0){setstatus(s,"Invalid GSM list.");return 0;}if(!s->hw_valid&&!query_hardware(s))return 0;for(i=0;i<c;i++)if(!hw_gsm_has(s,v[i]))bad=1;if(bad){out("Unsupported GSM band(s): ");{int f=1;for(i=0;i<c;i++)if(!hw_gsm_has(s,v[i])){if(!f)out(",");outnum(v[i]);f=0;}}out("\nSupported GSM bands: ");pgsm(s->hw_legacy);setstatus(s,"GSM command not sent.");return 0;}m=le64(s->legacy)&~((1ULL<<7)|(1ULL<<8)|(1ULL<<9)|(1ULL<<19)|(1ULL<<21));for(i=0;i<c;i++){if(v[i]==850)m|=1ULL<<19;else if(v[i]==900)m|=(1ULL<<8)|(1ULL<<9);else if(v[i]==1800)m|=1ULL<<7;else if(v[i]==1900)m|=1ULL<<21;}set64(x,m);pos=addtlv(p,pos,TLV_DURATION,&d,1);pos=addtlv(p,pos,TLV_LEGACY,x,8);return setter(s,p,(u16)pos);}
static int wbit(u32 b){switch(b){case 1:return 22;case 2:return 23;case 3:return 24;case 4:return 25;case 5:return 26;case 6:return 27;case 7:return 48;case 8:return 49;case 9:return 50;case 10:return 51;case 11:return 52;case 12:return 53;case 13:return 54;case 14:return 55;case 15:return 56;case 16:return 57;case 17:return 58;case 18:return 59;case 19:return 60;default:return -1;}}
static int cmd_wcdma(struct state*s,char*a){u32 v[32];int c,i,b,pos=0,bad=0;u64 m,clr=0;u8 x[8],p[20],d=1;if(!s->valid)return 0;c=plist(a,v,32,1,19);if(c<=0){setstatus(s,"Invalid WCDMA list.");return 0;}if(!s->hw_valid&&!query_hardware(s))return 0;for(i=0;i<c;i++)if(!hw_wcdma_has(s,v[i]))bad=1;if(bad){out("Unsupported WCDMA band(s): ");{int f=1;for(i=0;i<c;i++)if(!hw_wcdma_has(s,v[i])){if(!f)out(",");out("B");outnum(v[i]);f=0;}}out("\nSupported WCDMA bands: ");pwcdma(s->hw_legacy);setstatus(s,"WCDMA command not sent.");return 0;}for(i=1;i<=19;i++){b=wbit((u32)i);clr|=1ULL<<b;}m=le64(s->legacy)&~clr;for(i=0;i<c;i++){b=wbit(v[i]);m|=1ULL<<b;}set64(x,m);pos=addtlv(p,pos,TLV_DURATION,&d,1);pos=addtlv(p,pos,TLV_LEGACY,x,8);return setter(s,p,(u16)pos);}


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

static int process(struct state*s,char*l){
 char*a;
 if(eq(l,"exit"))return -1;
 if(eq(l,"hardware")){if(query_hardware(s)){print_hardware(s);setstatus(s,"Hardware bands listed.");}return 1;}
 if(eq(l,"refresh")){
  int sim=s->sim;
  if(!reopen_bound(s,sim)){s->valid=0;return 1;}
  if(query(s))setstatus(s,"State refreshed.");
  return 1;
 }
 if(starts(l,"sim ")){
  a=l+4;
  if(eq(a,"1")||eq(a,"2")){
   int sim=a[0]-'0';
   if(reopen_bound(s,sim)){
    if(query(s))setstatus(s,sim==1?"Now applying settings to SIM1.":"Now applying settings to SIM2.");
   }else s->valid=0;
  }else setstatus(s,"Use sim 1 or sim 2.");
  return 1;
 }
 if(starts(l,"rat "))cmd_rat(s,l+4);
 else if(starts(l,"lte "))cmd_lte(s,l+4);
 else if(starts(l,"nr "))cmd_nr(s,l+3,TLV_NR_COMBINED);
 else if(starts(l,"mode "))cmd_mode(s,l+5);
 else if(starts(l,"nsa ")||starts(l,"sa "))setstatus(s,"Separate SA/NSA band masks are unsupported; use nr <bands> and mode <sa|nsa|both>.");
 else if(starts(l,"gsm "))cmd_gsm(s,l+4);
 else if(starts(l,"wcdma "))cmd_wcdma(s,l+6);
 else{setstatus(s,"Unknown command.");return 1;}
 query(s);return 1;
}

static int run(void){struct state s;char line[512];zero(&s,sizeof(s));s.fd=-1;s.sim=1;setstatus(&s,"Starting...");if(!open_nas(&s)){draw_initial(&s);return 2;}if(!bind_sim(&s,1)){draw_initial(&s);sc1(SYS_close,s.fd);return 3;}query(&s);setstatus(&s,"Ready.");draw_initial(&s);for(;;){if(readline(line,sizeof(line))<0)break;if(!line[0]){out("Input: ");continue;}if(process(&s,line)<0)break;draw_state(&s);}out("\nQualcomm QMI Band Menu Mode Debug closed.\n");if(s.fd>=0)sc1(SYS_close,s.fd);return 0;}
void c_start(u64*stack){int rc;(void)stack;rc=run();sc1(SYS_exit,rc);for(;;){}}
__asm__(".global _start\n.type _start,%function\n_start:\nmov x0,sp\nbl c_start\nb .\n");
