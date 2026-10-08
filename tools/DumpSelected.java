import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.io.*;
public class DumpSelected extends GhidraScript {
 public void run() throws Exception {
  String[] args=getScriptArgs(); DecompInterface d=new DecompInterface(); d.openProgram(currentProgram);
  PrintWriter out=new PrintWriter(args[0]);
  for(int i=1;i<args.length;i++) {
   var a=toAddr(args[i]); Function f=getFunctionAt(a);
   if(f==null) { disassemble(a); f=createFunction(a,null); }
   out.println("// ADDRESS "+a);
   if(f!=null) {var r=d.decompileFunction(f,60,monitor); if(r.decompileCompleted()) out.println(r.getDecompiledFunction().getC());}
  } out.close(); d.dispose();
 }
}
