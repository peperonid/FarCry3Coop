/*
 * fc3_h3_trace.js — H3 runtime trace for the FC3 co-op mode switch (Frida).
 *
 * Implements the Phase-2 "H3" lead in PLAN.md ("Hook status after sessions 5-6"):
 * the offline global pokes are exhausted; the live mode is the mode-manager field
 * [this+0x4c] and the live co-op session is a PrivateCoopPlaylist entry in the
 * CFCXPlaylistService container [service+0x8c0]. This script OBSERVES those at
 * runtime (read + log) and optionally lets you poke [this+0x4c] by hand.
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
 */
'use strict';

var MODULE = 'FC3.dll';
var IMAGE_BASE = 0x10000000; // FC3.dll preferred/actual base; A() re-anchors to the real one

// --- 1.06 anchors (NOTES.md session 5-6). Map to 1.05 before use. ---
var ADDR = {
  dispatchMode:  0x113405a0, // mov eax,[ebx+0x4c]; 6-way cmp-chain; ebx = mode-manager this
  installer:     0x11279910, // co-op-id installer (thiscall, this in ecx); static callers 0x1127a0f0 / 0x11279fc0
  callerA:       0x1127a0f0, // one of the two static callers of the installer (playlist-service method)
  callerB:       0x11279fc0, // the other static caller
  svcSingleton:  0x12863890, // CFCXPlaylistService instance global (guard [0x1286388c])
  svcGetter:     0x103d1640, // singleton getter (NOT called here; we read the global instead)
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

var lastMode = { at: 0, thisPtr: 0, id: 0, idName: '' };
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
    var svc = A(ADDR.svcSingleton).readPointer();
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
  }
};

console.log('fc3_h3_trace loaded. base=' + base + ' module=' + MODULE +
  ' ; rpc: dumpService(), lastMode(), setModeId(addr,id)');
