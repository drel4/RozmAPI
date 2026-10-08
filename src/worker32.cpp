#include "protocol.h"
#include "engine32.cpp"
int wmain(int argc,wchar_t** argv) {
 if(argc!=4) return 2;
 // Failures in the legacy engine stay inside this short-lived helper process.
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
 try {
  auto req=readFile(argv[1],2*1024*1024);
  if(req.size()<sizeof(RequestHeader)) throw std::runtime_error("Invalid request");
  RequestHeader h; memcpy(&h,req.data(),sizeof h);
  if(h.magic!=ROZM_MAGIC||h.voice<1||h.voice>3||h.speed<1||h.speed>9||h.path_chars>32767||h.text_chars>262144||req.size()!=sizeof(h)+2ull*(h.path_chars+h.text_chars)) throw std::runtime_error("Invalid request");
  auto data=(const wchar_t*)(req.data()+sizeof h);
  std::wstring root(data,h.path_chars),wide(data+h.path_chars,h.text_chars);
  BOOL lossy=FALSE;
  int n=WideCharToMultiByte(1251,WC_NO_BEST_FIT_CHARS,wide.c_str(),(int)wide.size(),NULL,0,NULL,&lossy);
  std::string text(n,'\0');
  if(n) WideCharToMultiByte(1251,WC_NO_BEST_FIT_CHARS,wide.c_str(),(int)wide.size(),&text[0],n,NULL,&lossy);
  if(lossy) throw std::runtime_error("Text contains characters not supported by the CP1251 engine");
  // Typographic punctuation supported by CP1251 is normalized for the legacy lexer.
  for(char& x:text) {unsigned char ch=x; if(ch==0x92||ch==0x91) x='\''; if(ch==0x96||ch==0x97) x='-';}
  auto p=synth(root,text,h.voice,h.speed); writeFile(argv[2],p.data(),p.size()); return 0;
 }catch(const std::exception& e) {
  try {writeFile(argv[3],e.what(),strlen(e.what()));} catch(...) {} return 1;
 }
}
