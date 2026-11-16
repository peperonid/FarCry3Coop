// DecompileA2b.java - A2 spike pass 2: decompile the functions that USE the RV connect format
// string and the session-key member, and list every call they make (to spot RC4/OpenSSL/KDF).
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.util.*;

public class DecompileA2b extends GhidraScript {
  // data addresses of the FORMAT / key strings (FC3.dll 1.05 base 0x10000000), from pass 1:
  long[] DATA = {
    0x11671dc8L, // "prudps:/address=%s;port=%d;..."
    0x11671fd4L, // "prudp:/address..."
    0x116718e4L, 0x11671934L, // stream= format
    0x117439e7L, 0x11743c9aL, 0x11743de9L, 0x11743e0aL, 0x11743ffbL, 0x1174410cL, // GameSessionKey / m_sessionKey
    0x11692b14L, // INVALIDSESSIONKEY
    0x116947b8L  // KEY_DECRYPTION
  };

  public void run() throws Exception {
    DecompInterface decomp = new DecompInterface();
    decomp.openProgram(currentProgram);
    PrintStream out = new PrintStream(new FileOutputStream("H:/FarCryCoop/scratch/ghidra/a2_spike2.txt"));
    AddressSpace sp = currentProgram.getAddressFactory().getDefaultAddressSpace();
    LinkedHashSet<Function> funcs = new LinkedHashSet<>();
    for (long da : DATA) {
      Address a = sp.getAddress(String.format("0x%x", da));
      Reference[] refs = getReferencesTo(a);
      for (Reference r : refs) {
        Function f = getFunctionContaining(r.getFromAddress());
        if (f != null) funcs.add(f);
      }
    }
    out.println("### pass2: functions to decompile = " + funcs.size());
    int idx = 0;
    for (Function f : funcs) {
      if (idx++ >= 10) break;
      out.println("\n===================== FUNC " + idx + "  " + f.getEntryPoint() + "  " + f.getName() + " =====================");
      // list its external/global calls
      out.println("-- calls --");
      for (Instruction instr : currentProgram.getListing().getInstructions(f.getBody(), true)) {
        for (Reference ref : getReferencesFrom(instr.getAddress())) {
          if (ref.getReferenceType().isCall()) {
            Function cf = getFunctionAt(ref.getToAddress());
            String nm = (cf != null ? cf.getName() : String.valueOf(ref.getToAddress()));
            out.println("   call " + nm);
          }
        }
      }
      try {
        DecompileResults dr = decomp.decompileFunction(f, 120, monitor);
        if (dr != null && dr.decompileCompleted()) out.println(dr.getDecompiledFunction().getC());
        else out.println("(decompile failed)");
      } catch (Exception ex) { out.println("(ex) " + ex.getMessage()); }
    }
    out.flush(); out.close();
    println("A2 pass2 done funcs=" + funcs.size());
  }
}
