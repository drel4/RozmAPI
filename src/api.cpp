#define ROZM_BUILD
#include "../include/RozmAPI.h"
#include "protocol.h"
#include <cstring>
#include <cstdlib>
#include <new>
struct RozmContext {std::wstring root;};
static HMODULE module;
static thread_local std::string error;
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID) {if(reason==DLL_PROCESS_ATTACH) module=h; return TRUE;}
static int fail(int status,const char* message) {error=message; return status;}
static std::wstring fullPath(const wchar_t* path) {
 DWORD n=GetFullPathNameW(path,0,NULL,NULL); if(!n) throw std::runtime_error("Invalid path");
 std::vector<wchar_t> b(n); if(!GetFullPathNameW(path,n,b.data(),NULL)) throw std::runtime_error("Invalid path"); return b.data();
}
static std::wstring helperPath() {
 std::vector<wchar_t> b(32768); DWORD n=GetModuleFileNameW(module,b.data(),(DWORD)b.size());
 if(!n||n==b.size()) throw std::runtime_error("Cannot locate DLL directory");
 std::wstring s(b.data(),n); return s.substr(0,s.find_last_of(L"\\/")+1)+L"RozmWorker32.exe";
}
struct TempFiles {
 std::wstring request,output,errors;
 TempFiles() {
  wchar_t dir[MAX_PATH],name[MAX_PATH];
  if(!GetTempPathW(MAX_PATH,dir)||!GetTempFileNameW(dir,L"rzm",0,name)) throw std::runtime_error("Cannot create temporary request");
  request=name; output=request+L".pcm"; errors=request+L".err";
 }
 ~TempFiles() {DeleteFileW(request.c_str());DeleteFileW(output.c_str());DeleteFileW(errors.c_str());}
};
extern "C" {
int __cdecl Rozm_CreateW(const wchar_t* installation_directory,RozmHandle* out) {
 error.clear(); if(!out) return fail(ROZM_INVALID_ARGUMENT,"out is null"); *out=NULL;
 try {
  std::wstring root;
  if(installation_directory) {if(!*installation_directory) return fail(ROZM_INVALID_ARGUMENT,"Installation directory is empty"); root=fullPath(installation_directory);}
  else {wchar_t pf[32768]; DWORD n=GetEnvironmentVariableW(L"ProgramFiles(x86)",pf,32768); if(!n||n>=32768) n=GetEnvironmentVariableW(L"ProgramFiles",pf,32768); if(!n||n>=32768) throw std::runtime_error("Cannot locate Program Files"); root=std::wstring(pf)+L"\\\u0420\u043e\u0437\u043c\u043e\u0432\u043b\u044f\u043b\u044c\u043a\u0430";}
  if(GetFileAttributesW((root+L"\\Rozm.exe").c_str())==INVALID_FILE_ATTRIBUTES) return fail(ROZM_IO_ERROR,"Rozm.exe not found in the installation directory");
  if(GetFileAttributesW(helperPath().c_str())==INVALID_FILE_ATTRIBUTES) return fail(ROZM_IO_ERROR,"RozmWorker32.exe must be beside RozmAPI.dll");
  *out=new RozmContext{root}; return ROZM_OK;
 }catch(const std::bad_alloc&) {return fail(ROZM_OUT_OF_MEMORY,"Out of memory");} catch(const std::exception& e) {return fail(ROZM_IO_ERROR,e.what());}
}
void __cdecl Rozm_Destroy(RozmHandle handle) {delete handle;}
void __cdecl Rozm_FreeAudio(RozmAudio* audio) {if(audio) {free(audio->data);memset(audio,0,sizeof(*audio));}}
int __cdecl Rozm_SynthesizeW(RozmHandle handle,const wchar_t* text,int voice,int speed,RozmAudio* out) {
 error.clear();
 if(!out) return fail(ROZM_INVALID_ARGUMENT,"out is null"); memset(out,0,sizeof(*out));
 if(!handle||!text||voice<1||voice>3||speed<1||speed>9) return fail(ROZM_INVALID_ARGUMENT,"Expected context, text, voice 1..3 and speed 1..9");
 try {
  size_t chars=wcsnlen(text,262145); if(chars>262144||handle->root.size()>32767) return fail(ROZM_INVALID_ARGUMENT,"Input exceeds size limit");
  out->sample_rate=11025;out->channels=1;out->bits_per_sample=8;
  if(!chars) return ROZM_OK;
  TempFiles temp; RequestHeader h={ROZM_MAGIC,(uint32_t)voice,(uint32_t)speed,(uint32_t)handle->root.size(),(uint32_t)chars};
  std::vector<uint8_t> request(sizeof h+2*(h.path_chars+h.text_chars)); memcpy(request.data(),&h,sizeof h);
  memcpy(request.data()+sizeof h,handle->root.data(),2*h.path_chars); memcpy(request.data()+sizeof h+2*h.path_chars,text,2*h.text_chars);
  writeFile(temp.request,request.data(),request.size());
  std::wstring helper=helperPath();
  std::wstring cmd=L"\""+helper+L"\" \""+temp.request+L"\" \""+temp.output+L"\" \""+temp.errors+L"\"";
  STARTUPINFOW si={sizeof si}; PROCESS_INFORMATION pi={};
  if(!CreateProcessW(helper.c_str(),&cmd[0],NULL,NULL,FALSE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi)) return fail(ROZM_IO_ERROR,"Cannot start RozmWorker32.exe");
  CloseHandle(pi.hThread); DWORD wait=WaitForSingleObject(pi.hProcess,60000),code=0;
  if(wait!=WAIT_OBJECT_0) {TerminateProcess(pi.hProcess,ROZM_TIMEOUT);WaitForSingleObject(pi.hProcess,5000);CloseHandle(pi.hProcess);return fail(ROZM_TIMEOUT,"Engine worker exceeded 60 seconds");}
  GetExitCodeProcess(pi.hProcess,&code);CloseHandle(pi.hProcess);
  if(code) {
   try {auto b=readFile(temp.errors,65536); error.assign(b.begin(),b.end());} catch(...) {error="Legacy engine worker failed (exit code "+std::to_string(code)+")";}
   return ROZM_ENGINE_ERROR;
  }
  auto b=readFile(temp.output,256*1024*1024); if(!b.empty()) {out->data=(uint8_t*)malloc(b.size());if(!out->data) return fail(ROZM_OUT_OF_MEMORY,"Out of memory");memcpy(out->data,b.data(),b.size());}
  out->size=(uint32_t)b.size(); return ROZM_OK;
 }catch(const std::bad_alloc&) {return fail(ROZM_OUT_OF_MEMORY,"Out of memory");} catch(const std::exception& e) {return fail(ROZM_IO_ERROR,e.what());}
}
int __cdecl Rozm_SynthesizeUtf8(RozmHandle handle,const char* text,int voice,int speed,RozmAudio* out) {
 error.clear(); if(out) memset(out,0,sizeof *out);
 if(!text) return fail(ROZM_INVALID_ARGUMENT,"text is null");
 size_t len=strnlen(text,1048577); if(len>1048576) return fail(ROZM_INVALID_ARGUMENT,"UTF-8 input exceeds size limit");
 if(!len) return Rozm_SynthesizeW(handle,L"",voice,speed,out);
 int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text,(int)len,NULL,0);
 if(!n) return fail(ROZM_INVALID_ARGUMENT,"Invalid UTF-8");
 try {std::wstring wide(n,L'\0'); MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text,(int)len,&wide[0],n);return Rozm_SynthesizeW(handle,wide.c_str(),voice,speed,out);}
 catch(const std::bad_alloc&) {return fail(ROZM_OUT_OF_MEMORY,"Out of memory");}
}
int __cdecl Rozm_SaveWavW(RozmHandle handle,const wchar_t* text,int voice,int speed,const wchar_t* filename) {
 error.clear(); if(!filename||!*filename) return fail(ROZM_INVALID_ARGUMENT,"filename is empty");
 RozmAudio a={}; int status=Rozm_SynthesizeW(handle,text,voice,speed,&a); if(status) return status;
 try {
  uint8_t header[44]={'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,1,0,1,0,0x11,0x2b,0,0,0x11,0x2b,0,0,1,0,8,0,'d','a','t','a',0,0,0,0};
  uint32_t riff=36+a.size+(a.size&1);memcpy(header+4,&riff,4);memcpy(header+40,&a.size,4);
  std::vector<uint8_t> wav(44+a.size+(a.size&1),0);memcpy(wav.data(),header,44);if(a.size) memcpy(wav.data()+44,a.data,a.size);
  writeFile(filename,wav.data(),wav.size());Rozm_FreeAudio(&a);return ROZM_OK;
 }catch(const std::bad_alloc&) {Rozm_FreeAudio(&a);return fail(ROZM_OUT_OF_MEMORY,"Out of memory");} catch(const std::exception& e) {Rozm_FreeAudio(&a);return fail(ROZM_IO_ERROR,e.what());}
}
const char* __cdecl Rozm_GetLastError(void) {return error.c_str();}
uint32_t __cdecl Rozm_GetVersion(void) {return 0x00010000;}
}
