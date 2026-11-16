import pefile
uplay=r"H:/FarCryCoop/Downloads/patch_solo_offline_coop_vanilla-262/uplay_r14.dll"
game=r"H:/FarCryCoop/TestFC3_105/bin/FC3_d3d11.dll"
def info(fp,label):
    pe=pefile.PE(fp)
    mach=hex(pe.FILE_HEADER.Machine)
    is32 = pe.OPTIONAL_HEADER.Magic==0x10b
    print(f"\n===== {label} =====")
    print("machine",mach,"32bit" if is32 else "64bit","ImageBase",hex(pe.OPTIONAL_IMAGE_BASE if hasattr(pe,'OPTIONAL_IMAGE_BASE') else pe.OPTIONAL_HEADER.ImageBase))
    return pe
pu=info(uplay,"uplay_r14.dll")
pu.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_EXPORT']])
exports={}
for e in pu.DIRECTORY_ENTRY_EXPORT.symbols:
    if e.name: exports[e.name.decode(errors="replace")] = e.address  # RVA
print("total exports",len(exports))
def show(filt):
    for n,a in sorted(exports.items()):
        if filt(n): print(f"   RVA 0x{a:08x}  {n}")
print("\n-- target surface (GetNextEvent / Init / PARTY / GameSession / FRIENDS) --")
for k in ["GetNextEvent","UPLAY_Init","UPLAY_PARTY_","GameSession","UPLAY_FRIENDS_","UPLAY_USER","UPLAY_OPTIONS"]:
    show(lambda n,k=k: k in n)

pg=info(game,"FC3_d3d11.dll (the game)")
pg.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_IMPORT']])
print("\n-- game imports: DLLs --")
for d in pg.DIRECTORY_ENTRY_IMPORT:
    print("  ",d.dll.decode(errors="replace"))
print("\n-- game imports FROM uplay_r14 / uplay_r1 / ubiorbit / any UPLAY_* --")
for d in pg.DIRECTORY_ENTRY_IMPORT:
    dlln=d.dll.decode(errors="replace").lower()
    if "uplay" in dlln or "ubiorbit" in dlln or "upc" in dlln:
        print(f"  [{d.dll.decode(errors='replace')}] imports:")
        for im in d.imports:
            if im.name: print("     ",im.name.decode(errors="replace"))
