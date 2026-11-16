import re
fp=r"H:/FarCryCoop/Downloads/patch_solo_offline_coop_vanilla-262/uplay_r14.dll"
data=open(fp,"rb").read()
def ascii_strings(b,minlen=5):
    out=[]; cur=bytearray()
    for c in b:
        if 32<=c<127: cur.append(c)
        else:
            if len(cur)>=minlen: out.append(cur.decode('ascii','replace'))
            cur=bytearray()
    return out
def utf16_strings(b,minlen=4):
    out=[]; cur=[]
    i=0
    while i+1<len(b):
        lo=b[i]; hi=b[i+1]
        if 32<=lo<127 and hi==0: cur.append(chr(lo)); i+=2
        else:
            if len(cur)>=minlen: out.append("".join(cur))
            cur=[]; i+= (1 if cur else 1)
            # simpler: just advance
    return out
asc=ascii_strings(data)
# URLs / hosts / endpoints
urlp=re.compile(r'(https?://|\.ubi\.com|\.ubisoft|\.uplay|:443|:80\b|ws://|wss://)')
print("== URL/HOST-ish ASCII strings ==")
u=sorted({s for s in asc if urlp.search(s)})
for s in u[:60]: print("   ",s[:180])
print("count",len(u))
# words of interest
print("\n== interest ASCII (session/party/multiplayer/player/lobby/room/invite/net) ==")
kw=["Session","PARTY","Party","Multiplayer","MULTIPLAYER","Player","PLAYER","Lobby","LOBBY","Room","ROOM","Invite","INVITE","Match","MATCH","Host","HOST","join","Join","JOIN","online","Online","ONLINE","GameSession","GetNextEvent","WINHTTP","Http","WinHttp"]
seen=set()
for p in kw:
    for s in asc:
        if p in s and (p,s) not in seen:
            seen.add((p,s))
            if len(seen)<200 and len(s)<160:
                print(f"   [{p}] {s}")
# UTF-16 wide URLs/hosts
wide=[s for s in utf16_strings(data) if urlp.search(s) or "ubi" in s.lower()]
print("\n== UTF-16 URL/host-ish ==")
for s in sorted(set(wide))[:60]: print("   ",s[:180])
# UPLAY export names mentioning session/party/friend already known; dump remaining exports 60..136 for multiplayer surface
import pefile
pe=pefile.PE(fp); pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_EXPORT']])
print("\n== remaining exports 60.. ==")
if hasattr(pe,"DIRECTORY_ENTRY_EXPORT"):
    for e in pe.DIRECTORY_ENTRY_EXPORT.symbols[59:]:
        print("   ", e.name.decode(errors="replace") if e.name else e.ordinal)
