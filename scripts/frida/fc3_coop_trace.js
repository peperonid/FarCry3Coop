// fc3_coop_trace.js - Frida: trace Far Cry 3 co-op service connection (direction B recon).
// Attach to farcry3_d3d11.exe (launched via Ubisoft Connect), then navigate main menu -> co-op.
// Core = winsock hooks (module-independent): reveal the co-op service host:port + connect/DNS failures.
// Best-effort = FC3.dll gate offsets (may not resolve if the running build differs).
'use strict';
// Use send() (guaranteed delivered to the python host via on_message 'send'); console.log is not captured there.
var log = function(){ var a=Array.prototype.slice.call(arguments).join(' '); try { send(a); } catch(e) { console.log(a); } };

// hex of first n bytes at p (guarded).
function hex(p, n){ try { var b=p.readByteArray(n); if(!b) return ''; var u=new Uint8Array(b), s=''; for(var i=0;i<u.length;i++){ s+=(u[i]<16?'0':'')+u[i].toString(16); } return s; } catch(e){ return '<bad>'; } }

// ---------- winsock ----------
function sockaddrInfo(p){
  try {
    var fam = p.readU16();
    if (fam !== 2 /*AF_INET*/) return {fam: fam, s: 'fam='+fam};
    var port = (p.add(2).readU8()<<8) | p.add(3).readU8();   // network order -> host
    var ipBytes = p.add(4).readByteArray(4);
    var b = new Uint8Array(ipBytes);
    return {fam: fam, s: b[0]+'.'+b[1]+'.'+b[2]+'.'+b[3]+':'+port};
  } catch(e){ return {s:'<bad sockaddr>'}; }
}
['ws2_32.dll','ws2_32'].forEach(function(mod){
  var m = Process.findModuleByName(mod); if(!m) return;
  var ghn = m.findExportByName('gethostbyname');
  if (ghn) Interceptor.attach(ghn, { onEnter:function(args){ this.n = args[0].readCString(); },
    onLeave:function(r){ var hosts = r.isNull()?'-':''; if(!r.isNull()){ try{ var hn=r.add(0).readPointer(); hosts = r.add(0).readPointer().readCString(); }catch(e){} }
      log('[DNS] gethostbyname("' + this.n + '") -> ' + (r.isNull()?'NULL(fail)':('hostent '+r+' name='+hosts))); } });
  var gai = m.findExportByName('getaddrinfo');
  if (gai) Interceptor.attach(gai, { onEnter:function(args){ this.node = args[0].isNull()?'null':args[0].readCString(); this.svc = args[1].isNull()?'null':args[1].readCString(); },
    onLeave:function(r){ log('[DNS] getaddrinfo(node="'+this.node+'", svc="'+this.svc+'") -> ret='+r); } });
  var con = m.findExportByName('connect');
  if (con) Interceptor.attach(con, { onEnter:function(args){ var si = sockaddrInfo(args[1]); this.si=si.s; },
    onLeave:function(r){ log('[NET] connect(fd, '+this.si+') -> '+r.toInt32()+(r.toInt32()===0?' OK':' (WSAErro r '+(r.toInt32()<0?(-r.toInt32()):r.toInt32())+')')); } });
  var sto = m.findExportByName('sendto');
  if (sto) Interceptor.attach(sto, { onEnter:function(args){ this.dst = args[4].isNull()?'<none/connected>':sockaddrInfo(args[4]).s; this.len=args[2].toInt32(); this.buf=args[1]; },
    onLeave:function(r){ if(this.len>0) log('[NET] sendto(fd, len='+this.len+', dst='+this.dst+') -> '+r.toInt32()+' data='+hex(this.buf, Math.min(this.len,64))); } });
  var rfo = m.findExportByName('recvfrom');
  if (rfo) Interceptor.attach(rfo, { onEnter:function(args){ this.src = args[4].isNull()?'<none>':sockaddrInfo(args[4]).s; this.buf=args[1]; },
    onLeave:function(r){ var n=r.toInt32(); if(n>0) log('[NET] recvfrom(fd, n='+n+', src='+this.src+') data='+hex(this.buf, Math.min(n,64))); } });
  var bnd = m.findExportByName('bind');
  if (bnd) Interceptor.attach(bnd, { onEnter:function(args){ log('[NET] bind(fd, '+sockaddrInfo(args[1]).s+')'); } });
  log('winsock hooks installed on '+mod);
});

// ---------- FC3.dll gate offsets (best-effort) ----------
function tryGate(){
  var names = ['FC3.dll'];
  for (var i=0;i<names.length;i++){
    var m = Process.findModuleByName(names[i]); if(!m) continue;
    var base = m.base;
    // RVA = VA - 0x10000000 (image base)
    var offs = { 'FUN_11218480_joinapprove':0x1218480, 'FUN_102000d0_connected':0x02000d0,
                 'FUN_103660e0_partysvc':0x03660e0, 'FUN_11273650_trustedlist':0x1273650 };
    Object.keys(offs).forEach(function(k){
      try { Interceptor.attach(base.add(offs[k]), { onEnter:function(args){ this.a0=args[0]; },
        onLeave:function(r){ log('[FC3] '+k+'(a0='+this.a0+') -> '+r); } }); }
      catch(e){ log('[FC3] hook '+k+' failed: '+e.message); }
    });
    try {
      var g = base.add(0x0285180).readPointer(); // DAT_12805180
      log('[FC3] DAT_12805180 = '+g+'  (mgr would be '+(g.isNull()?'null':g.add(4).readPointer())+')');
    } catch(e){}
    log('FC3.dll gate hooks attempted, base='+base);
  }
}
setTimeout(tryGate, 1500);
log('fc3_coop_trace loaded. Navigate to co-op in-game now.');
