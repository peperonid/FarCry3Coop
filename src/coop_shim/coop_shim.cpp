// coop_shim.cpp - Route B "session-establishment shim" SCAFFOLD for Far Cry 3 (Uplay 1.06, x86).
//
// PURPOSE (design scaffold, NOT deployed, NOT tested): turn the RendezVous-bound co-op session
// establishment into a local storm::echo LAN session. See NOTES 28s-28ad and docs/coop-online-routes.md.
//
// All RVAs are for the RUNNING module FC3_d3d11.dll, md5 46bcc097, ImageBase 0x10000000 (NOT the DX9 FC3.dll).
// RVA = absolute - 0x10000000. If the module changes, re-derive every site (see scripts/probe_rv_live.py).
//
// THIS FILE IS A SKELETON: the detour plumbing is real; the per-hook bodies are intended-correct stubs with
// explicit TODO markers where the storm::echo CFCX session-op registry API and the exact struct ABIs must be
// filled in and validated against a running client (which the agent cannot attach to: FC3 blocks injection).
//
// BUILD: build_x86.bat -> coop_shim.dll (x86). DO NOT copy into the game without an explicit owner go-ahead.

#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

static FILE* g_logf = 0;
static HINSTANCE g_hinst = 0;
// Open (lazily) the file log: env FC3COOP_LOG wins, else "<dir of coop_shim.dll>\coop_shim.log".
static void logOpen(void) {
    if (g_logf) return;
    char path[MAX_PATH] = {0};
    const char* e = getenv("FC3COOP_LOG");
    if (e && e[0]) { lstrcpynA(path, e, MAX_PATH); }
    else {
        char self[MAX_PATH] = {0};
        if (g_hinst && GetModuleFileNameA(g_hinst, self, MAX_PATH)) {
            char* b = strrchr(self, 92); if (b) b[1] = 0;
            lstrcpynA(path, self, MAX_PATH); lstrcpynA(path + strlen(path), "coop_shim.log", MAX_PATH - (int)strlen(path));
        } else strcpy(path, "coop_shim.log");
    }
    g_logf = fopen(path, "a");
    if (g_logf) { fprintf(g_logf, "===== coop_shim log pid=%u =====\n", GetCurrentProcessId()); fflush(g_logf); }
}
#define LOGF(...) do { char _b[512]; int _n = wsprintfA(_b, __VA_ARGS__); OutputDebugStringA(_b); \
    if (g_logf) { fprintf(g_logf, "%s\n", _b); fflush(g_logf); } (void)_n; } while (0)

// ------------------------------------------------------------------ module + RVA map
#define FC3_BASE_DEFAULT 0x10000000u
static uintptr_t g_mod = 0;
static inline uintptr_t VA(uint32_t rva) { return g_mod + rva; }

// Sites (RVA = abs - 0x10000000). Source of truth: NOTES 28r-28ad.
enum {
  RVA_gate            = 0x01279ba0, // FUN_11279ba0 co-op session gate (online flag AND RV/SandboxUrl)
  RVA_host_handler    = 0x01275d90, // FUN_11275d90 host -> SubAccountClient RV auth (short-circuit, see hk_host)
  RVA_join_handler    = 0x0127a1d0, // FUN_1127a1d0 join -> UbiAuthentication  RV auth (short-circuit, see hk_join)
  RVA_join_online_var = 0x0127a300, // FUN_1127a300 online-join flavor (also gates on gate)
  RVA_online_flag     = 0x01245090, // FUN_11245090 global online-enable check (0 => gate fails)
  RVA_rv_sandboxurl   = 0x012492a0, // FUN_112492a0 RV net-op needing resolved SandboxUrl
  RVA_gate_addr_out   = 0x01232e20, // FUN_11232e20 gate address producer
  RVA_gate_port_out   = 0x01232ef0, // FUN_11232ef0 gate port producer (ntohs(*(p+2)))
  RVA_ticket_validate = 0x01254be0, // FUN_11254be0 MD5 validate (vtable+4)
  RVA_ticket_derive   = 0x01254ae0, // FUN_11254ae0 MD5 derive   (vtable+8)
  RVA_ticket_prepare  = 0x0124e5e0, // FUN_1124e5e0 AcquireTicket::PrepareCall (skip the network)
  RVA_ticket_response = 0x0124d660, // FUN_1124d660 AcquireTicket::ProcessResponse (out-ticket buffer)
  RVA_rv_connected    = 0x01237670, // FUN_11237670 "RendezVous connected" gate (AcquireTicket + builder require it)
  // storm::echo session-op registry (op types lazy-registered by NAME; C++ factory vtable - NOT hand-callable from a detour, NOTES 28ag):
  RVA_op_register     = 0x01008bf50, // FUN_1008bf50 register("CFCXCreateSessionOperation", factory)
  RVA_net_engine_init = 0x01092d256, // FUN_1092d256 net-engine init; param[0xe]: 0=LAN / 1=Online / 2=Offline
  RVA_join_precond    = 0x01127a0d0, // FUN_1127a0d0 join precondition; returns 0 unless rv_connected (28af hook unlocks it)
  RVA_login_event     = 0x007db689, // FUN_107db689 FCX online-login EVENT dispatcher (code 6=LoggingIn 9=Succeeded 0x11=Failed 0xf/0x10=Out), observer-only (NOTES 28dc)
  RVA_logout_op       = 0x007dc7b7, // FUN_107dc7b7 LogoutOperation (logs 'logout', posts FUN_107db689 code 0xf, tears down session) - target of BLOCK_LOGOUT (NOTES 28df)
  RVA_connect_auth_svc  = 0x01252f70, // FUN_11252f70 JobBackEndServicesLogin::ConnectToAuthenticationService - RV connect enqueue (RV_PROBE observer, 28dn)
  RVA_rv_sendto       = 0x01218510, // FUN_11218510 RV socket sendto wrapper (RV_PROBE observer, 28dn)
  RVA_rv_wsasendto    = 0x01214630, // FUN_11214630 RV socket WSASendTo wrapper (RV_PROBE observer, 28dn)
};

// Op-registry descriptors (static op-type data, created via their lazy registrar only; for reference, NOTES 28ag):
#define VA_DESC_CFCX_CREATE  0x11e6b208u // "CFCXCreateSessionOperation"  (via FUN_107de8fa)
#define VA_DESC_CFCX_JOIN    0x11e74ee0u // "CFCXJoinSessionOperation"    (via FUN_1092e764)
#define VA_DESC_SEARCHIP     0x11e42de8u // "CSessionSearchIPOperation"   (via FUN_10425918)
// Join/create op PARAM BLOCK schema (registered by FUN_1061e581): NetEngineType field id = 0x3c (0=LAN), SearchIdAddress, JoinSessionId.
#define OUTFIELD_NETENGINETYPE_ID 0x3c

// Online-enable global read by FUN_11245090 ("return DAT_11f07e48"). Setting it to 1 lets the gate precondition pass.
#define RVA_online_flag_global 0x01f07e48

// Master gate (NOTES 28av/28aw): the game-wide online session-manager singleton, abs 0x11e42c90, read ~250x and the
// first thing the co-op Host action 0x10443663 checks (bails if null). Offline it stays null. The path A'' tracer
// (FC3COOP_TRACE_MGR) waits for it to be populated during a NORMAL online login, then instruments its vtable to
// record which methods (slot offsets) the game actually calls -> the minimal interface a fake manager must implement.
#define RVA_online_mgr_global 0x01e42c90
// Online session-manager CLASS vtable (observed live NOTES 28ay; lives in .rdata so it is ASLR/login independent).
// A fake manager object can point its vptr here so the 16 hot slots stay stack-correct (correct ret-n-bytes); the
// cheap getters then just read the zeroed object's fields.
#define RVA_mgr_vtable 0x01527870
// FCX's OWN online-manager creator (NOTES 28cn): FUN_10420a77 = factory=FUN_10418a3d(); DAT_11e42c90=(factory->vtable[+4])();
// Its sole caller is FUN_1041936b. Calling this lets FCX construct a REAL manager instead of our zeroed fake.
#define RVA_manager_creator 0x00420a77

// Provider-factory probe site (NOTES 28ar): inside host_handler FUN_11275d90, the session provider is dispatched
// polymorphically via 'mov edx,[esi]; mov edx,[edx+0x14]; call edx'. Hook the 'call edx' site to log the provider
// object 'esi' + its vtable so the RV (and later LAN) provider class vtable can be identified.
#define RVA_provider_factory_site 0x01275e7e // VA 0x11275e7e: 'FF D2' = call edx
#define RVA_provider_factory_back 0x01275e86 // resume after displaced 'call edx' + 'mov ecx,[ebp-0x128]'

// The gate out-struct is a classic sockaddr_in (NOTES 28af): family@+0 u16, port@+2 network order
// (FUN_11232ef0 reads it via ntohs(*(p+2))), IPv4 addr@+4 network order (FUN_11232e20 via ntohl/inet_ntoa).

// ------------------------------------------------------------------ shim config (set by your loader / env / file)
struct ShimConfig { uint32_t lan_ip; uint16_t lan_port; char shared_secret[64]; int enable; };
static ShimConfig g_cfg = { 0x7f000001u /*127.0.0.1*/, 9000, "fc3coop", 0 }; // enable=0 => inert until configured
static int g_hook_provider = 0; // env FC3COOP_HOOK_PROVIDER=1 -> run the REAL host path (no host short-circuit) and probe the provider vtable
static int g_trace_mgr = 0;     // env FC3COOP_TRACE_MGR=1 -> instrument the online session-manager vtable (path A'' survey)
static int g_fake_mgr = 0;      // env FC3COOP_FAKE_MGR=1 -> install a zeroed fake manager so the [0x11e42c90]!=0 gate passes offline (path A'' experiment)
static int g_fake_force = 0;    // env FC3COOP_FAKE_MGR=2 -> watchdog also OVERRIDES a real manager (probe fake survivability while online)
static int g_mgr_call = 0;      // env FC3COOP_FAKE_MGR=3 -> do NOT fake the object; CALL the game's own creator FUN_10420a77 so FCX builds a real online manager (NOTES 28cn)
static int g_probe_login = 0;   // env FC3COOP_PROBE_LOGIN=1 -> entry-log the RV guest-login send + its async callback (A-vs-D discriminator)
static int g_no_ticket = 0;     // env FC3COOP_NO_TICKET=1 -> skip ticket_validate/ticket_prepare hooks (isolate post-callback crash)
static int g_fake_mgr_only = 0; // env FC3COOP_FAKE_MGR_ONLY=1 -> ONLY force the online-manager global [0x11e42c90]; do NOT write the online flag or patch any code (Stage0b: pass the FUN_10443663 '==0 -> service unavailable' gate with zero collateral code changes)
static int g_watch_login = 0;   // env FC3COOP_WATCH_LOGIN=1 -> ONLY observe FUN_107db689 login-event dispatch (log code), no other patches (NOTES 28dc)
static int g_block_logout = 0;  // env FC3COOP_BLOCK_LOGOUT=1 -> in WATCH_LOGIN mode, also skip FUN_107dc7b7 (LogoutOper
static int g_dns_redirect = 0;  // env FC3COOP_DNS_REDIRECT=1 -> IAT-patch getaddrinfo so onlineconfigservice.ubi.com resolves 127.0.0.1 (feed a crafted OnlineConfig with SandboxUrl->our rv_server, no hosts edit) (NOTES 28dh)
static int g_rv_probe = 0;      // env FC3COOP_RV_PROBE=1 -> OBSERVE whether FCX reaches the RV connect/dial path at Co-op entry (NOTES 28dn)ation) to stop the login<->logout cycle (NOTES 28df)
static uintptr_t g_fake_obj = 0;
static volatile LONG g_logbusy = 0;  // tiny spinlock so the tracer thread does not interleave LOGF with the main thread
static inline void logLock(){ while (InterlockedExchange(&g_logbusy,1)) { YieldProcessor(); } }
static inline void logUnlock(){ InterlockedExchange(&g_logbusy,0); }
static void load_config(void) {
  char buf[64];
  if (GetEnvironmentVariableA("FC3COOP_SHIM", buf, sizeof(buf)) > 0) g_cfg.enable = (buf[0] == '1');
  if (GetEnvironmentVariableA("FC3COOP_SECRET", buf, sizeof(buf)) > 0) { strncpy(g_cfg.shared_secret, buf, sizeof(g_cfg.shared_secret)-1); }
  if (GetEnvironmentVariableA("FC3COOP_HOOK_PROVIDER", buf, sizeof(buf)) > 0) g_hook_provider = (buf[0] == '1');
  if (GetEnvironmentVariableA("FC3COOP_TRACE_MGR", buf, sizeof(buf)) > 0) g_trace_mgr = (buf[0] == '1');
  if (GetEnvironmentVariableA("FC3COOP_FAKE_MGR", buf, sizeof(buf)) > 0) { if (buf[0] == '2') { g_fake_mgr = 1; g_fake_force = 1; } else if (buf[0] == '3') { g_fake_mgr = 1; g_mgr_call = 1; } else g_fake_mgr = (buf[0] == '1'); }
  if (GetEnvironmentVariableA("FC3COOP_PROBE_LOGIN", buf, sizeof(buf)) > 0) g_probe_login = (buf[0] == '1');
  if (GetEnvironmentVariableA("FC3COOP_NO_TICKET", buf, sizeof(buf)) > 0) g_no_ticket = (buf[0] == '1');
  if (GetEnvironmentVariableA("FC3COOP_FAKE_MGR_ONLY", buf, sizeof(buf)) > 0) g_fake_mgr_only = (buf[0] == '1');
  if (GetEnvironmentVariableA("FC3COOP_WATCH_LOGIN", buf, sizeof(buf)) > 0) g_watch_login = (buf[0] == '1');
  if (GetEnvironmentVariableA("FC3COOP_BLOCK_LOGOUT", buf, sizeof(buf)) > 0) g_block_logout = (buf[0] == '1');
  if (GetEnvironmentVariableA("FC3COOP_DNS_REDIRECT", buf, sizeof(buf)) > 0) g_dns_redirect = (buf[0] == '1');
  if (GetEnvironmentVariableA("FC3COOP_RV_PROBE", buf, sizeof(buf)) > 0) g_rv_probe = (buf[0] == '1');
  // TODO: also parse FC3COOP_HOST_IP (dotted -> uint32, network order) for the join address.
}

// ------------------------------------------------------------------ minimal x86 inline detour (E9 rel32)
// NOTE: replaces the first 5 bytes of *target*. Validated only for sites whose first 5 bytes are not
// IP-relative near the boundary; confirm each at runtime (disassemble before patching). Trampoline re-runs the
// saved bytes then jumps back - ONLY safe for the chosen sites; several hooks here never call original.
struct Detour { void* target; uintptr_t detour; uint8_t saved[5]; void* trampoline; };
static Detour g_hooks[] = {
  { (void*)0, 0, {0}, 0 }, // placeholder table; real per-hook saved bytes are a TODO (see install_hooks)
};

static void* make_trampoline(void* target, const uint8_t* saved5) {
  // exec trampoline: saved 5 bytes + jmp back to target+5
  uint8_t* tr = (uint8_t*)VirtualAlloc(0, 32, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  if (!tr) return 0;
  memcpy(tr, saved5, 5);
  tr[5] = 0xE9;                                  // jmp rel32
  int32_t rel = (int32_t)((uint8_t*)target + 5 - (tr + 10));
  memcpy(tr + 6, &rel, 4);
  return tr;
}
static int patch_5(void* target, void* detour) {
  DWORD old; VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &old);
  uint8_t* t = (uint8_t*)target;
  uint8_t jmp = 0xE9;
  int32_t rel = (int32_t)((uint8_t*)detour - ((uint8_t*)target + 5));
  memcpy(g_hooks[0].saved, t, 5);                // NOTE: per-hook saved; see install_hooks TODO
  memcpy(t, &jmp, 1);
  memcpy(t + 1, &rel, 4);
  VirtualProtect(target, 5, old, &old);
  FlushInstructionCache(GetCurrentProcess(), target, 5);
  return 0;
}

// ------------------------------------------------------------------ MD5 (public-domain compact impl) for local ticket derive
// The RV ticket "crypto" is MD5-only (NOTES 28z), so a deterministic local MD5 over a shared secret makes both
// peers derive the same GameSessionKey. Fill inputs to match the InitiateLogin bundle (NOTES 28aa) at test time.
typedef struct { uint32_t s[4]; uint64_t n; uint8_t buf[64]; size_t have; } MD5;
static const uint32_t MD5_K[64] = {
  0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
  0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
  0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
  0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
  0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
  0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
  0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
  0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391 };
static const uint8_t MD5_R[64] = {
  7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
  4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21 };
static inline uint32_t rol(uint32_t x,int c){return (x<<c)|(x>>(32-c));}
static void md5_block(MD5*m,const uint8_t*p){
  uint32_t w[16]; for(int i=0;i<16;i++) w[i]=(uint32_t)p[i*4]|(p[i*4+1]<<8)|(p[i*4+2]<<16)|((uint32_t)p[i*4+3]<<24);
  uint32_t a=m->s[0],b=m->s[1],c=m->s[2],d=m->s[3];
  for(int i=0;i<64;i++){uint32_t f;int g;
    if(i<16){f=(b&c)|(~b&d);g=i;} else if(i<32){f=(d&b)|(~d&c);g=(5*i+1)%16;}
    else if(i<48){f=b^c^d;g=(3*i+5)%16;} else {f=c^(b|~d);g=(7*i)%16;}
    f=f+a+MD5_K[i]+w[g]; a=d; d=c; c=b; b=b+rol(f,MD5_R[i]);}
  m->s[0]+=a;m->s[1]+=b;m->s[2]+=c;m->s[3]+=d;
}
static void md5_init(MD5*m){m->s[0]=0x67452301;m->s[1]=0xefcdab89;m->s[2]=0x98badcfe;m->s[3]=0x10325476;m->n=0;m->have=0;}
static void md5_upd(MD5*m,const void*data,size_t len){const uint8_t*p=(const uint8_t*)data;m->n+=len;
  while(len){size_t take=64-m->have;if(take>len)take=len;memcpy(m->buf+m->have,p,take);m->have+=take;p+=take;len-=take;
    if(m->have==64){md5_block(m,m->buf);m->have=0;}}}
static void md5_final(MD5*m,uint8_t out[16]){uint64_t bits=m->n*8;uint8_t pad=0x80;md5_upd(m,&pad,1);
  while(m->have!=56){uint8_t z=0;md5_upd(m,&z,1);} md5_upd(m,(const uint8_t*)&bits,8);
  for(int i=0;i<4;i++){out[i]=m->s[0]>>(8*i);out[4+i]=m->s[1]>>(8*i);out[8+i]=m->s[2]>>(8*i);out[12+i]=m->s[3]>>(8*i);}}
static void derive_local_ticket(uint8_t out[16]) { MD5 m; md5_init(&m); md5_upd(&m, g_cfg.shared_secret, strlen(g_cfg.shared_secret));
  // TODO(NOTES 28aa): also mix the shared session identity (session id / playlist id / account) so the ticket
  // matches the InitiateLogin bundle both peers see; secret alone is only a stand-in.
  md5_final(&m, out); }

// ------------------------------------------------------------------ hook bodies (route B)
// Each signature must MATCH how the game calls the original (thiscall/fastcall) - the stubs below use the
// observed arg counts; confirm stack balance at runtime (TODO).

// Pretend "online enabled" so the gate precondition passes.
static int __cdecl hk_online_flag(void) { LOGF("shim: online_flag -> 1\n"); return 1; }

// Produce a LAN address instead of resolving RV SandboxUrl.
static int hk_rv_sandboxurl(void* out /*FUN_112492a0 local_54 out*/) {
  LOGF("shim: rv_sandboxurl -> LAN\n");
  // TODO(NOTES 28ad): write g_cfg.lan_ip into the exact struct layout the gate reads (FUN_11232e20/FUN_11232ef0).
  (void)out;
  return 1;
}

// Co-op session gate: succeed + hand out the LAN address+port (instead of RV).
static int hk_gate(void* out1, void* out2) {
  LOGF("shim: gate -> success (LAN %u.%u.%u.%u:%u)\n", (g_cfg.lan_ip>>24)&255,(g_cfg.lan_ip>>16)&255,(g_cfg.lan_ip>>8)&255,g_cfg.lan_ip&255, g_cfg.lan_port);
  // The gate hands the game a sockaddr_in to dial (NOTES 28af). Fill family/port/addr in NETWORK byte order.
  // out1 is the address struct slot (FUN_11232e20 shape); out2 is the port (FUN_11232ef0 shape).
  // TODO(NOTES 28af): confirm which pointer is the sockaddr and which is the scalar port; then:
  //   ((unsigned short*)sa)[0] = htons(2 /*AF_INET*/); ((unsigned short*)sa)[1] = htons(g_cfg.lan_port);
  //   *(unsigned*)(sa+4) = htonl(g_cfg.lan_ip);
  (void)out1; (void)out2;
  return 1;
}

// "RendezVous connected" (FUN_11237670): the ticket AcquireTicket + message builder require it -> force true.
static int __cdecl hk_rv_connected(void* a) { (void)a; return 1; }

// Host path (NOTES 28ag): do NOT hand-build a CFCX op - the op registry is a C++ name factory with an async op lifecycle.
// Instead neutralize the RV auth step (it only allocates SubAccountClient + issues an RV service vtable call) so the game's own
// storm::echo create op (already built from a NetEngineType=LAN param) forms the session over LAN. Returns success, issues no RV call.
static int hk_host(void* a,void* b,void* c,void* d,void* e,int f,void* g) {
  LOGF("shim: host -> short-circuit RV auth (SubAccountClient) so native LAN storm::echo op proceeds\n");
  (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;
  // TODO(NOTES 28ag): confirm the caller ignores the RV async handle; force NetEngineType=0 (LAN) at the op-param builder
  //   (FUN_1061e581 field id 0x3c / FUN_1092d256 param[0xe]) so the native create op is the LAN flavor.
  return 1;
}

// Join path (NOTES 28ag): same idea as hk_host - short-circuit the UbiAuthentication RV auth to success (no RV service call);
// the native storm::echo join op (NetEngineType=LAN, SearchIdAddress=our sockaddr) forms the session.
static int hk_join(void* a,void* b,void* c,void* d,void* e,void* f,void* g,void* h,void* i) {
  LOGF("shim: join -> short-circuit RV auth (UbiAuthenticationClient) so native LAN storm::echo op proceeds\n");
  (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;
  // TODO(NOTES 28ag): the join precondition FUN_1127a0d0 needs rv_connected=1 (hooked); ensure SearchIdAddress in the
  //   op-param block points at g_cfg.lan_ip:lan_port (sockaddr_in) so the LAN storm::echo join op dials us.
  return 1;
}

// Ticket validate: accept any (MD5 self-consistent) ticket.
static int hk_ticket_validate(void* a,void* b,uint32_t c,void* d,void* e,void* f) {
  uint8_t t[16]; derive_local_ticket(t);
  LOGF("shim: ticket_validate -> accept (local md5 %02x%02x)\n", t[0], t[1]);
  (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;
  return 1; // TODO: match the original return shape (thiscall via FUN_112547c0)
}
// Skip the network AcquireTicket; ProcessResponse will be fed the local ticket.
static void* hk_ticket_prepare(void* a,void* b,void* c,void* d,void* e,void* f) {
  LOGF("shim: ticket_prepare -> skip network\n");
  (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;
  return 0; // TODO: still build the job object so the response handler runs with a LOCAL ticket
}

// ------------------------------------------------------------------ install
// ---- call-site provider-factory probe (NOTES 28ar) ----
// Hook the 'call edx' (vtable[+0x14] provider dispatch) inside host_handler. The trampoline logs the provider
// object + vtable, performs the ORIGINAL call edx with the exact same registers/stack, replays the displaced
// 'mov ecx,[ebp-0x128]', then resumes at base+RVA_provider_factory_back. All sites assume module md5 46bcc097.
static uintptr_t g_provider_jmpback = 0;
static void __cdecl log_provider_factory(void* thisp, void* method) {
  __try {
    uintptr_t vtbl = *(volatile uintptr_t*)thisp;
    uintptr_t slot = *(volatile uintptr_t*)(vtbl + 0x14);
    LOGF("shim: provider_factory: this=%p vtable=%p slot+0x14=%p (edx=%p) mod=0x%08x",
         thisp, (void*)vtbl, (void*)slot, method, (uint32_t)g_mod);
    LOGF("shim:   vtbl_rva=0x%08x slot_rva=0x%08x", (uint32_t)(vtbl - g_mod), (uint32_t)(slot - g_mod));
    LOGF("shim:   vtable[0..7]= %p %p %p %p %p %p %p %p",
      *(void**)(vtbl), *(void**)(vtbl+0x04), *(void**)(vtbl+0x08), *(void**)(vtbl+0x0C),
      *(void**)(vtbl+0x10), *(void**)(vtbl+0x14), *(void**)(vtbl+0x18), *(void**)(vtbl+0x1C));
  } __except (EXCEPTION_EXECUTE_HANDLER) { LOGF("shim: provider_factory read fault (caught)"); }
}
__declspec(naked) static void hk_provider_factory(void) {
  __asm {
    pushad
    push edx
    push esi
    call log_provider_factory
    add  esp, 8
    popad
    call edx                                // original vtable[+0x14] dispatch (ecx=this, edx=method, stack intact)
    mov  ecx, dword ptr [ebp - 0x128]       // displaced instruction originally at the hook site
    jmp  dword ptr [g_provider_jmpback]      // resume at base + RVA_provider_factory_back (ASLR-safe)
  }
}
static int patch_provider_factory(void) {
  void* site = (void*)VA(RVA_provider_factory_site);
  uint8_t* t = (uint8_t*)site;
  if (!(t[0] == 0xFF && t[1] == 0xD2)) { LOGF("shim: provider_factory site bytes mismatch @%p (%02x %02x) - skip", site, t[0], t[1]); return -1; }
  if (!(t[2] == 0x8B && t[3] == 0x8D)) { LOGF("shim: provider_factory displaced bytes unexpected (%02x %02x) - skip", t[2], t[3]); return -1; }
  g_provider_jmpback = VA(RVA_provider_factory_back);
  DWORD old; VirtualProtect(site, 5, PAGE_EXECUTE_READWRITE, &old);
  uint8_t jmp = 0xE9; int32_t rel = (int32_t)((uint8_t*)hk_provider_factory - ((uint8_t*)site + 5));
  memcpy(t, &jmp, 1); memcpy(t + 1, &rel, 4);
  VirtualProtect(site, 5, old, &old);
  FlushInstructionCache(GetCurrentProcess(), site, 5);
  LOGF("shim: patch provider_factory @0x%08x -> back 0x%08x", (uint32_t)(uintptr_t)site, (uint32_t)g_provider_jmpback);
  return 0;
}

// ------------------------------------------------------------------ path A'' online-session-manager vtable tracer (NOTES 28ax)
// The gate [0x11e42c90] is the manager. Offline it is null, so the survey must run during a NORMAL online login
// (do NOT set FC3COOP_FAKE_ONLINE). Once the manager is stable, each vtable slot that is a code pointer is replaced
// by a generated stub: pushad; push i; call mgr_log_slot; add esp,4; popad; jmp [g_mgr_orig[i]] (tail-call, stack
// neutral, thiscall ecx preserved). We log the slot byte-offset + hit count -> the set of methods actually invoked
// is the minimal interface a fake manager must implement.
#define MAX_MGR_SLOTS 96
static volatile LONG g_mgr_hits[MAX_MGR_SLOTS];
static uintptr_t g_mgr_orig[MAX_MGR_SLOTS];
static uintptr_t g_imgsize = 0;
static unsigned char* g_stub_pool = 0;

static void __cdecl mgr_log_slot(uintptr_t i) {
  if (i >= MAX_MGR_SLOTS) return;
  LONG c = InterlockedIncrement(&g_mgr_hits[i]);
  if (c <= 3) {
    logLock();
    LOGF("shim: MGR slot[+0x%02x] hit#%ld (method_rva=0x%08x)", (unsigned)(i*4), (long)c,
         g_mgr_orig[i] ? (uint32_t)(g_mgr_orig[i] - g_mod) : 0u);
    logUnlock();
  }
}

static DWORD WINAPI mgr_tracer_thread(LPVOID p) {
  (void)p;
  int seen = 0;
  for (int iter = 0; iter < 3600; ++iter) {        // poll up to ~30 min; only memory reads + Sleep before mgr found
    uintptr_t mgr = *(volatile uintptr_t*)VA(RVA_online_mgr_global);
    uintptr_t vptr = 0;
    if (mgr) { __try { vptr = *(volatile uintptr_t*)mgr; } __except (EXCEPTION_EXECUTE_HANDLER) { vptr = 0; } }
    if (mgr && vptr) { if (++seen >= 2) break; } else seen = 0;
    Sleep(500);
  }
  uintptr_t mgr = *(volatile uintptr_t*)VA(RVA_online_mgr_global);
  if (!mgr) { logLock(); LOGF("shim: MGR tracer: manager never created (offline/no login) - nothing surveyed"); logUnlock(); return 0; }
  uintptr_t vptr = 0;
  __try { vptr = *(volatile uintptr_t*)mgr; } __except (EXCEPTION_EXECUTE_HANDLER) {}
  if (!vptr) { LOGF("shim: MGR tracer: manager null vptr"); return 0; }
  logLock(); LOGF("shim: MGR tracer: mgr=%p vtable=%p (vtable_rva=0x%08x) imgsize=0x%x", (void*)mgr, (void*)vptr, (uint32_t)(vptr - g_mod), (unsigned)g_imgsize); logUnlock();
  g_stub_pool = (unsigned char*)VirtualAlloc(0, MAX_MGR_SLOTS * 24, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  if (!g_stub_pool) { LOGF("shim: MGR tracer: VirtualAlloc failed"); return 0; }
  DWORD old; VirtualProtect((void*)vptr, MAX_MGR_SLOTS * 4, PAGE_READWRITE, &old);
  int n = 0;
  for (int i = 0; i < MAX_MGR_SLOTS; ++i) {
    uintptr_t fn = 0;
    __try { fn = *(volatile uintptr_t*)(vptr + (uintptr_t)i * 4); } __except (EXCEPTION_EXECUTE_HANDLER) { break; }
    if (fn < g_mod || fn >= g_mod + g_imgsize) break;   // not a code pointer -> treat as end of this vtable
    g_mgr_orig[i] = fn;
    unsigned char* s = g_stub_pool + (size_t)i * 24; int off = 0;
    s[off++] = 0x60;                                     // pushad
    s[off++] = 0x6A; s[off++] = (unsigned char)i;        // push imm8 i
    s[off++] = 0xE8;                                     // call rel32 -> mgr_log_slot
    int32_t rel = (int32_t)((uintptr_t)&mgr_log_slot - (uintptr_t)(s + off + 4));
    memcpy(s + off, &rel, 4); off += 4;
    s[off++] = 0x83; s[off++] = 0xC4; s[off++] = 0x04;   // add esp,4
    s[off++] = 0x61;                                     // popad
    s[off++] = 0xFF; s[off++] = 0x25;                    // jmp dword ptr [&g_mgr_orig[i]]
    { uintptr_t pp = (uintptr_t)&g_mgr_orig[i]; memcpy(s + off, &pp, 4); off += 4; }
    FlushInstructionCache(GetCurrentProcess(), s, off);
    *(volatile uintptr_t*)(vptr + (uintptr_t)i * 4) = (uintptr_t)s;
    n++;
  }
  VirtualProtect((void*)vptr, MAX_MGR_SLOTS * 4, old, &old);
  logLock(); LOGF("shim: MGR tracer: patched %d slots; now play online a bit + try Co-op Host, then read the log", n); logUnlock();
  return 0;
}

// ------------------------------------------------------------------ path A'' fake online-session-manager (NOTES 28ay/28az)
// Offline [0x11e42c90] is null, which is the ONLY thing that makes the co-op Host action bail early (28ay). Install a
// zeroed object whose vptr points at the real manager class vtable (RVA 0x1527870) so stack discipline of the 16 hot
// slots is preserved (cheap getters read zeroed fields). A watchdog re-forces the global ONLY when the game has left
// it null, so this never clobbers a real online manager (safe to run while online, it only helps the offline case).
static DWORD WINAPI fake_mgr_thread(LPVOID p) {
  (void)p;
  uintptr_t g = VA(RVA_online_mgr_global);
  for (int i = 0; i < 24000; ++i) {                 // ~60 min watchdog
    if (g_fake_force || *(volatile uintptr_t*)g == 0) *(volatile uintptr_t*)g = g_fake_obj;
    Sleep(150);
  }
  return 0;
}
// FC3COOP_FAKE_MGR=3 (NOTES 28cn): call the game's own manager creator so FCX builds a REAL manager.
static DWORD WINAPI mgr_forcer_thread(LPVOID p) {
  (void)p;
  Sleep(20000);                       // let the game finish booting to the menu first
  uintptr_t before = *(volatile uintptr_t*)VA(RVA_online_mgr_global);
  logLock(); LOGF("shim: MGR_FORCER calling game creator FUN_10420a77 (global before=0x%08x)", (uint32_t)before); logUnlock();
  __try {
    typedef void (__cdecl *creator_t)(void);
    ((creator_t)VA(RVA_manager_creator))();
  } __except (EXCEPTION_EXECUTE_HANDLER) { logLock(); LOGF("shim: MGR_FORCER creator call EXC"); logUnlock(); }
  uintptr_t after = *(volatile uintptr_t*)VA(RVA_online_mgr_global);
  logLock(); LOGF("shim: MGR_FORCER global after=0x%08x (%s)", (uint32_t)after, after ? "REAL manager built" : "still NULL - factory needs live RV/online"); logUnlock();
  return 0;
}
static void install_fake_mgr(void) {
  if (!g_fake_obj) {
    void* obj = VirtualAlloc(0, 0x600, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!obj) { LOGF("shim: FAKE_MGR VirtualAlloc failed"); return; }
    g_fake_obj = (uintptr_t)obj;
    *(volatile uintptr_t*)g_fake_obj = VA(RVA_mgr_vtable);
  }
  *(volatile uintptr_t*)VA(RVA_online_mgr_global) = g_fake_obj;
  HANDLE h = CreateThread(0, 0, fake_mgr_thread, 0, 0, 0);
  LOGF("shim: FAKE_MGR obj=%p vptr=vtable_rva 0x%x forced into [0x11e42c90]; watchdog h=%p", (void*)g_fake_obj,
       (unsigned)(VA(RVA_mgr_vtable) - g_mod), (void*)h);
}

// ------------------------------------------------------------------ RV guest-login probe (NOTES 28bc) - A-vs-D discriminator
// The co-op join op Execute (FUN_11279d80) either posts a pre-check error (0x80030064/0x80030065, RV not ready) or
// actually sends the RV guest login FUN_11246b90 (dummy creds UbiDummyPwd) and awaits callback FUN_11253890 (status
// via FUN_11210df0; <0 -> 0x8001000b). Logging SEND + CB tells us whether the guest login round-trips at all.
// Both targets need a 6-byte-aligned entry displacement (55 8B EC ..83 EC 18 / ..8B 45 14); patch_entry_n fills the
// extra byte with int3 and the trampoline re-runs the displaced 6 bytes then jumps back. Detour is pushad-preserved
// so the thiscall `ecx` reaches the original intact.
#define RVA_rvlogin_send 0x01246b90
#define RVA_rvlogin_cb   0x01253890
#define RVA_rvlogin_status_site   0x012538bc // 'E8 xx' call FUN_11210df0 inside the callback (eax=status)
#define RVA_rvlogin_status_resume 0x012538c1 // 'test eax,eax' right after that call
#define RVA_rvlogin_status_callee 0x01210df0 // login status getter
static const char g_tag_send[] = "RVLOGIN-SEND";
static const char g_tag_cb[]   = "RVLOGIN-CB";
static const char g_tag_status[] = "RVLOGIN-STATUS";
static uintptr_t g_status_callee = 0, g_status_resume = 0;
static void __cdecl log_login_status(int status, const char* tag) {
  logLock(); LOGF("shim: %s status=0x%08x %s", tag, (uint32_t)status, status < 0 ? "REJECT" : "ACCEPT"); logUnlock();
}
__declspec(naked) static void hk_rvlogin_status(void) {
  __asm {
    call dword ptr [g_status_callee]
    pushad
    push eax
    mov  eax, OFFSET g_tag_status
    push eax
    call log_login_status
    add  esp, 8
    popad
    jmp  dword ptr [g_status_resume]
  }
}
static uintptr_t g_send_tr = 0, g_cb_tr = 0;
static void __cdecl log_login_ev(const char* tag, void* selfp) {
  logLock(); LOGF("shim: %s this=%p mod=0x%08x", tag, selfp, (uint32_t)g_mod); logUnlock();
}
__declspec(naked) static void hk_rvlogin_send(void) {
  __asm {
    pushad
    push ecx
    mov  eax, OFFSET g_tag_send
    push eax
    call log_login_ev
    add  esp, 8
    popad
    jmp  dword ptr [g_send_tr]
  }
}
__declspec(naked) static void hk_rvlogin_cb(void) {
  __asm {
    pushad
    push ecx
    mov  eax, OFFSET g_tag_cb
    push eax
    call log_login_ev
    add  esp, 8
    popad
    jmp  dword ptr [g_cb_tr]
  }
}
static int patch_entry_n(uintptr_t target, int n, void* detour, uintptr_t* tr_out) {
  uint8_t saved[16]; if (n > (int)sizeof(saved)) return -1;
  memcpy(saved, (void*)target, n);
  uint8_t* tr = (uint8_t*)VirtualAlloc(0, n + 16, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  if (!tr) return -1;
  memcpy(tr, saved, n);
  tr[n] = 0xE9; int32_t rel = (int32_t)((target + n) - ((uintptr_t)(tr + n + 5)));
  memcpy(tr + n + 1, &rel, 4);
  *tr_out = (uintptr_t)tr;
  DWORD old; VirtualProtect((void*)target, n, PAGE_EXECUTE_READWRITE, &old);
  uint8_t* t = (uint8_t*)target; uint8_t jmp = 0xE9; int32_t d = (int32_t)((uintptr_t)detour - (target + 5));
  memcpy(t, &jmp, 1); memcpy(t + 1, &d, 4);
  for (int i = 5; i < n; ++i) t[i] = 0xCC;
  VirtualProtect((void*)target, n, old, &old);
  FlushInstructionCache(GetCurrentProcess(), (void*)target, n);
  return 0;
}
static void install_login_probe(void) {
  int a = patch_entry_n(VA(RVA_rvlogin_send), 6, (void*)hk_rvlogin_send, &g_send_tr);
  int b = patch_entry_n(VA(RVA_rvlogin_cb), 6, (void*)hk_rvlogin_cb, &g_cb_tr);
  g_status_callee = VA(RVA_rvlogin_status_callee);
  g_status_resume = VA(RVA_rvlogin_status_resume);
  uintptr_t site = VA(RVA_rvlogin_status_site);
  int c = -1;
  if (*(volatile uint8_t*)site == 0xE8) {
    DWORD old; VirtualProtect((void*)site, 5, PAGE_EXECUTE_READWRITE, &old);
    uint8_t* t = (uint8_t*)site; uint8_t jmp = 0xE9; int32_t d = (int32_t)((uintptr_t)hk_rvlogin_status - (site + 5));
    t[0] = jmp; memcpy(t + 1, &d, 4);
    VirtualProtect((void*)site, 5, old, &old); FlushInstructionCache(GetCurrentProcess(), (void*)site, 5);
    c = 0;
  } else { LOGF("shim: PROBE_LOGIN status site not E8 (got %02x) - skip status capture", *(volatile uint8_t*)site); }
  LOGF("shim: PROBE_LOGIN installed send=%d cb=%d status=%d no_ticket=%d (Co-op -> Host; watch RVLOGIN-SEND/CB/STATUS)", a, b, c, g_no_ticket);
}

// ---- NOTES 28dc: OBSERVE the FCX online-login event dispatcher FUN_107db689(this, handle, code) ----
// thiscall: ecx=this, [esp+4]=handle, [esp+8]=code. Observer-only: log the code, tail-call the original
// via a 6-byte trampoline (same proven pattern as hk_rvlogin_send). NO behaviour change.
static const char g_tag_loginev[] = "LOGIN-EV";
static uintptr_t g_login_tr = 0;
static void __cdecl log_login_ev2(uint32_t selfp, uint32_t handle, uint32_t code) {
  const char* nm = code==6 ? "LoggingIn" : code==9 ? "LoggingInSucceeded" : code==0x11 ? "LoggingInFailed"
    : code==0xf ? "LoggingOut" : code==0x10 ? "LoggingOutSucceeded" : code==3 ? "code3" : code==5 ? "code5" : "";
  logLock(); LOGF("shim: LOGIN-EV this=0x%08x handle=0x%08x code=%u%s%s", selfp, handle, code, nm?" (":"", nm?nm:""); logUnlock();
}
__declspec(naked) static void hk_login_event(void) {
  __asm {
    pushad
    lea  ebx, [esp+32]        // ebx = entry esp ([esp]=retaddr,[esp+4]=handle,[esp+8]=code)
    push dword ptr [ebx+8]    // code
    push dword ptr [ebx+4]    // handle
    push ecx                  // this
    call log_login_ev2
    add  esp, 12
    popad
    jmp  dword ptr [g_login_tr]
  }
}
static void install_login_event_watch(void) {
  uintptr_t f = VA(RVA_login_event);
  uint8_t* p = (uint8_t*)f;
  __try {
    if (p[0]==0x55 && p[1]==0x8B && p[2]==0xEC) {
      int r = patch_entry_n(f, 6, (void*)hk_login_event, &g_login_tr);
      LOGF("shim: LOGIN_EVENT watch on FUN_107db689 r=%d (watch LOGIN-EV code=6 LoggingIn / 9 Succeeded / 0x11 Failed)", r);
    } else {
      LOGF("shim: LOGIN_EVENT prologue at 0x%x not 55 8B EC (got %02x %02x %02x) - skip", (uint32_t)f, p[0], p[1], p[2]);
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) { LOGF("shim: LOGIN_EVENT read fault at 0x%x - skip", (uint32_t)f); }
}

// ---- NOTES 28df: BLOCK the LogoutOperation FUN_107dc7b7 so the login (which now reaches 6/9) does not
// immediately log out. Skip = do not run the original (log + thiscall ret with 1 stack arg). Test whether a
// sticking login lets the co-op front-end proceed / the client dial RV. ----
static void log_logout_block(void){ logLock(); LOGF("shim: LOGOUT-BLOCK suppressed FUN_107dc7b7 (LogoutOperation)"); logUnlock(); }
__declspec(naked) static void hk_logout_block(void) {
  __asm {
    pushad
    call log_logout_block
    popad
    ret 4            // thiscall(this=ecx, param_2 on stack): callee pops 1 arg
  }
}
static void install_logout_block(void) {
  uintptr_t site = VA(RVA_logout_op);
  __try {
    DWORD old; VirtualProtect((void*)site, 5, PAGE_EXECUTE_READWRITE, &old);
    uint8_t* t = (uint8_t*)site; t[0]=0xE9;
    int32_t rel = (int32_t)((uintptr_t)hk_logout_block - (site + 5)); memcpy(t+1, &rel, 4);
    VirtualProtect((void*)site, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), (void*)site, 5);
    LOGF("shim: LOGOUT-BLOCK hook installed on FUN_107dc7b7 (watch LOGOUT-BLOCK + LOGIN-EV should stop showing code 15)");
  } __except (EXCEPTION_EXECUTE_HANDLER) { LOGF("shim: LOGOUT-BLOCK install fault at 0x%x", (uint32_t)site); }
}

// ---- NOTES 28dh: DNS REDIRECT via IAT patch (no C:\\hosts edit). FC3_d3d11.dll fetches its OnlineConfig with a raw
// winsock HTTP GET to http://onlineconfigservice.ubi.com/OnlineConfigService.svc/GetOnlineConfig and resolves the
// host through the imported WS2_32!getaddrinfo. We overwrite the DLL's getaddrinfo IAT slot so that host resolves to
// 127.0.0.1 -> a local config_server.py on :80 serves a crafted OnlineConfig whose SandboxUrl points the RV client
// at our local rv_server. Forwarding to the REAL getaddrinfo with "127.0.0.1" keeps allocation/free semantics valid.
static void* find_iat_slot(uintptr_t base, const char* dllName, const char* funcName) {
  __try {
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((unsigned char*)base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;
    DWORD impRva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (!impRva) return 0;
    PIMAGE_IMPORT_DESCRIPTOR imp = (PIMAGE_IMPORT_DESCRIPTOR)(base + impRva);
    for (; imp->Name; imp++) {
      const char* dn = (const char*)(base + imp->Name);
      if (_stricmp(dn, dllName) != 0) continue;
      PIMAGE_THUNK_DATA orig = (PIMAGE_THUNK_DATA)(base + (imp->OriginalFirstThunk ? imp->OriginalFirstThunk : imp->FirstThunk));
      PIMAGE_THUNK_DATA iat  = (PIMAGE_THUNK_DATA)(base + imp->FirstThunk);
      for (; orig->u1.AddressOfData; orig++, iat++) {
        if (IMAGE_SNAP_BY_ORDINAL(orig->u1.Ordinal)) continue;
        PIMAGE_IMPORT_BY_NAME ibn = (PIMAGE_IMPORT_BY_NAME)(base + orig->u1.AddressOfData);
        if (_stricmp((const char*)ibn->Name, funcName) == 0) return (void*)&iat->u1.Function;
      }
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
  return 0;
}
typedef int (WINAPI *getaddrinfo_t)(const char*, const char*, void*, void**);
static getaddrinfo_t g_real_getaddrinfo = 0;
static int WINAPI hk_getaddrinfo(const char* node, const char* service, void* hints, void** res) {
  if (node) { logLock(); LOGF("shim: DNS-LOOKUP '%s' svc=%s", node, service?service:"-"); logUnlock(); }
  if (node && (strstr(node, "onlineconfigservice") || _stricmp(node, "config.ubi.com") == 0)) {
    logLock(); LOGF("shim: DNS-REDIRECT '%s' -> 127.0.0.1", node); logUnlock();
    return g_real_getaddrinfo("127.0.0.1", service, hints, res);
  }
  return g_real_getaddrinfo(node, service, hints, res);
}
static void install_dns_redirect(void) {
  void* slot = find_iat_slot(g_mod, "WS2_32.dll", "getaddrinfo");
  if (!slot) { LOGF("shim: DNS-REDIRECT: WS2_32!getaddrinfo IAT slot NOT found in FC3_d3d11.dll"); return; }
  HMODULE w = LoadLibraryA("ws2_32.dll");
  g_real_getaddrinfo = (getaddrinfo_t)GetProcAddress(w ? w : GetModuleHandleA("ws2_32.dll"), "getaddrinfo");
  if (!g_real_getaddrinfo) { LOGF("shim: DNS-REDIRECT: could not resolve real getaddrinfo"); return; }
  __try {
    DWORD old; if (VirtualProtect(slot, 4, PAGE_READWRITE, &old)) {
      *(volatile void**)slot = (void*)hk_getaddrinfo;
      VirtualProtect(slot, 4, old, &old);
      LOGF("shim: DNS-REDIRECT installed on getaddrinfo IAT slot %p (onlineconfigservice.ubi.com -> 127.0.0.1)", slot);
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) { LOGF("shim: DNS-REDIRECT install fault"); }
}

// ---- NOTES 28dn: RV bring-up PROBE. OBSERVER-only trampolines (log 'was called', then tail-call the original via a
// 6-byte trampoline) on the RV connect path, to learn where FCX's co-op RV bring-up stops. All three targets have the
// 55 8b ec prologue (verified in scratch rv_entry.txt) so a 6-byte entry trampoline is safe. NO behaviour change.
static uintptr_t g_rv_cas_tr = 0, g_rv_st_tr = 0, g_rv_ws_tr = 0;
static void rv_log_cas(void){ logLock(); LOGF("shim: RV-PROBE ConnectToAuthenticationService (FUN_11252f70) CALLED"); logUnlock(); }
static void rv_log_st(void){ logLock(); LOGF("shim: RV-PROBE RV sendto (FUN_11218510) CALLED -> RV packet being sent"); logUnlock(); }
static void rv_log_ws(void){ logLock(); LOGF("shim: RV-PROBE RV WSASendTo (FUN_11214630) CALLED -> RV packet being sent"); logUnlock(); }
__declspec(naked) static void hk_rv_cas(void){ __asm {
  pushad
  call rv_log_cas
  popad
  jmp dword ptr [g_rv_cas_tr]
} }
__declspec(naked) static void hk_rv_st(void){ __asm {
  pushad
  call rv_log_st
  popad
  jmp dword ptr [g_rv_st_tr]
} }
__declspec(naked) static void hk_rv_ws(void){ __asm {
  pushad
  call rv_log_ws
  popad
  jmp dword ptr [g_rv_ws_tr]
} }
static int install_rv_observer(uintptr_t rva, void* detour, uintptr_t* tr, const char* nm) {
  uintptr_t f = VA(rva); uint8_t* p = (uint8_t*)f;
  __try {
    if (p[0]==0x55 && p[1]==0x8B && p[2]==0xEC) return patch_entry_n(f, 6, detour, tr);
    LOGF("shim: RV-PROBE %s prologue at 0x%x not 55 8b EC (got %02x %02x %02x) - skip", nm, (uint32_t)f, p[0], p[1], p[2]);
  } __except (EXCEPTION_EXECUTE_HANDLER) { LOGF("shim: RV-PROBE %s read fault at 0x%x", nm, (uint32_t)f); }
  return -1;
}
static void install_rv_probe(void) {
  install_rv_observer(RVA_connect_auth_svc, (void*)hk_rv_cas, &g_rv_cas_tr, "ConnectToAuthSvc");
  install_rv_observer(RVA_rv_sendto,        (void*)hk_rv_st,  &g_rv_st_tr,     "rv_sendto");
  install_rv_observer(RVA_rv_wsasendto,     (void*)hk_rv_ws,  &g_rv_ws_tr,     "rv_wsasendto");
  LOGF("shim: RV-PROBE observers installed (watch RV-PROBE lines: none => RV never reached; CAS only => RV connect gated by precondition; sendto too => RV actually dials, point DNS-redirect at rv_server)");
}

static void install_hooks(void) {
  struct { uintptr_t rva; void* detour; const char* name; } tbl[] = {
    { RVA_online_flag,     (void*)hk_online_flag,     "online_flag" },
    { RVA_rv_sandboxurl,   (void*)hk_rv_sandboxurl,   "rv_sandboxurl" },
    { RVA_gate,            (void*)hk_gate,            "gate" },
    { RVA_host_handler,    (void*)hk_host,            "host_handler" },
    { RVA_join_handler,    (void*)hk_join,            "join_handler" },
    { RVA_ticket_validate, (void*)hk_ticket_validate, "ticket_validate" },
    { RVA_ticket_prepare,  (void*)hk_ticket_prepare,  "ticket_prepare" },
    { RVA_rv_connected,    (void*)hk_rv_connected,     "rv_connected" },
  };
  for (unsigned i = 0; i < sizeof(tbl)/sizeof(tbl[0]); ++i) {
    if (g_hook_provider && tbl[i].rva == RVA_host_handler) { LOGF("shim: SKIP host_handler short-circuit (provider probe: run real host)\n"); continue; }
    if (g_probe_login && (tbl[i].rva == RVA_join_handler || tbl[i].rva == RVA_host_handler)) { LOGF("shim: SKIP %s short-circuit (login probe: run real co-op op)\n", tbl[i].name); continue; }
    if (g_no_ticket && (tbl[i].rva == RVA_ticket_validate || tbl[i].rva == RVA_ticket_prepare)) { LOGF("shim: SKIP %s (NO_TICKET isolation)\n", tbl[i].name); continue; }
    void* tgt = (void*)VA((uint32_t)tbl[i].rva);
    LOGF("shim: patch %s @0x%08x\n", tbl[i].name, (uint32_t)(uintptr_t)tgt);
    patch_5(tgt, tbl[i].detour); // TODO: per-hook saved bytes + trampoline; currently shares g_hooks[0].saved
  }
  if (g_hook_provider) patch_provider_factory();
}

// ------------------------------------------------------------------ init
static void init(void) {
  logOpen();
  HMODULE m = GetModuleHandleA("FC3_d3d11.dll");
  g_mod = (uintptr_t)(m ? m : (HMODULE)FC3_BASE_DEFAULT);
  __try {
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)g_mod;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((unsigned char*)g_mod + dos->e_lfanew);
    g_imgsize = nt->OptionalHeader.SizeOfImage;
  } __except (EXCEPTION_EXECUTE_HANDLER) { g_imgsize = 0x2000000; }
  load_config();
  LOGF("shim: FC3_d3d11 base=0x%08x enable=%d\n", (uint32_t)g_mod, g_cfg.enable);
  if (!g_cfg.enable) { LOGF("shim: inert (set FC3COOP_SHIM=1 to enable)\n"); return; }
  // Stage0b (NOTES 28ci): minimal, collateral-free experiment. The co-op Host action FUN_10443663 returns the
  // 'service unavailable' error code 4 iff DAT_11e42c90 (online session-manager) == 0. Forcing that global to a
  // valid object is the gate; it does NOT read the online flag, so we skip the online_flag write AND all code
  // patches (which is what woke netconfig FUN_1126dc60 and crashed in 28cc/28cg). Only install_fake_mgr() runs.
  if (g_fake_mgr_only || g_mgr_call) {
    // Clean mode (NOTES 28ci/28cn): no online_flag write and no code hooks (avoids the 28cg netconfig crash).
    if (g_mgr_call) { HANDLE h = CreateThread(0,0,mgr_forcer_thread,0,0,0); LOGF("shim: FAKE_MGR=3 active - will CALL the game's creator FUN_10420a77 in ~20s; no online flag, no hooks. h=%p", (void*)h); }
    else { install_fake_mgr(); LOGF("shim: FAKE_MGR_ONLY active - only [0x11e42c90] forced; no online flag, no hooks. Try Co-op -> Host now.\n"); }
    return;
  }
  if (g_dns_redirect) install_dns_redirect();   // independent lever: works in any mode (NOTES 28dh)
  if (g_rv_probe) install_rv_probe();           // independent observer: works in any mode (NOTES 28dn)
  if (g_watch_login) { install_login_event_watch(); if (g_block_logout) install_logout_block(); LOGF("shim: WATCH_LOGIN clean mode - FUN_107db689 observer%s; no online flag, no other hooks", g_block_logout?" + LOGOUT-BLOCK":""); return; }
  // TODO: verify module md5 == 46bcc097 before patching (else RVAs are wrong -> crash).
  // Cheap precondition: force the online-enable global (FUN_11245090 returns DAT_11f07e48) to 1.
  *(volatile unsigned char*)VA(RVA_online_flag_global) = 1;
  LOGF("shim: online global DAT_11f07e48=1\n");
  install_hooks();
  LOGF("shim: hooks installed (SCAFFOLD - untested)\n");
  if (g_trace_mgr) {
    HANDLE h = CreateThread(0, 0, mgr_tracer_thread, 0, 0, 0);
    LOGF("shim: MGR tracer thread started h=%p (set FC3COOP_FAKE_ONLINE OFF; just play online + try Co-op Host)", (void*)h);
  }
  if (g_fake_mgr) { if (g_mgr_call) { HANDLE h = CreateThread(0,0,mgr_forcer_thread,0,0,0); LOGF("shim: FAKE_MGR=3 manager-creator call thread started h=%p (wait ~20s then try Co-op Host)", (void*)h); } else install_fake_mgr(); }
  if (g_probe_login) install_login_probe();
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID r) {
  (void)r;
  if (reason == DLL_PROCESS_ATTACH) { DisableThreadLibraryCalls(h); g_hinst = h; init(); }
  return TRUE;
}
