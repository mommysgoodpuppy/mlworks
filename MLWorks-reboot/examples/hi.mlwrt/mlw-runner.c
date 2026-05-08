#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUPPORT_DIR "hi.mlwrt"
#define MAX_CMD 32768

static void strip_filename(char *path){char *slash=strrchr(path,'\\');if(slash)slash[1]='\0';}
static void append(char *cmd,const char *text){size_t u=strlen(cmd),a=strlen(text);if(u+a+1>=MAX_CMD)exit(2);memcpy(cmd+u,text,a+1);}
static void append_quoted(char *cmd,const char *text){const char *p;append(cmd,"\"");for(p=text;*p;p++){if(*p=='\"')append(cmd,"\\\"");else{char one[2];one[0]=*p;one[1]='\0';append(cmd,one);}}append(cmd,"\"");}
int main(int argc,char **argv){char exe[MAX_PATH],dir[MAX_PATH],support[MAX_PATH],runtime[MAX_PATH],cmd[MAX_CMD]="";STARTUPINFOA si;PROCESS_INFORMATION pi;DWORD code=1;int i;if(!GetModuleFileNameA(NULL,exe,sizeof(exe)))return 2;strcpy(dir,exe);strip_filename(dir);snprintf(support,sizeof(support),"%s%s",dir,SUPPORT_DIR);snprintf(runtime,sizeof(runtime),"%s\\main.exe",support);append_quoted(cmd,runtime);append(cmd," -MLWpass MLWARGS -relaxed -load pervasive-test.img program.mo MLWARGS");for(i=1;i<argc;i++){append(cmd," ");append_quoted(cmd,argv[i]);}memset(&si,0,sizeof(si));si.cb=sizeof(si);memset(&pi,0,sizeof(pi));if(!CreateProcessA(NULL,cmd,NULL,NULL,TRUE,0,NULL,support,&si,&pi)){fprintf(stderr,"failed to launch MLWorks runtime: %lu\n",GetLastError());return 127;}WaitForSingleObject(pi.hProcess,INFINITE);GetExitCodeProcess(pi.hProcess,&code);CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return (int)code;}
