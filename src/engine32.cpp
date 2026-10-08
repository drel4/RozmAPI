// Headless adapter for the 2007 Rozm.exe engine. No PE entry point or VCL startup is executed.
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <cstring>
#include <bcrypt.h>

static BYTE* image;
static std::vector<BYTE>* pcm;
static void* addr(uint32_t va) { return image + va - 0x400000; }
template<class T> static T& at(uint32_t va) { return *static_cast<T*>(addr(va)); }
static void* __cdecl allocMem(unsigned n) { return HeapAlloc(GetProcessHeap(),0,n); }
static int __cdecl freeMem(void* p) { return HeapFree(GetProcessHeap(),0,p) ? 0 : 1; }
static void* __cdecl reallocMem(void* p,unsigned n) { return HeapReAlloc(GetProcessHeap(),0,p,n); }
__declspec(naked) static void getMemThunk() { __asm {
 push eax
 call allocMem
 add esp,4
 ret
} }
__declspec(naked) static void freeMemThunk() { __asm {
 push eax
 call freeMem
 add esp,4
 ret
} }
__declspec(naked) static void reallocMemThunk() { __asm {
 push edx
 push eax
 call reallocMem
 add esp,8
 ret
} }
static void __cdecl capture(unsigned n) {
 if(n>500000 || pcm->size()+n>256*1024*1024) throw std::runtime_error("PCM limit exceeded");
 BYTE* p=(BYTE*)addr(0x46fca4); pcm->insert(pcm->end(),p,p+n);
}
__declspec(naked) static void captureThunk() { __asm {
 pushad
 push dword ptr [ebp-0ch]
 call capture
 add esp,4
 popad
 ret
} }
static uintptr_t call(uint32_t va,uintptr_t a=0,uintptr_t d=0,uintptr_t c=0) {
 void* fn=addr(va); uintptr_t result;
 __asm {
 mov eax,a
 mov edx,d
 mov ecx,c
 call fn
 mov result,eax
 }
 return result;
}
static std::vector<BYTE> read(const std::wstring& path) {
 std::ifstream f(path.c_str(),std::ios::binary); if(!f) throw std::runtime_error("Cannot open engine/data file");
 f.seekg(0,std::ios::end); auto size=f.tellg(); if(size<0 || size>64*1024*1024) throw std::runtime_error("Invalid data file size");
 f.seekg(0); std::vector<BYTE> b((size_t)size); if(!b.empty()&&!f.read((char*)b.data(),b.size())) throw std::runtime_error("Cannot read engine/data file"); return b;
}
static void loadData(const std::wstring& root,const wchar_t* file,uint32_t va,size_t capacity) {
 auto b=read(root+L"\\"+file); if(b.size()>capacity) throw std::runtime_error("Oversized engine data"); memcpy(addr(va),b.data(),b.size());
}
static char* str(const std::string& s) {
 char* p=(char*)allocMem((unsigned)s.size()+9); if(!p) throw std::bad_alloc();
 ((int*)p)[0]=1; ((int*)p)[1]=(int)s.size(); memcpy(p+8,s.c_str(),s.size()+1); return p+8;
}
static void clear(char*& s) { call(0x40494c,(uintptr_t)&s); }
static void mapEngine(const std::wstring& root) {
 auto b=read(root+L"\\Rozm.exe");
 if(b.size()!=520704) throw std::runtime_error("Unsupported Rozm.exe build");
 const BYTE expected[32]={0x90,0xe2,0x6b,0x39,0xa4,0xca,0x51,0x8d,0xf3,0xb1,0xeb,0xcf,0x4b,0xba,0xf9,0x0a,0x85,0x43,0xed,0x90,0xa3,0x12,0x73,0x24,0x1d,0x58,0x0d,0x22,0xb3,0x9e,0x1a,0x54};
 BCRYPT_ALG_HANDLE alg=NULL; BYTE digest[32];
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,NULL,0)<0) throw std::runtime_error("Cannot open SHA256 provider");
 NTSTATUS hashed=BCryptHash(alg,NULL,0,b.data(),(ULONG)b.size(),digest,sizeof digest); BCryptCloseAlgorithmProvider(alg,0);
 if(hashed<0||memcmp(digest,expected,sizeof digest)) throw std::runtime_error("Unsupported Rozm.exe SHA256; this adapter requires the July 27 2007 build");
 auto dos=(IMAGE_DOS_HEADER*)b.data(); auto nt=(IMAGE_NT_HEADERS32*)(b.data()+dos->e_lfanew);
 if(dos->e_magic!=IMAGE_DOS_SIGNATURE || nt->Signature!=IMAGE_NT_SIGNATURE || nt->OptionalHeader.ImageBase!=0x400000 || nt->OptionalHeader.AddressOfEntryPoint!=0x6c7a8) throw std::runtime_error("Unsupported engine image");
 image=(BYTE*)VirtualAlloc(NULL,nt->OptionalHeader.SizeOfImage,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
 if(!image) throw std::bad_alloc(); memcpy(image,b.data(),nt->OptionalHeader.SizeOfHeaders);
 auto sec=IMAGE_FIRST_SECTION(nt);
 for(unsigned i=0;i<nt->FileHeader.NumberOfSections;i++) memcpy(image+sec[i].VirtualAddress,b.data()+sec[i].PointerToRawData,sec[i].SizeOfRawData);
 intptr_t delta=(intptr_t)image-0x400000;
 auto rd=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
 for(unsigned offset=0;offset<rd.Size;) {
  auto r=(IMAGE_BASE_RELOCATION*)(image+rd.VirtualAddress+offset); if(r->SizeOfBlock<8) throw std::runtime_error("Bad relocations");
  auto entries=(WORD*)(r+1); for(unsigned j=0;j<(r->SizeOfBlock-8)/2;j++) {
   if(entries[j]>>12==IMAGE_REL_BASED_HIGHLOW) *(DWORD*)(image+r->VirtualAddress+(entries[j]&0xfff))+=(DWORD)delta;
  } offset+=r->SizeOfBlock;
 }
 auto id=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
 auto imp=(IMAGE_IMPORT_DESCRIPTOR*)(image+id.VirtualAddress);
 for(;imp->Name;imp++) {
  HMODULE lib=LoadLibraryA((char*)(image+imp->Name)); if(!lib) throw std::runtime_error("Missing Windows import");
  auto src=(DWORD*)(image+(imp->OriginalFirstThunk?imp->OriginalFirstThunk:imp->FirstThunk)); auto dst=(DWORD*)(image+imp->FirstThunk);
  for(;*src;src++,dst++) { auto proc=GetProcAddress(lib,IMAGE_SNAP_BY_ORDINAL32(*src)?(char*)IMAGE_ORDINAL32(*src):(char*)(image+*src+2)); if(!proc) throw std::runtime_error("Missing imported function"); *dst=(DWORD)proc; }
 }
 at<void*>(0x46d040)=getMemThunk; at<void*>(0x46d044)=freeMemThunk; at<void*>(0x46d048)=reallocMemThunk;
 // Replace both audio-device and Delphi file-output branches with our PCM callback.
 BYTE* patch=(BYTE*)addr(0x4674f2); patch[0]=0xe8; *(int32_t*)(patch+1)=(int32_t)((BYTE*)captureThunk-patch-5);
 patch[5]=0xe9; *(int32_t*)(patch+6)=(int32_t)((BYTE*)addr(0x46768f)-patch-10);
 loadData(root,L"BSNbn",0x7ee2f4,0x1b00ff8-0x7ee2f4);
 auto bn=read(root+L"\\BSNbn"); at<int>(0x1b00ff8)=(int)bn.size(); at<int>(0x1b00ffc)=0;
 unsigned count=1; for(unsigned i=1;i<bn.size();i++) if(bn[i]>0x25&&bn[i]<0x51) at<int>(0x1b00ff8+(++count)*4)=i+1;
 at<int>(0x4ec204)=count-1;
 loadData(root,L"skfs\\skf",0x67c860,0x770aa4-0x67c860); at<int>(0x4ec218)=(int)read(root+L"\\skfs\\skf").size();
 loadData(root,L"skfs\\wif",0x770aa4,0x7d2528-0x770aa4); at<int>(0x4ec200)=at<int>(0x770aa4);
 loadData(root,L"skfs\\wlf",0x7d2528,0x7eabcc-0x7d2528);
 auto dic=read(root+L"\\EUtDic.txt"); std::istringstream lines(std::string((char*)dic.data(),dic.size())); std::string line; count=0;
 while(std::getline(lines,line)) { if(!line.empty()&&line.back()=='\r') line.pop_back(); if(++count>110000) throw std::runtime_error("Oversized dictionary"); at<char*>(0x4ec218+count*4)=str(line); }
 at<int>(0x5578dc)=count;
 for(int i=0;i<17;i++) {at<double>(0x4ec0f0+i*8)=i/16.0; at<double>(0x4ec178+i*8)=i/16.0;}
 // Only the original CODE section becomes executable. Mutable strings/constants remain writable.
 DWORD old; if(!VirtualProtect(image+0x1000,440440,PAGE_EXECUTE_READ,&old)) throw std::runtime_error("Cannot enable engine code");
 FlushInstructionCache(GetCurrentProcess(),image,nt->OptionalHeader.SizeOfImage);
}
static std::vector<BYTE> synth(const std::wstring& root,const std::string& text,int voice,int speed) {
 mapEngine(root);
 wchar_t sub[32]; swprintf_s(sub,L"snf%d\\Sd2f",voice); loadData(root,sub,0x217d27c,0x2365700-0x217d27c);
 swprintf_s(sub,L"snf%d\\ip2f",voice); loadData(root,sub,0x7eabd4,0x7ec764-0x7eabd4);
 swprintf_s(sub,L"snf%d\\lp2f",voice); loadData(root,sub,0x7ec764,0x7ee2f4-0x7ec764);
 at<char>(0x46e6e8)=(char)('0'+voice); at<int>(0x46e6e4)=speed;
 std::vector<BYTE> out; pcm=&out;
 std::string sentence;
 for(size_t i=0;i<=text.size();i++) {
  unsigned char ch=i==text.size()?'.':text[i]; if(ch=='\n'||ch=='\r') ch='.';
  sentence+=(char)ch;
  bool boundary=(at<BYTE>(0x46dff0+ch/8)&(1<<(ch%8)))!=0;
  if(!boundary&&i<text.size()&&sentence.size()<160&&!(ch==' '&&sentence.size()>=120)) continue;
  bool meaningful=false; for(unsigned char x:sentence) if(x>32 && x!='.' && x!=',' && x!='!' && x!='?' && x!=';' && x!=':') meaningful=true;
  if(meaningful) {
   sentence+=' '; char* s=str(sentence); char* tmp=NULL;
   const uint32_t stages[]={0x465888,0x4659dc,0x465e9c,0x46621c};
   for(auto stage:stages) {call(stage,(uintptr_t)s,(uintptr_t)&tmp); clear(s); s=tmp; tmp=NULL;}
   if(s&&((int*)s)[-1]>1) {
    call(0x465210,(uintptr_t)&s);
    call(0x46662c,(uintptr_t)s,(uintptr_t)&tmp); clear(s); s=tmp; tmp=NULL;
    call(0x466e7c,(uintptr_t)s);
   } clear(s);
  } sentence.clear();
 }
 return out;
}

#ifdef ROZM_PROBE
int wmain(int argc,wchar_t** argv) {
 try {
  std::string text="\xcf\xf0\xe8\xe2\xb3\xf2, \xf1\xe2\xb3\xf2\xe5!";
  auto p=synth(L"C:\\Program Files (x86)\\\u0420\u043e\u0437\u043c\u043e\u0432\u043b\u044f\u043b\u044c\u043a\u0430",text,argc>1?_wtoi(argv[1]):1,argc>2?_wtoi(argv[2]):5);
  printf("PCM bytes: %zu\n",p.size()); std::ofstream f("probe.pcm",std::ios::binary); f.write((char*)p.data(),p.size());
 }catch(const std::exception& e) {fprintf(stderr,"%s\n",e.what());return 1;} return 0;
}
#endif
