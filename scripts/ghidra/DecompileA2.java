// DecompileA2.java - A2 de-risk spike (Ghidra 12.1 postScript).
// Content-based: locate PRUDP/RV/session-key strings, take cross-references to them, and
// decompile the referencing functions (connect handshake + GameSessionKey/KDF + ticket).
// Output -> H:/FarCryCoop/scratch/ghidra/a2_spike.txt
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.data.*;
import java.io.*;
import java.util.*;

public class DecompileA2 extends GhidraScript {
  String[] NEEDLES = {
    "prudp:/address", "prudps:/address", "prudp:", "prudps:",
    "GameSessionKey", "m_sessionKey", "RequestTicketSessionRefId",
    "SessionTicketRequestAttachment", "KEY_DECRYPTION", "INVALIDSESSIONKEY",
    "MagicBytes", "magicbytes", "connectKey", "ConnectKey",
    "InitConnection", "ConnectionRequest", "PRUDP", "StreamID", "stream="
  };
  int MAXFUNCS = 14;

  public void run() throws Exception {
    DecompInterface decomp = new DecompInterface();
    decomp.openProgram(currentProgram);
    PrintStream out = new PrintStream(new FileOutputStream("H:/FarCryCoop/scratch/ghidra/a2_spike.txt"));
    AddressSpace sp = currentProgram.getAddressFactory().getDefaultAddressSpace();
    Address start = sp.getAddress(0x10000000L);
    Address end   = sp.getAddress(0x12C00000L);
    AddressSet set = new AddressSet();
    set.add(new AddressRangeImpl(start, end));
    AddressSetView whole = set;

    out.println("### A2 spike: string-anchored cross-reference decompilation (FC3.dll 1.05, base 0x10000000)");
    // 1) find defined data whose content matches a needle
    LinkedHashMap<String,ArrayList<Address>> hits = new LinkedHashMap<>();
    for (Data d : currentProgram.getListing().getData(whole, true)) {
      Object v;
      try { v = d.getValue(); } catch (Exception e) { continue; }
      if (!(v instanceof String)) continue;
      String s = (String) v;
      for (String n : NEEDLES) {
        if (s.contains(n)) {
          ArrayList<Address> arr = hits.get(n);
          if (arr == null) { arr = new ArrayList<>(); hits.put(n, arr); }
          arr.add(d.getAddress());
          break;
        }
      }
    }
    out.println("\n### string hits (needle -> count, addresses):");
    for (Map.Entry<String,ArrayList<Address>> e : hits.entrySet()) {
      ArrayList<Address> a = e.getValue();
      StringBuilder sb = new StringBuilder();
      for (int i = 0; i < Math.min(4, a.size()); i++) sb.append(a.get(i)).append(" ");
      out.println("  " + e.getKey() + " : " + a.size() + "  [" + sb + "]");
    }

    // 2) gather functions referencing those data addresses
    LinkedHashSet<Function> funcs = new LinkedHashSet<>();
    for (ArrayList<Address> addrs : hits.values()) {
      for (Address a : addrs) {
        Reference[] refs = getReferencesTo(a);
        for (Reference r : refs) {
          Function f = getFunctionContaining(r.getFromAddress());
          if (f != null) funcs.add(f);
        }
      }
    }
    out.println("\n### functions referencing those strings: " + funcs.size() + " (decompiling up to " + MAXFUNCS + ")");

    int idx = 0;
    for (Function f : funcs) {
      if (idx++ >= MAXFUNCS) break;
      out.println("\n===================== FUNC " + idx + "  " + f.getEntryPoint() + "  " + f.getName() + " =====================");
      try {
        DecompileResults dr = decomp.decompileFunction(f, 120, monitor);
        if (dr != null && dr.decompileCompleted()) {
          out.println(dr.getDecompiledFunction().getC());
        } else {
          out.println("(decompile failed) " + (dr != null ? dr.getErrorMessage() : "null"));
        }
      } catch (Exception ex) { out.println("(ex) " + ex.getMessage()); }
    }
    out.flush();
    out.close();
    println("A2 spike done, funcs referenced=" + funcs.size());
  }
}
