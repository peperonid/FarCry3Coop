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
#include <psapi.h>
#include <intrin.h>
#pragma comment(lib, "psapi.lib")

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
  RVA_md5_update      = 0x01219270, // FUN_11219270 OpenSSL MD5_Update inside the RV module (RV-MD5 key-derivation observer, 28fc/28fd)
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
static int g_rv_probe = 0;      // env FC3COOP_RV_PROBE=1 -> OBSERVE whether FCX reaches the RV connect/dial path at Co-op entry (NOTES 28dn)
static int g_rv_md5 = 0;        // env FC3COOP_RV_MD5=1 -> read-only hook MD5_Update FUN_11219270 and hexdump prudps key-derivation input buffers (NOTES 28fc/28fd)
static int g_rv_enc = 0;        // env FC3COOP_RV_ENC=1 -> read-only capture of the RV send wrapper FUN_11214630: dump the packet buffer before+after the driver encrypts -> a known plaintext/ciphertext pair (NOTES 28g2)
static int g_mgr_watch = 0;     // env FC3COOP_MGR_WATCH=1 -> poll the online session-manager global DAT_11e42c90 and log on change (NOTES 28dp)
static int g_online_on = 0;     // env FC3COOP_ONLINE_ON=1 -> set online-enable global DAT_11f07e48=1 even in WATCH_LOGIN clean mode (try to enqueue the RV connect job) (NOTES 28dp)ation) to stop the login<->logout cycle (NOTES 28df)
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
  if (GetEnvironmentVariableA("FC3COOP_RV_MD5", buf, sizeof(buf)) > 0) g_rv_md5 = (buf[0] == '1');
  if (GetEnvironmentVariableA("FC3COOP_RV_ENC", buf, sizeof(buf)) > 0) g_rv_enc = (buf[0] == '1');
  if (GetEnvironmentVariableA("FC3COOP_MGR_WATCH", buf, sizeof(buf)) > 0) g_mgr_watch = (buf[0] == '1');
  if (GetEnvironmentVariableA("FC3COOP_ONLINE_ON", buf, sizeof(buf)) > 0) g_online_on = (buf[0] == '1');
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
// NOTES 28i7.1: like install_rv_observer but with an arbitrary clean-instruction boundary n (bytes). Needed for
// FUN_1123a870 whose prologue 55 8B EC + 81 EC <imm32> needs a 9-byte cut (6 bytes would split the sub-esp imm32).
static int install_rv_observer_n(uintptr_t rva, void* detour, uintptr_t* tr, const char* nm, int n) {
  uintptr_t f = VA(rva); uint8_t* p = (uint8_t*)f;
  __try {
    if (p[0]==0x55 && p[1]==0x8B && p[2]==0xEC) return patch_entry_n(f, n, detour, tr);
    LOGF("shim: RV-PROBE %s prologue at 0x%x not 55 8b EC (got %02x %02x %02x) - skip", nm, (uint32_t)f, p[0], p[1], p[2]);
  } __except (EXCEPTION_EXECUTE_HANDLER) { LOGF("shim: RV-PROBE %s read fault at 0x%x", nm, (uint32_t)f); }
  return -1;
}

static void install_rv_probe(void) {
  install_rv_observer(RVA_connect_auth_svc, (void*)hk_rv_cas, &g_rv_cas_tr, "ConnectToAuthSvc");
  if (!g_rv_enc) install_rv_observer(RVA_rv_sendto, (void*)hk_rv_st, &g_rv_st_tr, "rv_sendto");   // NOTES 28i2: RV-ENC-ST overrides the sendto observer with its capturing hook (same address)
  if (!g_rv_enc) install_rv_observer(RVA_rv_wsasendto, (void*)hk_rv_ws, &g_rv_ws_tr, "rv_wsasendto");   // RV-ENC overrides the WSASendTo observer with its capturing hook (same address)
  LOGF("shim: RV-PROBE observers installed (watch RV-PROBE lines: none => RV never reached; CAS only => RV connect gated by precondition; sendto too => RV actually dials, point DNS-redirect at rv_server)");
}

// ---- NOTES 28fc/28fd: RV-MD5 key-derivation OBSERVER. Read-only hook on the RV module's OpenSSL
// MD5_Update FUN_11219270 (prologue 55 8B EC 56 8B F1: ctx passed in ECX, moved to ESI). prudps derives
// its per-connection RC4 keys + packet HMAC with MD5 over the DH shared secret + the game magic/salt +
// connection ids; logging the MD5_Update INPUT buffer captures the EXACT key-derivation inputs, which
// lets us rebuild the whole scheme offline (more reconstructive than the final keys). To filter out the
// TLS/other-process MD5 use we hexdump only callers inside the RV/NetConnect code range. NO behaviour
// change: hexdump args, then tail-call the original through a 6-byte trampoline.
static uintptr_t g_rv_md5_tr = 0;
static volatile LONG g_md5_logged = 0;
static volatile LONG g_md5_calls  = 0;
#define RV_MD5_DUMP_CAP 600
static void __cdecl rv_md5_log(void* ctx, void* buf, unsigned len, void* ret){
  LONG calls = InterlockedIncrement(&g_md5_calls);
  uintptr_t r = (uintptr_t)ret;
  int rvrange = (r >= g_mod + 0x01210000u) && (r <= g_mod + 0x01280000u);
  LONG done = InterlockedCompareExchange(&g_md5_logged, 0, 0);
  if (!rvrange) { if ((calls % 5000) == 0) { logLock(); LOGF("shim: RV-MD5 %ld MD5_Update calls so far (%ld RV-range dumped)", calls, done); logUnlock(); } return; }
  if (InterlockedIncrement(&g_md5_logged) > RV_MD5_DUMP_CAP) return;
  unsigned cap = len > 256 ? 256 : len;
  logLock();
  LOGF("shim: RV-MD5 MD5_Update ctx=%p buf=%p len=%u ret=0x%08x (RV-range caller)", ctx, buf, len, (uint32_t)r);
  __try {
    unsigned char* b = (unsigned char*)buf;
    char line[100];
    for (unsigned i = 0; i < cap; ) {
      int k = wsprintfA(line, "    %04x:", i);
      unsigned j = 0;
      for (; j < 16 && i + j < cap; ++j) k += wsprintfA(line + k, " %02x", b[i + j]);
      LOGF("%s", line);
      i += 16;
    }
    if (len > cap) LOGF("    ...(%u more bytes)", len - cap);
  } __except (EXCEPTION_EXECUTE_HANDLER) { LOGF("    <read fault>"); }
  logUnlock();
}
__declspec(naked) static void hk_rv_md5(void){
  __asm {
    pushad                       // [esp]=EDI [+4]=ESI [+8]=EBP [+12]=esp0 [+16]=EBX [+20]=EDX [+24]=ECX [+28]=EAX
    mov ebp, esp                 // base; ctx=ECX=[ebp+24]  ret=[ebp+32]  buf=[ebp+36]  len=[ebp+40]
    push dword ptr [ebp+32]      // ret (4th arg)
    push dword ptr [ebp+40]      // len (3rd arg)
    push dword ptr [ebp+36]      // buf (2nd arg)
    push dword ptr [ebp+24]      // ctx (1st arg)
    call rv_md5_log
    add esp, 16
    popad
    jmp dword ptr [g_rv_md5_tr]
  }
}
static int install_rv_md5(void){
  int rc = install_rv_observer(RVA_md5_update, (void*)hk_rv_md5, &g_rv_md5_tr, "rv_md5_update");
  LOGF("shim: RV-MD5 observer install rc=%d (FUN_11219270 MD5_Update RVA 0x01219270); read RV-MD5 lines: each RV-range MD5_Update input buffer = a prudps key-derivation input");
  return rc;
}

// ---- NOTES 28g2: RV-ENC plaintext/ciphertext CAPTURE. Read-only observer on the RV socket send wrapper
// FUN_11214630 (thiscall: ECX=this, param_2=buffer, param_3=length, param_4=sockaddr, param_5=out struct).
// Confirmed in SigSend.java: when the secured socket driver is present (*(this+0x9c)!=0) this function does NOT
// call the raw WSASendTo; it calls the driver vtable send (slot +0x10 with an addr, or +0x1c without), which
// applies the Quazal prudps stream cipher and sends. So param_2 here is the PLAIN pre-cipher packet buffer.
// We dump the buffer BEFORE the original, invoke the original (driver encrypts + sends) via a 6-byte thiscall
// trampoline, then dump the buffer AFTER. If the driver encrypts IN PLACE, after = ciphertext and one capture
// yields a known plaintext/ciphertext pair => the per-connection keystream = pt^ct, NO static secret needed.
// NO behaviour change: exactly one call to the original, its return value passed through unchanged.
typedef int (__thiscall *rv_wsasendto_t)(void* thisp, void* buf, int len, void* addr, void** out);
static void rv_drv_hook_resolve(void* thisp);   // NOTES 28g3: forward decl (defined after install_rv_enc)
static void rv_this_window_scan(void* thisp, const char* from);  // NOTES 28i2: log thisp window + scan for the real driver vtable offset (call with logLock held)
static uintptr_t g_rv_enc_tr = 0;
static volatile LONG g_enc_logged = 0;
#define RV_ENC_DUMP_CAP 80
static void rv_enc_dump(const char* tag, unsigned char* b, int len) {
  int cap = len > 256 ? 256 : len;
  char line[120];
  LOGF("shim:   %s:", tag);
  for (int i = 0; i < cap; ) {
    int k = wsprintfA(line, "    %04x:", i);
    for (int j = 0; j < 16 && i + j < cap; ++j) k += wsprintfA(line + k, " %02x", b[i + j]);
    LOGF("%s", line);
    i += 16;
  }
}
static unsigned rv_enc_sum(unsigned char* b, int len) {  // checksum over the payload region [8,len)
  unsigned s = 0; for (int i = 8; i < len; ++i) s = (s + b[i] + (s << 5)) & 0xffffffffu; return s;
}
static int __cdecl rv_enc_capture(void* thisp, void* buf, int len, void* addr, void** out) {
  int rc = -1;
  LONG idx = InterlockedIncrement(&g_enc_logged);
  int do_log = (idx <= RV_ENC_DUMP_CAP);
  unsigned s0 = 0;
  __try {
    int ok = (buf && len > 8 && len < 65536);
    if (do_log && ok) {
      unsigned char* b = (unsigned char*)buf;
      unsigned drv = 0; int slotsel = -1;
      if (thisp) { drv = *(volatile unsigned*)((char*)thisp + 0x9c); slotsel = *(volatile char*)((char*)thisp + 0x88); }
      unsigned cid = (unsigned)*(volatile int*)((char*)buf + 4);
      s0 = rv_enc_sum(b, len);
      logLock();
      LOGF("shim: RV-ENC #%ld this=%p driver=0x%08x slotSel=%d len=%d connId=%08x", idx, thisp, drv, slotsel, len, cid);
      rv_enc_dump("PRE(plaintext)", b, len);
      if (thisp) rv_this_window_scan(thisp, "wsasendto");   // NOTES 28i2: learn the REAL driver-pointer offset (this read 0x0 at +0x9c)
      logUnlock();
    }
    // NOTES 28g3: arm the driver-vtable capture on the FIRST RV send, BEFORE we invoke the original (which is
    // what calls the driver vtable send). Even if the cipher encrypts an internal COPY (payload_changed=0 here),
    // the driver-method repl sees the buffer at the vtable boundary -> a second independent plaintext view, and
    // the on-wire ciphertext is already in the captured pcap, so PRE + pcap still yields the keystream.
    if (do_log && ok && thisp) rv_drv_hook_resolve(thisp);
    if (g_rv_enc_tr) rc = ((rv_wsasendto_t)g_rv_enc_tr)(thisp, buf, len, addr, out);
    if (do_log && ok) {
      unsigned char* b = (unsigned char*)buf;
      unsigned s1 = rv_enc_sum(b, len);
      int changed = (s0 != s1);
      logLock();
      LOGF("shim: RV-ENC #%ld POST rc=%d payload_changed=%d%s", idx, rc, changed, changed ? "  <== IN-PLACE cipher: this PRE/POST pair IS the keystream (pt^ct), no secret needed" : "  <== driver encrypted a copy (no in-place change); a driver-side hook is needed next");
      rv_enc_dump("POST(after)", b, len);
      logUnlock();
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) { LOGF("shim: RV-ENC #%ld capture fault rc=%d", idx, rc); }
  return rc;
}
__declspec(naked) static void hk_rv_ws_enc(void){ __asm {
  push ebp
  mov  ebp, esp
  // ECX=this; [ebp+4]=ret; [ebp+8]=buf(param_2); [ebp+0xc]=len(param_3); [ebp+0x10]=addr(param_4); [ebp+0x14]=out(param_5)
  push dword ptr [ebp+0x14]
  push dword ptr [ebp+0x10]
  push dword ptr [ebp+0x0c]
  push dword ptr [ebp+0x08]
  push ecx
  call rv_enc_capture
  add  esp, 20
  pop  ebp
  ret  16                       // thiscall: callee cleans the 4 stack args (16 bytes), like the original
} }
static int install_rv_enc(void){
  int rc = install_rv_observer(RVA_rv_wsasendto, (void*)hk_rv_ws_enc, &g_rv_enc_tr, "rv_wsasendto_enc");
  LOGF("shim: RV-ENC capture install rc=%d (FUN_11214630 RVA 0x01214630). Do Co-op -> Host with a REAL login; read RV-ENC lines. payload_changed=1 => the PRE/POST pair IS the plaintext/ciphertext -> keystream = pt^ct (no static secret). payload_changed=0 => the driver encrypts a copy; next add a driver-vtable-side capture.");
  return rc;
}

// ---- NOTES 28i2: thisp-window dump + driver-offset scan. The RV send wrappers log driver=0 at thisp+0x9c
// (28g3/28g4), so the connection's socket-driver pointer is NOT at +0x9c for the connections we hit. This dumps
// the thisp region around +0x9c and scans +0x0..0x140 for ANY pointer that behaves like a socket-driver object
// (an in-module object whose vtable has in-module function pointers at +0x10 and/or +0x1c), and logs every
// candidate so the real offset can be found from the log (NO writes: pure read, cannot crash the game beyond the
// __try). Call it while holding logLock (it only calls LOGF, no internal locking).
static void rv_this_window_scan(void* thisp, const char* from) {
  __try {
    unsigned char* o = (unsigned char*)thisp;
    LOGF("shim:   THIS[%s] window around +0x9c:", from);
    for (int off = 0x80; off < 0x120; off += 16) {
      char line[140]; int k = wsprintfA(line, "    +%02x:", off);
      for (int j = 0; j < 16; j++) k += wsprintfA(line + k, " %02x", o[off + j]);
      LOGF("%s", line);
    }
    LOGF("shim:   THIS[%s] driver-candidate scan (off=field; obj in-module; vtbl in-module; +0x10/+0x1c in-module):", from);
    int found = 0;
    for (int off = 0; off <= 0x140; off += 4) {
      uintptr_t p = *(volatile uintptr_t*)(o + off);
      if (p < g_mod || p >= g_mod + g_imgsize) continue;            // field not an in-module object pointer
      uintptr_t vt = *(volatile uintptr_t*)p;                        // candidate vtable
      if (vt < g_mod || vt >= g_mod + g_imgsize) continue;          // not an in-module vtable
      uintptr_t m10 = *(volatile uintptr_t*)(vt + 0x10), m1c = *(volatile uintptr_t*)(vt + 0x1c);
      int ok10 = (m10 >= g_mod && m10 < g_mod + g_imgsize), ok1c = (m1c >= g_mod && m1c < g_mod + g_imgsize);
      if (ok10 || ok1c) {
        LOGF("      off=0x%02x obj=0x%08x (rva 0x%x) vtbl=0x%08x slot10=%s slot1c=%s", off, (uint32_t)p, (uint32_t)(p - g_mod), (uint32_t)vt, ok10 ? "in" : "-", ok1c ? "in" : "-");
        found++; if (found >= 16) break;
      }
    }
    if (!found) LOGF("      none in +0x0..0x140");
  } __except (EXCEPTION_EXECUTE_HANDLER) { LOGF("shim:   THIS[%s] scan fault", from); }
}

// ---- NOTES 28i4: name the module that OWNS an address. The RV-PKT packet-object vtable (e.g. 0x79F7EBBC with FC3
// base 0x10000000) is NOT inside FC3_d3d11.dll -> the Quazal packet class and the prudps cipher live in ANOTHER
// module we never analyzed statically (all RC4 hunts scanned only FC3). This logs the owning module name + base +
// rva so Ghidra can target the right binary. Caller must hold logLock.
static void rv_module_of(void* addr, const char* label) {
  __try {
    HMODULE mods[512]; DWORD cb = 0; uintptr_t a = (uintptr_t)addr;
    if (!K32EnumProcessModules(GetCurrentProcess(), mods, sizeof(mods), &cb)) { LOGF("shim:   RV-MOD %s: enum failed err=%u", label, GetLastError()); return; }
    int n = (int)(cb / sizeof(HMODULE));
    for (int i = 0; i < n; i++) {
      MODULEINFO mi;
      if (!K32GetModuleInformation(GetCurrentProcess(), mods[i], &mi, sizeof(mi))) continue;
      uintptr_t b = (uintptr_t)mi.lpBaseOfDll;
      if (a >= b && a < b + mi.SizeOfImage) {
        char nm[MAX_PATH]; nm[0] = 0; K32GetModuleBaseNameA(GetCurrentProcess(), mods[i], nm, sizeof(nm));
        LOGF("shim:   RV-MOD %s addr=0x%08x -> module '%s' base=0x%08x size=0x%x rva=0x%x", label, (uint32_t)a, nm, (uint32_t)b, (unsigned)mi.SizeOfImage, (uint32_t)(a - b));
        return;
      }
    }
    LOGF("shim:   RV-MOD %s addr=0x%08x not in any module (heap/jit?)", label, (uint32_t)a);
  } __except (EXCEPTION_EXECUTE_HANDLER) { LOGF("shim:   RV-MOD %s fault", label); }
}

// ---- NOTES 28i2: RV-ENC-ST. Capture the RV *sendto* wrapper FUN_11218510 - in the 28g8 run the RV data actually
// left via sendto, not WSASendTo, so the WSASendTo-only capture saw 0. This is an ABI-NEUTRAL observer (save all
// regs+flags, call a PRE dumper, restore, tail-jump the trampoline) so it cannot corrupt the stack regardless of
// the wrapper's arg count - unlike the ret-N capture hook. We dump only PRE (the plaintext at the wrapper); the
// matching on-wire CIPHERTEXT is already captured by pktmon in rv.pcap, so PRE^pcap still yields the keystream.
static uintptr_t g_rv_st_enc_tr = 0;
static volatile LONG g_st_logged = 0;
static void __cdecl rv_st_pre(void* thisp, void** sp) {
  LONG idx = InterlockedIncrement(&g_st_logged); int do_log = (idx <= RV_ENC_DUMP_CAP);
  __try {
    if (!do_log) return;
    // at the wrapper entry: sp[0]=retaddr, sp[1]=param_2(buf), sp[2]=param_3(len)
    unsigned char* b = (unsigned char*)sp[1]; int len = (int)sp[2];
    unsigned drv = 0; if (thisp) drv = *(volatile unsigned*)((char*)thisp + 0x9c);
    logLock();
    if (b && len > 8 && len < 65536) {
      unsigned cid = (unsigned)*(volatile int*)(b + 4);
      LOGF("shim: RV-ENC-ST #%ld this=%p driver=0x%08x len=%d connId=%08x  (sendto FUN_11218510 PRE only; on-wire ciphertext is in rv.pcap)", idx, thisp, drv, len, cid);
      rv_enc_dump("PRE(sendto-in)", b, len);
      if (thisp) rv_this_window_scan(thisp, "sendto");
    } else {
      LOGF("shim: RV-ENC-ST #%ld this=%p driver=0x%08x argbuf=%p arglen=%d (not a plausible buffer)", idx, thisp, drv, b, len);
    }
    logUnlock();
  } __except (EXCEPTION_EXECUTE_HANDLER) { logLock(); LOGF("shim: RV-ENC-ST #%ld fault", idx); logUnlock(); }
}
__declspec(naked) static void hk_rv_st_capture(void){ __asm {
  pushfd
  pushad
  mov  eax, esp
  add  eax, 36            // ESP0 (== &retaddr) = current esp + 4 (pushfd) + 32 (pushad)
  push eax                // arg2 = sp
  push ecx                // arg1 = this (thiscall this is in ECX on entry)
  call rv_st_pre
  add  esp, 8
  popad
  popfd
  jmp  dword ptr [g_rv_st_enc_tr]
} }
static int install_rv_st_enc(void) {
  int rc = install_rv_observer(RVA_rv_sendto, (void*)hk_rv_st_capture, &g_rv_st_enc_tr, "rv_sendto_enc");
  LOGF("shim: RV-ENC-ST install rc=%d (FUN_11218510 RV sendto RVA 0x01218510). ABI-safe PRE capture; read RV-ENC-ST lines + THIS[sendto] window to find the real driver offset. POST/ciphertext comes from rv.pcap.");
  return rc;
}

// ---- NOTES 28g3: driver-vtable capture (the payload_changed=0 fallback). Instead of inline-patching an
// unknown driver method (relocating 6 bytes is only safe for a known '55 8B EC..' prologue), we SWAP the two
// send slots in the socket-driver vtable: resolve it at runtime from the RV object thisp (+0x9c) -> vtable, then
// write our thiscall replacements into slots +0x10 (4 args, ret16) and +0x1c (3 args, ret12), saving the original
// pointers. The replacement dumps the buffer on ENTRY (plaintext at the driver boundary) and on EXIT (in-place
// change), then calls the original. vtable swap needs NO instruction relocation, so it is safe on an unknown
// target. Runs once (InterlockedCompareExchange guard); both send slots are in one 16-byte RW page (vtbl+0x10).
typedef int (__thiscall *drv10_t)(void* thisp, void* buf, int len, void* addr, void** out);
typedef int (__thiscall *drv1c_t)(void* thisp, void* buf, int len, void** out);
static uintptr_t g_drv10_orig = 0, g_drv1c_orig = 0;
static volatile LONG g_drv_hooked = 0;
static volatile LONG g_drv_logged = 0;
#define RV_DRV_DUMP_CAP 80
static unsigned rv_drv_sum(unsigned char* b, int len) { unsigned s = 0; for (int i = 8; i < len; ++i) s = (s + b[i] + (s << 5)) & 0xffffffffu; return s; }
static int __cdecl rv_drv_work10(void* thisp, void* buf, int len, void* addr, void** out) {
  int rc = -1; LONG idx = InterlockedIncrement(&g_drv_logged); int do_log = (idx <= RV_DRV_DUMP_CAP); unsigned s0 = 0;
  __try {
    int ok = (buf && len > 8 && len < 65536);
    if (do_log && ok) {
      unsigned char* b = (unsigned char*)buf; unsigned cid = (unsigned)*(volatile int*)((char*)buf + 4); s0 = rv_drv_sum(b, len);
      logLock(); LOGF("shim: RV-DRV10 #%ld this=%p len=%d connId=%08x", idx, thisp, len, cid); rv_enc_dump("PRE(driver-in)", b, len); logUnlock();
    }
    if (g_drv10_orig) rc = ((drv10_t)g_drv10_orig)(thisp, buf, len, addr, out);
    if (do_log && ok) {
      unsigned char* b = (unsigned char*)buf; unsigned s1 = rv_drv_sum(b, len); int changed = (s0 != s1);
      logLock(); LOGF("shim: RV-DRV10 #%ld POST rc=%d payload_changed=%d%s", idx, rc, changed, changed ? "  <== IN-PLACE cipher at driver boundary" : "  <== driver encrypts an internal copy (no in-place change here)"); rv_enc_dump("POST(driver-out)", b, len); logUnlock();
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) { LOGF("shim: RV-DRV10 #%ld fault rc=%d", idx, rc); }
  return rc;
}
static int __cdecl rv_drv_work1c(void* thisp, void* buf, int len, void** out) {
  int rc = -1; LONG idx = InterlockedIncrement(&g_drv_logged); int do_log = (idx <= RV_DRV_DUMP_CAP); unsigned s0 = 0;
  __try {
    int ok = (buf && len > 8 && len < 65536);
    if (do_log && ok) {
      unsigned char* b = (unsigned char*)buf; unsigned cid = (unsigned)*(volatile int*)((char*)buf + 4); s0 = rv_drv_sum(b, len);
      logLock(); LOGF("shim: RV-DRV1C #%ld this=%p len=%d connId=%08x", idx, thisp, len, cid); rv_enc_dump("PRE(driver-in)", b, len); logUnlock();
    }
    if (g_drv1c_orig) rc = ((drv1c_t)g_drv1c_orig)(thisp, buf, len, out);
    if (do_log && ok) {
      unsigned char* b = (unsigned char*)buf; unsigned s1 = rv_drv_sum(b, len); int changed = (s0 != s1);
      logLock(); LOGF("shim: RV-DRV1C #%ld POST rc=%d payload_changed=%d%s", idx, rc, changed, changed ? "  <== IN-PLACE cipher at driver boundary" : "  <== driver encrypts an internal copy (no in-place change here)"); rv_enc_dump("POST(driver-out)", b, len); logUnlock();
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) { LOGF("shim: RV-DRV1C #%ld fault rc=%d", idx, rc); }
  return rc;
}
// thiscall entry stubs (ECX=this; the game cleans the stack args with ret16/ret12, we mirror that) that hand
// off to the __cdecl workers (MSVC forbids __thiscall on a free function; only the fn-pointer typedefs carry it).
__declspec(naked) static void rv_drv_repl10(void){ __asm {
  push ebp
  mov  ebp, esp
  push dword ptr [ebp+0x14]   // out
  push dword ptr [ebp+0x10]   // addr
  push dword ptr [ebp+0x0c]   // len
  push dword ptr [ebp+0x08]   // buf
  push ecx                    // this
  call rv_drv_work10
  add  esp, 20
  pop  ebp
  ret  16
} }
__declspec(naked) static void rv_drv_repl1c(void){ __asm {
  push ebp
  mov  ebp, esp
  push dword ptr [ebp+0x10]   // out
  push dword ptr [ebp+0x0c]   // len
  push dword ptr [ebp+0x08]   // buf
  push ecx                    // this
  call rv_drv_work1c
  add  esp, 16
  pop  ebp
  ret  12
} }
static void rv_drv_hook_resolve(void* thisp) {
  if (InterlockedCompareExchange(&g_drv_hooked, 1, 0) != 0) return;
  __try {
    // NOTES 28i2: the driver field is NOT at +0x9c (it read 0x0 every run), so SCAN thisp for the field that
    // points at an object whose vtable has in-module function pointers at BOTH +0x10 and +0x1c (the secured
    // socket-driver send pair). Hook whichever offset we find instead of the hard-coded +0x9c.
    uintptr_t drv = 0; int drv_off = -1;
    unsigned char* o = (unsigned char*)thisp;
    for (int off = 0; off <= 0x200 && drv == 0; off += 4) {
      uintptr_t p = *(volatile uintptr_t*)(o + off);
      if (p < 0x10000u) continue;
      if (p < g_mod || p >= g_mod + g_imgsize) continue;                 // must be an in-module object
      uintptr_t vt = *(volatile uintptr_t*)p;
      if (vt < g_mod || vt >= g_mod + g_imgsize) continue;               // in-module vtable
      uintptr_t s10 = *(volatile uintptr_t*)(vt + 0x10), s1c = *(volatile uintptr_t*)(vt + 0x1c);
      if (s10 >= g_mod && s10 < g_mod + g_imgsize && s1c >= g_mod && s1c < g_mod + g_imgsize) { drv = p; drv_off = off; }
    }
    if (drv == 0) { logLock(); LOGF("shim: RV-DRV no driver-like field in thisp+0x0..0x200 (this=%p) - will retry next send", thisp); logUnlock(); InterlockedExchange(&g_drv_hooked, 0); return; }
    uintptr_t vtbl = *(volatile uintptr_t*)drv;
    uintptr_t m10 = *(volatile uintptr_t*)(vtbl + 0x10), m1c = *(volatile uintptr_t*)(vtbl + 0x1c);
    int ok10 = (m10 >= g_mod && m10 < g_mod + g_imgsize), ok1c = (m1c >= g_mod && m1c < g_mod + g_imgsize);
    logLock(); LOGF("shim: RV-DRV found driver at thisp+0x%x: driver=0x%08x vtbl=0x%08x slot10=0x%08x(rva 0x%x) slot1c=0x%08x(rva 0x%x)", drv_off, (uint32_t)drv, (uint32_t)vtbl, (uint32_t)m10, (uint32_t)(m10 - g_mod), (uint32_t)m1c, (uint32_t)(m1c - g_mod));
    DWORD old;
    if (VirtualProtect((void*)(vtbl + 0x10), 0x10, PAGE_READWRITE, &old)) {
      if (ok10) { g_drv10_orig = m10; *(volatile uintptr_t*)(vtbl + 0x10) = (uintptr_t)&rv_drv_repl10; }
      if (ok1c) { g_drv1c_orig = m1c; *(volatile uintptr_t*)(vtbl + 0x1c) = (uintptr_t)&rv_drv_repl1c; }
      VirtualProtect((void*)(vtbl + 0x10), 0x10, old, &old);
      FlushInstructionCache(GetCurrentProcess(), (void*)(vtbl + 0x10), 0x10);
      logUnlock(); logLock(); LOGF("shim: RV-DRV vtable slots swapped: slot10=%s slot1c=%s (watch RV-DRV10/RV-DRV1C lines)", ok10 ? "YES" : "no", ok1c ? "YES" : "no"); logUnlock();
    } else { logUnlock(); logLock(); LOGF("shim: RV-DRV VirtualProtect on vtable failed - skip"); logUnlock(); InterlockedExchange(&g_drv_hooked, 0); }
  } __except (EXCEPTION_EXECUTE_HANDLER) { logLock(); LOGF("shim: RV-DRV resolve fault"); logUnlock(); InterlockedExchange(&g_drv_hooked, 0); }
}

// ---- NOTES 28g5: Quazal packet-OBJECT serialize capture (the upstream plaintext hunt; NOTES 28g4 found RV-ENC is
// below the cipher). FUN_112161b0 (one of the two FUN_11214630 callers) is `__thiscall(this, packetObject, p3)`:
// it calls the packet object's vtable method +0x4 `(**(code**)(*obj + 4))()` then FUN_11214630(obj[2]=buf, obj[3]=len).
// Packet-object layout: vptr@0, [2]=buffer ptr (+8), [3]=length (+0xc), [4]=capacity (+0x10). We (a) install an
// inline observer on FUN_112161b0 that dumps the object buffer at entry (the plaintext candidate) and on exit, and
// (b) SWAP the object's vtable slot +0x4 (resolved at runtime from *obj) with a capture stub that dumps the same
// buffer before/after the serialize method runs. If the prudps cipher is applied inside that +0x4 method, its PRE is
// the plaintext and the pair yields the keystream; if entry is already ciphertext we still learn where it turns over.
// All read-only (one call to each original, return value passed through); vtable swap needs no instruction relocation.
typedef int (__thiscall *pkt_send_t)(void* thisp, void* obj, int p3);
static uintptr_t g_rv_pkt_tr = 0;          // FUN_112161b0 trampoline
static volatile LONG g_pkt_logged = 0;     // NOTES 28g6: vtable +0x4 wrap removed (crashed the game; +0x4 is not the cipher)
#define RV_PKT_DUMP_CAP 80
static int __cdecl rv_pkt_capture(void* thisp, void* obj, int p3, void* retaddr) {
  int rc = -1; LONG idx = InterlockedIncrement(&g_pkt_logged); int do_log = (idx <= RV_PKT_DUMP_CAP); unsigned s0 = 0;
  __try {
    unsigned char* b = 0; int len = 0;
    if (obj) { b = *(unsigned char**)((char*)obj + 8); len = *(volatile int*)((char*)obj + 0xc); }
    int ok = (b && len > 8 && len < 65536);
    if (do_log && ok) { s0 = rv_enc_sum(b, len); unsigned cid = (unsigned)*(volatile int*)(b + 4);
      logLock(); LOGF("shim: RV-PKT #%ld obj=%p vtbl=%p len=%d connId=%08x", idx, obj, *(void**)obj, len, cid);
      if (idx <= 4) LOGF("shim: RV-PKT #%ld CALLER=0x%08x (rva 0x%x) this=0x%08x", idx, (uint32_t)(uintptr_t)retaddr, (uint32_t)((uintptr_t)retaddr - g_mod), (uint32_t)(uintptr_t)thisp);
      rv_enc_dump("PRE(obj@entry)", b, len); logUnlock(); }
    // NOTES 28i2: FUN_112161b0 is the RV send path that ACTUALLY fires (its secured-driver branch skips
    // FUN_11214630, so RV-ENC/RV-ENC-ST saw 0 this run). Scan its connection `thisp` for the real socket-driver
    // vtable and ARM the send-slot swap BEFORE invoking the original, so this very call's driver send is captured
    // in-place by RV-DRV10/RV-DRV1C => payload_changed=1 => the PRE/POST pair IS the keystream (pt^ct), no secret.
    if (do_log && thisp) { logLock(); if (idx <= 3) { rv_module_of(thisp, "this"); if (obj) { rv_module_of(*(void**)obj, "obj-vtbl"); rv_module_of(obj, "obj"); } rv_this_window_scan(thisp, "pkt-conn"); } logUnlock(); rv_drv_hook_resolve(thisp); }
    // NOTES 28g6: vtable +0x4 wrap REMOVED - the +0x4 method is NOT the cipher (data is already encrypted
    // at FUN_112161b0 entry, NOTES 28g4/28g6) and the thiscall swap corrupted the method ABI and crashed the game on load.
    if (g_rv_pkt_tr) rc = ((pkt_send_t)g_rv_pkt_tr)(thisp, obj, p3);
    if (do_log && ok) {
      unsigned s1 = rv_enc_sum(b, len); int changed = (s0 != s1);
      logLock(); LOGF("shim: RV-PKT #%ld POST rc=%d payload_changed=%d", idx, rc, changed); rv_enc_dump("POST(obj@exit)", b, len); logUnlock();
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) { LOGF("shim: RV-PKT #%ld fault rc=%d", idx, rc); }
  return rc;
}
__declspec(naked) static void hk_rv_pkt(void){ __asm {   // FUN_112161b0 is thiscall: ECX=this, [ebp+8]=obj, [ebp+0xc]=p3
  push ebp
  mov  ebp, esp
  push dword ptr [ebp+0x04]   // NOTES 28i7: the return address = the CALLER of FUN_112161b0 (find the real upstream)
  push dword ptr [ebp+0x0c]
  push dword ptr [ebp+0x08]
  push ecx
  call rv_pkt_capture
  add  esp, 16
  pop  ebp
  ret  8
} }
static void install_rv_pkt(void){
  int rc = install_rv_observer(0x012161b0u, (void*)hk_rv_pkt, &g_rv_pkt_tr, "rv_pkt_send");   // FUN_112161b0 (prologue 55 8B EC verified)
  LOGF("shim: RV-PKT capture install rc=%d (FUN_112161b0 RVA 0x012161b0). Do Co-op -> Host w/ real login; read RV-PKT lines (entry/exit buffer dump around FUN_112161b0). NOTES 28g6: data is already encrypted at this entry -> the cipher is UPSTREAM of FUN_112161b0; the +0x4 vtable wrap was removed because it crashed the game.");
}

// ---- NOTES 28i6: dynamic INDIRECT-CALL tracer (path #2). Static call-graph dead-ends because the prudps RC4
// is dispatched through an indirect call (vtable / fn-ptr) that Ghidra cannot resolve. So: single-step ONE
// send-path descent at runtime (armed on FUN_11231860, prologue 55 8B EC) and log every indirect CALL target
// (eip rva -> target rva, flagged when the target is OUTSIDE the send block -> the cipher candidate). The
// target of the call that turns the packet buffer into ciphertext is the RC4; we then hook it next run.
// OBSERVER-ONLY: TF single-step via a vectored handler, never rewrites game code, one-shot, hard step cap.
static uintptr_t      g_tr31860 = 0;
static void*          g_step_veh = 0;
static volatile LONG  g_step_done = 0;     // arm exactly once
static volatile LONG  g_step_active = 0;   // stepping in progress (the armed thread has TF)
static volatile LONG  g_step_count = 0;
static volatile LONG  g_step_ilen = 0;
static const LONG     g_step_cap = 60000;     // per traced invocation

static uintptr_t x86_reg(const CONTEXT* c, int r) {
  switch (r & 7) {
    case 0: return c->Eax; case 1: return c->Ecx; case 2: return c->Edx; case 3: return c->Ebx;
    case 4: return c->Esp; case 5: return c->Ebp; case 6: return c->Esi; default: return c->Edi;
  }
}

// decode an indirect CALL (FF /2) at eip; resolve its target from the current register/memory state.
static int rv_icall_target(uintptr_t eip, const CONTEXT* c, uintptr_t* target) {
  unsigned char* b = (unsigned char*)eip;
  int i = 0;
  for (;;) { unsigned char p = b[i];
    if (p==0x66||p==0x67||p==0xF0||p==0xF2||p==0xF3||p==0x2E||p==0x36||p==0x3E||p==0x26||p==0x64||p==0x65) { i++; continue; }
    break;
  }
  if (b[i] != 0xFF) return 0;
  unsigned char modrm = b[i+1]; int mod = modrm>>6, reg = (modrm>>3)&7, rm = modrm&7;
  if (reg != 2) return 0;            // CALL /2 only (skip jmp /4, callf /3)
  i += 2;
  if (mod == 3) { *target = x86_reg(c, rm); return 1; }
  uintptr_t ea;
  if (rm == 4) {                      // SIB
    unsigned char sib = b[i++]; int scale = 1 << (sib>>6); int idx = (sib>>3)&7; int base = sib&7;
    ea = 0;
    if (base == 5) { ea = (uintptr_t)*(uint32_t*)(b+i); i += 4; }
    else           { ea = x86_reg(c, base); }
    if (idx != 4) ea += (uintptr_t)(scale * x86_reg(c, idx));
  } else if (mod == 0 && rm == 5) { ea = (uintptr_t)*(uint32_t*)(b+i); i += 4; }
  else { ea = x86_reg(c, rm); if (mod == 1) ea += (uintptr_t)(signed char)b[i++]; else if (mod == 2) { ea += (uintptr_t)*(uint32_t*)(b+i); i += 4; } }
  *target = *(volatile uintptr_t*)ea;
  return 1;
}

static LONG CALLBACK rv_step_veh(EXCEPTION_POINTERS* ep) {
  if (!g_step_active) return EXCEPTION_CONTINUE_SEARCH;
  if (ep->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP) return EXCEPTION_CONTINUE_SEARCH;
  CONTEXT* c = ep->ContextRecord;
  uintptr_t eip = (uintptr_t)c->Eip, base = g_mod;
  __try {
    uintptr_t tgt = 0;
    if (rv_icall_target(eip, c, &tgt)) {
      if (g_step_ilen < 600) {
        LONG n = InterlockedIncrement(&g_step_ilen);
        if (tgt >= base && tgt < base + g_imgsize) {
          uintptr_t r = tgt - base;
          int inpath = (r >= 0x01210000u && r <= 0x01245000u);
          logLock(); LOGF("shim: RV-TRGL #%ld icall eip=0x%08x(rva 0x%x) -> 0x%08x(rva 0x%x) %s", n, (uint32_t)eip, (uint32_t)(eip-base), (uint32_t)tgt, (uint32_t)r, inpath?"":"<<< NON-SENDPATH (cipher?)"); logUnlock();
        } else {
          logLock(); LOGF("shim: RV-TRGL #%ld icall eip=0x%08x(rva 0x%x) -> 0x%08x OUTSIDE module", n, (uint32_t)eip, (uint32_t)(eip-base), (uint32_t)tgt); logUnlock();
        }
      }
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) { /* bad decode - just continue */ }
  if (InterlockedIncrement(&g_step_count) > g_step_cap) {
    g_step_active = 0; c->EFlags &= ~0x100u;
    logLock(); LOGF("shim: RV-TRGL step cap %d reached; tracer stopped after logging %ld indirect calls", g_step_cap, g_step_ilen); logUnlock();
    return EXCEPTION_CONTINUE_EXECUTION;
  }
  c->EFlags |= 0x100u;   // keep single-stepping
  return EXCEPTION_CONTINUE_EXECUTION;
}

static void rv_set_tf(void) { __writeeflags(__readeflags() | 0x100u); }   // enable the trap flag (single-step)

// ---- NOTES 28i9: HARDWARE-BREAKPOINT cipher-boundary capture (replaces the single-step tracer that crashed).
// Disasm of FUN_1123a870 (the on-path send caller) pins the cipher EXACTLY:
//   0x1123ab17  CMP dword ptr [EDX+0x618],0x0     ; the cipher-handler object lives at session+0x618
//   0x1123ab5c  MOV ECX,[EAX+0x618]               ; ECX = the cipher-handler object
//   0x1123ab69  MOV EDX,[ECX]                     ; its vtable
//   0x1123ab6b  MOV EDX,[EDX+0x4]                 ; slot +4 = the buffer cipher method
//   0x1123ab6e  LEA EAX,[EBP-0x108] ; PUSH EAX    ; &packet-buffer
//   0x1123ab75  CALL EDX                           ; *** cipher: buffer becomes ciphertext in place ***
//   0x1123ab77  CMP byte ptr [EBP-0xc0],0x0        ; first insn AFTER the cipher returns
// So two HW execute breakpoints give a direct PRE(plaintext)/POST(ciphertext) pair on the SAME buffer -> the
// shim XORs them into the keystream itself. No TF single-step, no stepping into system code, no game slowdown.
static void*          g_hwbp_veh = 0;
static volatile LONG  g_hwbp_armed = 0;
static volatile LONG  g_cip_pairs = 0;          // completed PRE/POST keystream pairs
static volatile LONG  g_cip_ptr_logged = 0;
#define RV_CI_PRE  0x0123ab75u                    // CALL EDX (cipher)  - fires before -> buffer is plaintext
#define RV_CI_POST 0x0123ab77u                    // first insn after the cipher ret - buffer is ciphertext
static uintptr_t      g_pre_dp = 0;
static LONG           g_pre_len = 0;
static unsigned char  g_pre_buf[256];

static void cip_hex(const char* tag, const unsigned char* buf, int n) {
  char out[3*64+1]; int k = 0;
  if (n > 32) n = 32;                             // keep one line; keystream prefix is what we need first
  for (int i = 0; i < n; i++) { k += sprintf(out + k, "%02x ", buf[i]); }
  if (k && out[k-1] == ' ') out[k-1] = 0;
  LOGF("shim: %s %s", tag, out);
}

static LONG CALLBACK rv_hwbp_veh(EXCEPTION_POINTERS* ep) {
  DWORD code = ep->ExceptionRecord->ExceptionCode;
  CONTEXT* c = ep->ContextRecord;
  if (code == EXCEPTION_BREAKPOINT) {             // our arm-int3 fallback: install both HW execute BPs, skip int3
    c->Eip += 1;
    c->Dr0 = (DWORD)(g_mod + RV_CI_PRE);
    c->Dr1 = (DWORD)(g_mod + RV_CI_POST);
    c->Dr7 = (c->Dr7 & ~0x00FF00FFu) | 0x05u;     // BP0 + BP1 local EXECUTE (type/len 00)
    return EXCEPTION_CONTINUE_EXECUTION;
  }
  if (code != EXCEPTION_SINGLE_STEP) return EXCEPTION_CONTINUE_SEARCH;   // HW BP fires as a #DB
  DWORD d6 = c->Dr6; c->Dr6 = 0;
  if (!(d6 & 0x3)) return EXCEPTION_CONTINUE_SEARCH;                     // not our breakpoints
  if (g_cip_pairs > 48) return EXCEPTION_CONTINUE_EXECUTION;            // enough keystream samples
  __try {
    uintptr_t slot = (uintptr_t)c->Ebp - 0x108u;  // the 8-dword packet-buffer slot in FUN_1123a870
    uintptr_t dp = *(volatile uintptr_t*)slot;
    if (dp >= 0x10000u) {
      if (d6 & 1u) {                                                     // BP0: BEFORE the cipher call -> plaintext
        if (InterlockedCompareExchange(&g_cip_ptr_logged, 1, 0) == 0) {
          uintptr_t fn = (uintptr_t)c->Edx;
          LOGF("shim: RV-CIP cipher fn = 0x%08x (rva 0x%x)  [vtable slot+4 of session+0x618]", (uint32_t)fn, (uint32_t)(fn - g_mod));
        }
        g_pre_dp = dp; g_pre_len = 256;
        for (int i = 0; i < 256; i++) g_pre_buf[i] = ((volatile unsigned char*)dp)[i];
        cip_hex("RV-CIP-PRE", g_pre_buf, 32);
      } else if (dp == g_pre_dp && g_pre_len) {                          // BP1: AFTER the cipher -> ciphertext
        unsigned char ct[64], ks[64];
        for (int i = 0; i < 32; i++) { ct[i] = ((volatile unsigned char*)dp)[i]; ks[i] = (unsigned char)(g_pre_buf[i] ^ ct[i]); }
        cip_hex("RV-CIP-CT", ct, 32);
        cip_hex("RV-CIP-KS", ks, 32);                                    // keystream = plaintext ^ ciphertext
        InterlockedIncrement(&g_cip_pairs);
      }
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) { LOGF("shim: RV-CIP fault"); }
  return EXCEPTION_CONTINUE_EXECUTION;
}

static void __cdecl rv_trace_arm_worker(void* param) {
  // NOTES 28i9: this worker runs on the FUN_1123a870 poller thread. Arm the two HW execute breakpoints on THIS
  // thread once. Try SetThreadContext of the debug registers first (clean, no visible INT3); if the OS rejects it
  // for the current thread, fall back to __debugbreak() so rv_hwbp_veh can write Dr0/Dr7 into the live CONTEXT.
  (void)param;
  if (InterlockedCompareExchange(&g_hwbp_armed, 1, 0) != 0) return;
  int done = 0;
  __try {
    CONTEXT ctx; memset(&ctx, 0, sizeof(ctx)); ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (GetThreadContext(GetCurrentThread(), &ctx)) {
      ctx.Dr0 = (DWORD)(g_mod + RV_CI_PRE);
      ctx.Dr1 = (DWORD)(g_mod + RV_CI_POST);
      ctx.Dr6 = 0;
      ctx.Dr7 = (ctx.Dr7 & ~0x00FF00FFu) | 0x05u;
      if (SetThreadContext(GetCurrentThread(), &ctx)) { done = 1; LOGF("shim: RV-CIP armed via SetThreadContext Dr0=0x%08x Dr1=0x%08x Dr7=0x%x", ctx.Dr0, ctx.Dr1, ctx.Dr7); }
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) { done = 0; }
  if (!done) { LOGF("shim: RV-CIP arming via SetThreadContext failed (err %lu), using INT3 fallback", GetLastError()); __try { __debugbreak(); } __except (EXCEPTION_EXECUTE_HANDLER) {} }
}
__declspec(naked) static void hk_rv_trace31860(void) {
  __asm {
    push ecx                       // ECX = thiscall param_1; also serves as the worker's first argument
    call rv_trace_arm_worker       // arms the two HW breakpoints on the poller thread
    pop ecx                        // restore ECX (thiscall this) for the original
    jmp  dword ptr [g_tr31860]      // tail into the 9-byte trampoline -> original FUN_1123a870 body
  }
}
static void install_rv_trace31860(void) {
  // NOTES 28i9: register the HW-breakpoint handler, then the n=9 entry hook on the on-path send caller FUN_1123a870
  // (the hook only arms the HW BPs on the poller thread). The HW BPs sit on the cipher call (0x0123ab75 PRE) and
  // the instruction right after it returns (0x0123ab77 POST), giving the shim a direct plaintext/ciphertext pair.
  if (!g_hwbp_veh) g_hwbp_veh = AddVectoredExceptionHandler(1, rv_hwbp_veh);
  int rc = install_rv_observer_n(0x0123a870u, (void*)hk_rv_trace31860, &g_tr31860, "rv_trace_a870", 9);
  LOGF("shim: RV-CIP install rc=%d (hook FUN_1123a870 rva 0x0123a870 n=9; HW BP PRE rva 0x0123ab75 + POST rva 0x0123ab77)", rc);
}

// ---- NOTES 28dp: poll the online session-manager singleton DAT_11e42c90 and log on change, to see whether the
// Co-op "connecting to servers" hang is blocked on the manager still being NULL (auth/session never established).
static volatile unsigned g_last_mgr = 0xFFFFFFFFu;
static DWORD WINAPI mgr_watch_thread(LPVOID p) {
  (void)p;
  uintptr_t g = VA(RVA_online_mgr_global);
  for (;;) {
    unsigned v = 0xFFFFFFFFu;
    __try { v = *(volatile unsigned*)g; } __except (EXCEPTION_EXECUTE_HANDLER) { v = 0xFFFFFFFFu; }
    if (v != g_last_mgr) { logLock(); LOGF("shim: MGR-WATCH DAT_11e42c90=0x%08x (%s)", v, v == 0 ? "NULL = no session-manager" : (v == 0xFFFFFFFF ? "unreadable" : "NON-NULL = manager present")); logUnlock(); g_last_mgr = v; }
    Sleep(2000);
  }
  return 0;
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
  if (g_rv_md5) install_rv_md5();               // independent read-only key-derivation observer (NOTES 28fd)
  if (g_rv_enc) install_rv_enc();               // independent read-only plaintext/ciphertext capture (NOTES 28g2)
  if (g_rv_enc) install_rv_pkt();               // upstream Quazal packet-object serialize capture (NOTES 28g5)
  if (g_rv_enc) install_rv_st_enc();            // NOTES 28i2: capture the RV sendto wrapper too (the wrapper 28g8 actually used)
  if (g_rv_enc) install_rv_trace31860();        // NOTES 28i6: dynamic indirect-call tracer (path #2) to find the prudps RC4
  if (g_mgr_watch) { HANDLE mh = CreateThread(0, 0, mgr_watch_thread, 0, 0, 0); LOGF("shim: MGR_WATCH thread started h=%p (watch MGR-WATCH lines while doing Co-op -> Host)", (void*)mh); }
  if (g_watch_login) { install_login_event_watch(); if (g_block_logout) install_logout_block();
    if (g_online_on) { __try { *(volatile unsigned char*)VA(RVA_online_flag_global) = 1; LOGF("shim: ONLINE_ON set DAT_11f07e48=1 (try to enqueue RV connect)"); } __except (EXCEPTION_EXECUTE_HANDLER) { LOGF("shim: ONLINE_ON set fault"); } }
    LOGF("shim: WATCH_LOGIN clean mode - FUN_107db689 observer%s%s; no online flag unless ONLINE_ON, no other hooks", g_block_logout?" + LOGOUT-BLOCK":"", g_online_on?" + ONLINE_ON":""); return; }
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
