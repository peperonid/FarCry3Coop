import pefile, re, sys
fp=r"H:/FarCryCoop/Downloads/patch_solo_offline_coop_vanilla-262/uplay_r14.dll"
pe=pefile.PE(fp, fast_load=False)
pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_IMPORT'],
                                        pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_EXPORT']])
print("== EXPORTS (what uplay_r14.dll provides to the game/launcher) ==")
if hasattr(pe,"DIRECTORY_ENTRY_EXPORT"):
    exps=[(e.ordinal, e.name.decode(errors="replace") if e.name else None) for e in pe.DIRECTORY_ENTRY_EXPORT.symbols]
    print("count",len(exps))
    for o,n in exps[:60]: print("   ",o,n)
print("\n== IMPORTS (DLLs it pulls in) ==")
if hasattr(pe,"DIRECTORY_ENTRY_IMPORT"):
    for d in pe.DIRECTORY_ENTRY_IMPORT:
        name=d.dll.decode(errors="replace")
        print(f"   {name}  ({len(d.imports)} funcs)")
    impdlls=sorted({d.dll.decode(errors='replace').lower() for d in pe.DIRECTORY_ENTRY_IMPORT})
    print("   distinct:",impdlls)
# strings scan
data=open(fp,"rb").read()
def ascii_strings(b,minlen=6):
    out=[]; cur=bytearray()
    for c in b:
        if 32<=c<127: cur.append(c)
        else:
            if len(cur)>=minlen: out.append(cur.decode('ascii','replace'))
            cur=bytearray()
    return out
strs=ascii_strings(data)
pats=["gconnect","agora","passport","LoginStatus","LoggingIn","RendezVous","GameSessionKey",
      "Uplay","uplay","upc","Ubisoft","connect","login","auth","ticket","Session","autologin",
      ".dll",".exe","http","https","://","key","Key"]
hits={}
for s in strs:
    for p in ["gconnect","agora","passport","LoginStatus","RendezVous","GameSessionKey","autologin",
              "InitiateLogin","LoggingIn","UbiAuthentication","gator","GATOR","login","Login"]:
        if p in s:
            hits.setdefault(p,[]).append(s)
for p in ["gconnect","agora","passport","LoginStatus","RendezVous","GameSessionKey","autologin",
          "InitiateLogin","LoggingIn","UbiAuthentication","gator","GATOR","login","Login"]:
    arr=hits.get(p,[])
    if arr:
        uniq=sorted(set(arr))
        print(f"\n[{p}] {len(arr)} hits, {len(uniq)} uniq (first 12):")
        for u in uniq[:12]: print("   ",u[:160])
