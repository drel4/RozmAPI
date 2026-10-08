import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.io.*;
public class DumpRozm extends GhidraScript {
 public void run() throws Exception {
  DecompInterface d=new DecompInterface(); d.openProgram(currentProgram);
  PrintWriter out=new PrintWriter(new File(getScriptArgs()[0]));
  FunctionIterator it=currentProgram.getFunctionManager().getFunctions(true);
  while(it.hasNext()) { Function f=it.next(); if(f.getEntryPoint().getOffset()<0x440000) continue;
   DecompileResults r=d.decompileFunction(f,30,monitor);
   out.println("// ADDRESS "+f.getEntryPoint());
   if(r.decompileCompleted()) out.println(r.getDecompiledFunction().getC());
  } out.close(); d.dispose();
 }
}
