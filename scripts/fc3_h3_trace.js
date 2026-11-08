/*
 * fc3_h3_trace.js — H3 runtime trace for the FC3 co-op mode switch (Frida).
 *
 * Implements the Phase-2 "H3" lead in PLAN.md ("Hook status after session 7"):
 * session-7 decompilation pinned the mechanism. The mode-manager field [this+0x4c]
 * holds the live mode id; the real "switch to mode" call is modeManager->vtable[+0x5c]
 * (inside FUN_11340560); the co-op session lives in the CFCXPlaylistService container
 * [service+0x8c0], populated by FUN_1127a0f0 -> FUN_11279910 (see NOTES.md session 7).
 * This script OBSERVES those at runtime and exposes the two static levers as RPC
 * (switchCoop / populate) for the owner to run against an OFFLINE instance.
 *
 * All addresses below are the 1.06 build (H:\FarCryCoop\TestFC3\bin\FC3.dll,
 * image base 0x10000000). 1.05 is the primary target: re-locate every address
 * with scripts/fc3_addrmap.py before pointing this at a 1.05 process.
 * Anchors verified in NOTES.md:2176-2258 (probes p280-p315).
 *
 * SAFETY: attach only to an OFFLINE game instance (`-offline` debug menu), never
 * to an online / Ubisoft Connect session — frida injection there risks an anti-cheat
 * flag, and the process guard is OpenMutexA("FarCry3Instance"). Read the AGENTS
 * rules: never touch the real install, use the working copy only.
 *
 * Usage (owner's machine, game already running offline):
 *   frida -n FC3 -l scripts/fc3_h3_trace.js        # by image name
 *   frida -p <pid> -l scripts/fc3_h3_trace.js      # by pid
 * In the REPL:
 *   rpc.exports.dumpService()      # read the live playlist-service container
 *   rpc.exports.lastMode()         # last mode-manager this + [this+0x4c] seen
 *   rpc.exports.setModeId(addr, id)  # DANGEROUS experimental poke, see below
 *   rpc.exports.switchCoop(true)   # H3a: dryRun first; enter a menu so lastMode() caches `this`
 *   rpc.exports.switchCoop(false)  # H3a: write [this+0x4c]=CO-OP + call vtable[+0x5c] (may crash)
 *   rpc.exports.populate(true)     # H3b: dryRun, show svcInit address
 *   rpc.exports.populate(false)    # H3b: call FUN_1127a0f0(service) to build the co-op entry (may crash)
 */
'use strict';

var MODULE = 'FC3.dll';
var IMAGE_BASE = 0x10000000; // FC3.dll preferred/actual base; A() re-anchors to the real one

// --- 1.06 anchors (NOTES.md session 5-7). Map to 1.05 before use. ---
var ADDR = {
  dispatchMode:  0x113405a0, // cmp-chain inside FUN_11340560 (body 11340560-113409cd); ebx=this, [this+0x4c]=mode id
  modeSwitchVt:  0x5c,        // *(this->vtable+0x5c)(&id) is the real "switch to mode" virtual (FUN_11340560)
  svcInit:       0x1127a0f0, // FUN_1127a0f0(this): builds [svc+0x8c0] container + runs installer; sets +0x488/+0x4b0..
  containerAddVt:0x5c,        // *(svc+0x8c0)->vtable+0x5c(&obj,id,0) = Add a co-op entry into the container
  byKeyLookup:   0x113224e0, // FUN_113224e0(&id): resolve playlist entries by mode-key (used with 0x43521e6f)
  installer:     0x11279910, // co-op-id installer (thiscall, this in ecx); static callers 0x1127a0f0 / 0x11279fc0
  callerA:       0x1127a0f0, // one of the two static callers of the installer (playlist-service method)
  callerB:       0x11279fc0, // the other static caller
  svcSingleton:  0x12863890, // CFCXPlaylistService: the instance is the object AT this addr (guard 1286388c), not a ptr
  svcGetter:     0x103d1640, // singleton getter: returns &DAT_12863890 directly (NOT called here)
  idMaskReg:     0x128048c0, // id -> mask registry global
  centralObj:    0x12804acc, // central game object (4529 refs; +0x68 component container)
  installG_a:    0x12891ca0, // diagnostic install-global (NOT the live lever; see H1 demotion)
  installG_b:    0x12891ca8
};

// Co-op mode id + the five sibling mode-hash ids seen in the dispatch chain.
var MODE = {
  coop: 0x43521e6f,
  known: [0xab4fc8e2, 0x43521e6f, 0x9991daec, 0x6c2b3868, 0xf66ffb87, 0x2100ced7]
};

var mod = Process.findModuleByName(MODULE);
if (mod === null) { throw new Error(MODULE + ' not loaded; launch the game offline first'); }
var base = mod.base;
function A(va) { return base.add(va - IMAGE_BASE); }

var lastMode = { at: 0, thisPtr: 0, id: 0, idName: '', mmPtr: null };
function nameOf(id) {
  if (id === MODE.coop) return 'CO-OP';
  return '?';
}

// --- Hook 1: the mode dispatch. Reveals the live mode-manager `this` (ebx) and
//     the current mode id [this+0x4c]. This is H3 target #2.
try {
  Interceptor.attach(A(ADDR.dispatchMode), {
    onEnter: function () {
      var ebx = this.context.ebx;
      var id = 0;
      try { id = ebx.add(0x4c).readU32(); } catch (e) {}
      lastMode.at = ADDR.dispatchMode;
      lastMode.thisPtr = ebx.toString();
      lastMode.mmPtr = ebx;
      lastMode.id = id;
      lastMode.idName = nameOf(id);
      console.log('[dispatch] this(ebx)=' + ebx + ' [this+0x4c]=0x' +
        (id >>> 0).toString(16) + ' ' + lastMode.idName);
    }
  });
  console.log('attached dispatch @ ' + A(ADDR.dispatchMode));
} catch (e) { console.log('dispatch hook failed: ' + e); }

// --- Hook 2: the co-op-id installer. Reveals the RUNTIME caller (thiscall this in
//     ecx) and a full backtrace -> H3 target #1 (who sets the live mode / triggers
//     the co-op install). Statically the callers are callerA/callerB.
function installerHook(label) {
  return {
    onEnter: function () {
      var svc = this.context.ecx; // thiscall this (playlist-service object)
      var bt = [];
      try {
        bt = Thread.backtrace(this.context, Backtracer.ACCURATE)
             .map(function (a) {
               var off = a.sub(base); // RVA, comparable to ADDR values
               return a + ' (FC3+0x' + off.toString(16) + ')';
             });
      } catch (e) { bt = ['backtrace failed: ' + e]; }
      var container = 'n/a';
      try { container = svc.add(0x8c0).readPointer() + ' / +' +
        svc.add(0x8c4).readPointer().toString(); } catch (e) {}
      console.log('[' + label + '] installer entered; this(ecx)=' + svc +
        '  [this+0x8c0/+0x8c4]=' + container);
      console.log('  backtrace:\n    ' + bt.join('\n    '));
    }
  };
}
['installer', 'callerA', 'callerB'].forEach(function (k) {
  try {
    Interceptor.attach(A(ADDR[k]), installerHook(k));
    console.log('attached ' + k + ' @ ' + A(ADDR[k]));
  } catch (e) { console.log(k + ' hook failed: ' + e); }
});

// --- RPC: read the live playlist-service container without calling anything.
rpc.exports = {
  dumpService: function () {
    var out = {};
    // FUN_103d1640 returns &DAT_12863890: the singleton instance is the object AT
    // this address (C++ magic-static guarded by _DAT_1286388c), NOT a pointer stored
    // there. So `this` = the address itself; do NOT readPointer() it. (session 7)
    var svc = A(ADDR.svcSingleton);
    out.service = svc.toString();
    if (!svc.isNull()) {
      try {
        out.c_8c0 = svc.add(0x8c0).readPointer().toString();
        out.c_8c4 = svc.add(0x8c4).readPointer().toString();
      } catch (e) { out.readErr = '' + e; }
    }
    out.idMaskReg = A(ADDR.idMaskReg).readPointer().toString();
    out.centralObj = A(ADDR.centralObj).readPointer().toString();
    out.installG_a = '0x' + A(ADDR.installG_a).readU32().toString(16);
    out.installG_b = '0x' + A(ADDR.installG_b).readU32().toString(16);
    return out;
  },
  lastMode: function () { return lastMode; },
  // DANGEROUS, EXPERIMENTAL: write the live mode id [this+0x4c]. Only after the
  // trace confirms `this`; the plan does NOT expect an offline/global poke to
  // switch the mode (H1 demotion), so treat this as a live test, not a fix.
  setModeId: function (thisAddrStr, id) {
    var p = ptr(thisAddrStr).add(0x4c);
    p.writeU32(id >>> 0);
    return { wrote: p.toString(), value: '0x' + (id >>> 0).toString(16),
             nowAt: p.readU32().toString(16) };
  },
  // H3a - switch into co-op. NEEDS the live mode-manager `this` first: enter a
  // menu/lobby once so the dispatch hook caches it (check lastMode() != null),
  // then call switchCoop(false) to write [this+0x4c]=CO-OP and call
  // modeManager->vtable[+0x5c](&id). dryRun=true only shows what it WOULD do.
  // Calling an engine virtual blind can crash the game: run dryRun first.
  switchCoop: function (dryRun) {
    if (!lastMode.mmPtr) return { error: 'no cached mode-manager this; enter a menu so the dispatch hook fires, then check lastMode()' };
    var mm = lastMode.mmPtr;
    var id = MODE.coop;
    var slot = mm.readPointer().add(ADDR.modeSwitchVt).readPointer(); // *(mm->vtable + 0x5c)
    var modeAddr = Memory.alloc(4); modeAddr.writeU32(id >>> 0);
    var cur = mm.add(0x4c).readU32();
    if (dryRun) {
      return { modeManager: mm.toString(), curMode: '0x' + (cur >>> 0).toString(16),
               vtable5c: slot.toString(), argId: '0x' + id.toString(16), dryRun: true };
    }
    mm.add(0x4c).writeU32(id >>> 0);
    var switchFn = new NativeFunction(slot, 'int', ['pointer', 'pointer'], 'thiscall');
    var ret = switchFn(mm, modeAddr); // this=mm (ecx), arg=&id
    return { modeManager: mm.toString(), prevMode: '0x' + (cur >>> 0).toString(16),
             newMode: '0x' + id.toString(16), vtable5c: slot.toString(), ret: ret };
  },
  // H3b - populate the co-op session: call FUN_1127a0f0(service). service `this`
  // is the object AT svcSingleton (0x12863890), not a pointer. dryRun=true only
  // resolves the function address. Same crash caveat as switchCoop.
  populate: function (dryRun) {
    var svc = A(ADDR.svcSingleton);
    var initFn = A(ADDR.svcInit);
    if (dryRun) {
      return { service: svc.toString(), svcInit: initFn.toString(), dryRun: true };
    }
    var f = new NativeFunction(initFn, 'void', ['pointer'], 'thiscall');
    f(svc);
    var c = 'n/a';
    try { c = svc.add(0x8c0).readPointer().toString(); } catch (e) {}
    return { service: svc.toString(), svcInit: initFn.toString(), container8c0: c };
  }
};

console.log('fc3_h3_trace loaded. base=' + base + ' module=' + MODULE +
  ' ; rpc: dumpService(), lastMode(), setModeId(addr,id), switchCoop(dryRun), populate(dryRun)');
