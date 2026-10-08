import sys, struct
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
b=Path('analysis/Rozm.exe').read_bytes()
p=struct.unpack_from('<I',b,60)[0]; o=p+24
s=o+struct.unpack_from('<H',b,p+20)[0]
sections=[struct.unpack_from('<8sIIII',b,s+i*40) for i in range(struct.unpack_from('<H',b,p+6)[0])]
def data(va,n):
 for _,size,rva,raw,off in sections:
  if rva<=va-0x400000<rva+size: return b[off+va-0x400000-rva:off+va-0x400000-rva+n]
 return b''
if __name__=='__main__':
 va=int(sys.argv[1],16); n=int(sys.argv[2],0) if len(sys.argv)>2 else 256
 for i in Cs(CS_ARCH_X86,CS_MODE_32).disasm(data(va,n),va): print(f'{i.address:08x} {i.mnemonic:8} {i.op_str}')
