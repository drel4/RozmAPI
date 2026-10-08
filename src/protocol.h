#pragma once
#include <stdint.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <windows.h>
static const uint32_t ROZM_MAGIC=0x31545a52;
struct RequestHeader {uint32_t magic,voice,speed,path_chars,text_chars;};
static std::vector<uint8_t> readFile(const std::wstring& path,size_t maxBytes) {
 HANDLE h=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
 if(h==INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot open file");
 LARGE_INTEGER n; if(!GetFileSizeEx(h,&n)||n.QuadPart<0||(uint64_t)n.QuadPart>maxBytes) {CloseHandle(h); throw std::runtime_error("File size out of range");}
 std::vector<uint8_t> b((size_t)n.QuadPart); DWORD done=0;
 BOOL ok=b.empty()||ReadFile(h,b.data(),(DWORD)b.size(),&done,NULL); CloseHandle(h);
 if(!ok||done!=b.size()) throw std::runtime_error("Cannot read file"); return b;
}
static void writeFile(const std::wstring& path,const void* data,size_t n) {
 HANDLE h=CreateFileW(path.c_str(),GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
 if(h==INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create output file");
 DWORD done=0; BOOL ok=n==0||WriteFile(h,data,(DWORD)n,&done,NULL); CloseHandle(h);
 if(!ok||done!=n) throw std::runtime_error("Cannot write output file");
}
