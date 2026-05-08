#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILES 512
#define MAX_CMD 32768

static void die(const char *message)
{
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(2);
}

static void strip_filename(char *path)
{
  char *slash = strrchr(path, '\\');
  if (slash != NULL) {
    slash[1] = '\0';
  }
}

static const char *basename_of(const char *path)
{
  const char *slash = strrchr(path, '\\');
  const char *fslash = strrchr(path, '/');
  if (fslash != NULL && (slash == NULL || fslash > slash)) {
    slash = fslash;
  }
  return slash == NULL ? path : slash + 1;
}

static void dirname_of(const char *path, char *out, size_t out_size)
{
  const char *base = basename_of(path);
  size_t len = (size_t)(base - path);
  if (len == 0) {
    GetCurrentDirectoryA((DWORD)out_size, out);
    return;
  }
  if (len > 3 && (path[len - 1] == '\\' || path[len - 1] == '/')) {
    --len;
  }
  if (len >= out_size) {
    die("path too long");
  }
  memcpy(out, path, len);
  out[len] = '\0';
}

static int ends_with_sml(const char *name)
{
  size_t len = strlen(name);
  const char *ext;
  if (len < 4) {
    return 0;
  }
  ext = name + len - 4;
  return _stricmp(ext, ".sml") == 0;
}

static int ends_with_mlb(const char *name)
{
  size_t len = strlen(name);
  const char *ext;
  if (len < 4) {
    return 0;
  }
  ext = name + len - 4;
  return _stricmp(ext, ".mlb") == 0;
}

static int ends_with_mo(const char *name)
{
  size_t len = strlen(name);
  const char *ext;
  if (len < 3) {
    return 0;
  }
  ext = name + len - 3;
  return _stricmp(ext, ".mo") == 0;
}

static int ends_with_exe(const char *name)
{
  size_t len = strlen(name);
  const char *ext;
  if (len < 4) {
    return 0;
  }
  ext = name + len - 4;
  return _stricmp(ext, ".exe") == 0;
}

static void stem_of(const char *filename, char *out, size_t out_size)
{
  const char *base = basename_of(filename);
  size_t len = strlen(base);
  if (ends_with_sml(base)) {
    len -= 4;
  } else if (ends_with_mlb(base)) {
    len -= 4;
  } else if (ends_with_mo(base)) {
    len -= 3;
  } else if (ends_with_exe(base)) {
    len -= 4;
  }
  if (len >= out_size) {
    die("filename too long");
  }
  memcpy(out, base, len);
  out[len] = '\0';
}

static void join_path(const char *dir, const char *file, char *out, size_t out_size)
{
  size_t len = strlen(dir);
  const char *sep = "";
  if (len > 0 && dir[len - 1] != '\\' && dir[len - 1] != '/') {
    sep = "\\";
  }
  if (snprintf(out, out_size, "%s%s%s", dir, sep, file) >= (int)out_size) {
    die("path too long");
  }
}

static void append(char *cmd, size_t cmd_size, const char *text)
{
  size_t used = strlen(cmd);
  size_t add = strlen(text);
  if (used + add + 1 >= cmd_size) {
    die("command line too long");
  }
  memcpy(cmd + used, text, add + 1);
}

static void append_quoted(char *cmd, size_t cmd_size, const char *text)
{
  const char *p;
  append(cmd, cmd_size, "\"");
  for (p = text; *p != '\0'; ++p) {
    if (*p == '"') {
      append(cmd, cmd_size, "\\\"");
    } else {
      char one[2];
      one[0] = *p;
      one[1] = '\0';
      append(cmd, cmd_size, one);
    }
  }
  append(cmd, cmd_size, "\"");
}

static void mkdir_if_needed(const char *path)
{
  DWORD attrs = GetFileAttributesA(path);
  if (attrs != INVALID_FILE_ATTRIBUTES) {
    if ((attrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
      die("path exists and is not a directory");
    }
    return;
  }
  if (!CreateDirectoryA(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
    fprintf(stderr, "could not create directory, error %lu\n%s\n", GetLastError(), path);
    exit(2);
  }
}

static void copy_file_or_die(const char *from, const char *to)
{
  if (!CopyFileA(from, to, FALSE)) {
    fprintf(stderr, "could not copy file, error %lu\nfrom: %s\nto: %s\n",
            GetLastError(), from, to);
    exit(2);
  }
}

static int run_command(char *cmd, const char *cwd)
{
  char old_cwd[MAX_PATH];
  int code;
  if (getenv("MLW_DEBUG") != NULL) {
    fprintf(stderr, "cwd: %s\ncmd: %s\n", cwd, cmd);
  }
  if (GetCurrentDirectoryA(sizeof(old_cwd), old_cwd) == 0) {
    die("could not read current directory");
  }
  if (!SetCurrentDirectoryA(cwd)) {
    fprintf(stderr, "failed to enter directory, error %lu\n%s\n", GetLastError(), cwd);
    return 127;
  }
  code = system(cmd);
  SetCurrentDirectoryA(old_cwd);
  if (code == -1) {
    return 127;
  }
  return code;
}

static int list_sml_files(const char *dir, char files[MAX_FILES][MAX_PATH])
{
  char pattern[MAX_PATH];
  WIN32_FIND_DATAA data;
  HANDLE find;
  int count = 0;
  snprintf(pattern, sizeof(pattern), "%s\\*.sml", dir);
  find = FindFirstFileA(pattern, &data);
  if (find == INVALID_HANDLE_VALUE) {
    return 0;
  }
  do {
    if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
      if (count >= MAX_FILES) {
        FindClose(find);
        die("too many .sml files in source directory");
      }
      strcpy(files[count++], data.cFileName);
    }
  } while (FindNextFileA(find, &data));
  FindClose(find);
  return count;
}

static void emit_project_lines(FILE *f, int file_count, char files[MAX_FILES][MAX_PATH],
                               const char *target)
{
  int i;
  int body_lines = 49 + file_count;
  fprintf(f, "MLWorks 2.0\n");
  fprintf(f, "mlw-generated.mlp %d 7\n", body_lines);
  fprintf(f, "  AboutInfo 2 2\n");
  fprintf(f, "    Version 0 0\n");
  fprintf(f, "    Description 0 0\n");
  fprintf(f, "  Files %d 0\n", file_count);
  for (i = 0; i < file_count; ++i) {
    fprintf(f, "    %s\n", files[i]);
  }
  fprintf(f, "  Subprojects 0 0\n");
  fprintf(f, "  Targets 3 1\n");
  fprintf(f, "    Details 1 0\n");
  fprintf(f, "      %s OBJECT_FILE\n", target);
  fprintf(f, "    %s\n", target);
  fprintf(f, "  Modes 19 2\n");
  fprintf(f, "    Debug 8 2\n");
  fprintf(f, "      Location 1 0\n");
  fprintf(f, "        Debug\n");
  fprintf(f, "      Flags 5 0\n");
  fprintf(f, "        generate_interruptable_code true\n");
  fprintf(f, "        generate_interceptable_code true\n");
  fprintf(f, "        generate_debug_info true\n");
  fprintf(f, "        generate_variable_debug_info true\n");
  fprintf(f, "        mips_r4000 true\n");
  fprintf(f, "    Release 8 2\n");
  fprintf(f, "      Location 1 0\n");
  fprintf(f, "        Release\n");
  fprintf(f, "      Flags 5 0\n");
  fprintf(f, "        generate_interruptable_code true\n");
  fprintf(f, "        optimize_leaf_fns true\n");
  fprintf(f, "        optimize_tail_calls true\n");
  fprintf(f, "        optimize_self_tail_calls true\n");
  fprintf(f, "        mips_r4000 true\n");
  fprintf(f, "    Release\n");
  fprintf(f, "  Configurations 13 6\n");
  fprintf(f, "    SPARC/SunOS 1 1\n");
  fprintf(f, "      Library 0 0\n");
  fprintf(f, "    I386/Win95 1 1\n");
  fprintf(f, "      Library 0 0\n");
  fprintf(f, "    I386/Linux 1 1\n");
  fprintf(f, "      Library 0 0\n");
  fprintf(f, "    I386/NT 1 1\n");
  fprintf(f, "      Library 0 0\n");
  fprintf(f, "    MIPS/Irix 1 1\n");
  fprintf(f, "      Library 0 0\n");
  fprintf(f, "    SPARC/Solaris 1 1\n");
  fprintf(f, "      Library 0 0\n");
  fprintf(f, "    I386/NT\n");
  fprintf(f, "  Locations 5 3\n");
  fprintf(f, "    LibraryPath 0 0\n");
  fprintf(f, "    Objects 1 0\n");
  fprintf(f, "      objects\n");
  fprintf(f, "    Binaries 1 0\n");
        fprintf(f, "      \n");
}

static void write_project(const char *project_dir, char *project, size_t project_size,
                          char files[MAX_FILES][MAX_PATH], int count,
                          const char *target)
{
  FILE *f;

  snprintf(project, project_size, "%s\\mlw-generated.mlp", project_dir);
  f = fopen(project, "w");
  if (f == NULL) {
    die("could not write generated project file");
  }
  emit_project_lines(f, count, files, target);
  fclose(f);
}

static void make_project(const char *source, char *project, size_t project_size,
                         char files[MAX_FILES][MAX_PATH], int *count)
{
  char source_dir[MAX_PATH];
  const char *target = basename_of(source);
  int found = 0;
  int i;

  dirname_of(source, source_dir, sizeof(source_dir));
  *count = list_sml_files(source_dir, files);
  for (i = 0; i < *count; ++i) {
    if (_stricmp(files[i], target) == 0) {
      found = 1;
    }
  }
  if (!found) {
    if (*count >= MAX_FILES) {
      die("too many .sml files in source directory");
    }
    strcpy(files[(*count)++], target);
  }

  write_project(source_dir, project, project_size, files, *count, target);
}

static void trim_token(char *token)
{
  char *start = token;
  char *end;
  while (*start != '\0' && isspace((unsigned char)*start)) {
    ++start;
  }
  if (start != token) {
    memmove(token, start, strlen(start) + 1);
  }
  end = token + strlen(token);
  while (end > token && isspace((unsigned char)end[-1])) {
    *--end = '\0';
  }
}

static int is_mlb_keyword(const char *token)
{
  return _stricmp(token, "basis") == 0 ||
         _stricmp(token, "bas") == 0 ||
         _stricmp(token, "local") == 0 ||
         _stricmp(token, "in") == 0 ||
         _stricmp(token, "end") == 0;
}

static void add_mlb_file(char files[MAX_FILES][MAX_PATH], int *count, const char *file)
{
  int i;
  for (i = 0; i < *count; ++i) {
    if (_stricmp(files[i], file) == 0) {
      return;
    }
  }
  if (*count >= MAX_FILES) {
    die("too many files in MLB");
  }
  if (strlen(file) >= MAX_PATH) {
    die("MLB file path too long");
  }
  strcpy(files[(*count)++], file);
}

static void parse_mlb_file(const char *mlb, const char *base_dir,
                           char files[MAX_FILES][MAX_PATH], int *count,
                           int depth)
{
  FILE *f;
  char token[MAX_PATH];
  int token_len = 0;
  int c;
  int comment_depth = 0;

  if (depth > 16) {
    die("nested MLB includes too deep");
  }

  f = fopen(mlb, "r");
  if (f == NULL) {
    fprintf(stderr, "could not open MLB file: %s\n", mlb);
    exit(2);
  }

  while ((c = fgetc(f)) != EOF) {
    if (comment_depth > 0) {
      if (c == '(') {
        int next = fgetc(f);
        if (next == '*') {
          ++comment_depth;
        } else if (next != EOF) {
          ungetc(next, f);
        }
      } else if (c == '*') {
        int next = fgetc(f);
        if (next == ')') {
          --comment_depth;
        } else if (next != EOF) {
          ungetc(next, f);
        }
      }
      continue;
    }

    if (c == '(') {
      int next = fgetc(f);
      if (next == '*') {
        ++comment_depth;
        continue;
      }
      if (next != EOF) {
        ungetc(next, f);
      }
    }

    if (isspace((unsigned char)c)) {
      if (token_len > 0) {
        char child[MAX_PATH];
        char child_dir[MAX_PATH];
        token[token_len] = '\0';
        trim_token(token);
        if (ends_with_sml(token)) {
          add_mlb_file(files, count, token);
        } else if (ends_with_mlb(token)) {
          join_path(base_dir, token, child, sizeof(child));
          dirname_of(child, child_dir, sizeof(child_dir));
          parse_mlb_file(child, child_dir, files, count, depth + 1);
        } else if (token[0] != '\0' && !is_mlb_keyword(token)) {
          fprintf(stderr, "warning: ignoring unsupported MLB token `%s'\n", token);
        }
        token_len = 0;
      }
    } else {
      if (token_len + 1 >= MAX_PATH) {
        fclose(f);
        die("MLB token too long");
      }
      token[token_len++] = (char)c;
    }
  }

  if (token_len > 0) {
    char child[MAX_PATH];
    char child_dir[MAX_PATH];
    token[token_len] = '\0';
    trim_token(token);
    if (ends_with_sml(token)) {
      add_mlb_file(files, count, token);
    } else if (ends_with_mlb(token)) {
      join_path(base_dir, token, child, sizeof(child));
      dirname_of(child, child_dir, sizeof(child_dir));
      parse_mlb_file(child, child_dir, files, count, depth + 1);
    } else if (token[0] != '\0' && !is_mlb_keyword(token)) {
      fprintf(stderr, "warning: ignoring unsupported MLB token `%s'\n", token);
    }
  }

  fclose(f);
}

static void make_project_from_mlb(const char *mlb, char *project, size_t project_size,
                                  char *target, size_t target_size,
                                  char files[MAX_FILES][MAX_PATH], int *count)
{
  char mlb_dir[MAX_PATH];

  dirname_of(mlb, mlb_dir, sizeof(mlb_dir));
  *count = 0;
  parse_mlb_file(mlb, mlb_dir, files, count, 0);
  if (*count == 0) {
    die("MLB file did not list any .sml files");
  }

  snprintf(target, target_size, "%s", files[*count - 1]);
  write_project(mlb_dir, project, project_size, files, *count, target);
}

static int build_project_file(const char *root, const char *project, const char *target)
{
  char cmd[MAX_CMD] = "";
  char pervasive[MAX_PATH];
  const char *env_pervasive = getenv("MLWORKS_PERVASIVE_DIR");

  if (env_pervasive != NULL && env_pervasive[0] != '\0') {
    snprintf(pervasive, sizeof(pervasive), "%s", env_pervasive);
  } else {
    snprintf(pervasive, sizeof(pervasive), "%s..\\src\\pervasive", root);
  }

  append(cmd, sizeof(cmd), "compiler\\mlw-compiler.exe");
  append(cmd, sizeof(cmd), " -no-banner -pervasive-dir ");
  append_quoted(cmd, sizeof(cmd), pervasive);
  append(cmd, sizeof(cmd), " -project ");
  append_quoted(cmd, sizeof(cmd), project);
  append(cmd, sizeof(cmd), " -configuration I386/NT -target ");
  append_quoted(cmd, sizeof(cmd), target);
  append(cmd, sizeof(cmd), " -build");
  return run_command(cmd, root);
}

static void remove_if_exists(const char *path)
{
  DWORD attrs = GetFileAttributesA(path);
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    return;
  }
  if ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0) {
    die("refusing to remove directory where object file was expected");
  }
  if (!DeleteFileA(path)) {
    fprintf(stderr, "could not remove stale object file, error %lu\n%s\n",
            GetLastError(), path);
    exit(2);
  }
}

static void remove_project_objects(const char *project_dir,
                                   char files[MAX_FILES][MAX_PATH], int count)
{
  int i;
  for (i = 0; i < count; ++i) {
    char stem[MAX_PATH];
    char object[MAX_PATH];
    stem_of(files[i], stem, sizeof(stem));
    snprintf(object, sizeof(object), "%s\\objects\\i386\\nt\\release\\%s.mo",
             project_dir, stem);
    remove_if_exists(object);
  }
}

static int build_source(const char *root, const char *source, char *object, size_t object_size)
{
  char project[MAX_PATH];
  char source_dir[MAX_PATH];
  char stem[MAX_PATH];
  char files[MAX_FILES][MAX_PATH];
  int count;

  make_project(source, project, sizeof(project), files, &count);
  dirname_of(source, source_dir, sizeof(source_dir));
  stem_of(source, stem, sizeof(stem));

  snprintf(object, object_size, "%s\\objects\\i386\\nt\\release\\%s.mo", source_dir, stem);
  remove_project_objects(source_dir, files, count);
  return build_project_file(root, project, basename_of(source));
}

static int build_mlb(const char *root, const char *mlb, char *object, size_t object_size)
{
  char project[MAX_PATH];
  char mlb_dir[MAX_PATH];
  char target[MAX_PATH];
  char stem[MAX_PATH];
  char files[MAX_FILES][MAX_PATH];
  int count;

  make_project_from_mlb(mlb, project, sizeof(project), target, sizeof(target), files, &count);
  dirname_of(mlb, mlb_dir, sizeof(mlb_dir));
  stem_of(target, stem, sizeof(stem));
  snprintf(object, object_size, "%s\\objects\\i386\\nt\\release\\%s.mo", mlb_dir, stem);
  remove_project_objects(mlb_dir, files, count);
  return build_project_file(root, project, target);
}

static int run_object(const char *root, const char *object, int argc, char **argv)
{
  char cmd[MAX_CMD] = "";
  char image[MAX_PATH];
  char image_rel[MAX_PATH];
  char object_rel[MAX_PATH];
  char runner_dir[MAX_PATH];
  int i;
  const char *env_image = getenv("MLWORKS_RUN_IMAGE");

  if (env_image != NULL && env_image[0] != '\0') {
    snprintf(image, sizeof(image), "%s", env_image);
  } else {
    snprintf(image, sizeof(image), "%simages\\I386\\NT\\pervasive-test.img", root);
  }
  snprintf(runner_dir, sizeof(runner_dir), "%sbin\\I386\\NT", root);
  if (!PathRelativePathToA(image_rel, runner_dir, FILE_ATTRIBUTE_DIRECTORY, image, 0)) {
    snprintf(image_rel, sizeof(image_rel), "%s", image);
  }
  if (!PathRelativePathToA(object_rel, runner_dir, FILE_ATTRIBUTE_DIRECTORY, object, 0)) {
    snprintf(object_rel, sizeof(object_rel), "%s", object);
  }

  append(cmd, sizeof(cmd), "main.exe");
  append(cmd, sizeof(cmd), " -MLWpass MLWargs -relaxed -load ");
  append(cmd, sizeof(cmd), image_rel);
  append(cmd, sizeof(cmd), " ");
  append(cmd, sizeof(cmd), object_rel);
  append(cmd, sizeof(cmd), " MLWargs");
  for (i = 0; i < argc; ++i) {
    append(cmd, sizeof(cmd), " ");
    append_quoted(cmd, sizeof(cmd), argv[i]);
  }
  return run_command(cmd, runner_dir);
}

static void write_launcher_source(const char *source, const char *support_name)
{
  FILE *f = fopen(source, "w");
  if (f == NULL) {
    die("could not write launcher source");
  }
  fprintf(f, "#define _CRT_SECURE_NO_WARNINGS\n");
  fprintf(f, "#define WIN32_LEAN_AND_MEAN\n");
  fprintf(f, "#include <windows.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n\n");
  fprintf(f, "#define SUPPORT_DIR \"%s\"\n#define MAX_CMD 32768\n\n", support_name);
  fprintf(f, "static void strip_filename(char *path){char *slash=strrchr(path,'\\\\');if(slash)slash[1]='\\0';}\n");
  fprintf(f, "static void append(char *cmd,const char *text){size_t u=strlen(cmd),a=strlen(text);if(u+a+1>=MAX_CMD)exit(2);memcpy(cmd+u,text,a+1);}\n");
  fprintf(f, "static void append_quoted(char *cmd,const char *text){const char *p;append(cmd,\"\\\"\");for(p=text;*p;p++){if(*p=='\\\"')append(cmd,\"\\\\\\\"\");else{char one[2];one[0]=*p;one[1]='\\0';append(cmd,one);}}append(cmd,\"\\\"\");}\n");
  fprintf(f, "int main(int argc,char **argv){char exe[MAX_PATH],dir[MAX_PATH],support[MAX_PATH],runtime[MAX_PATH],cmd[MAX_CMD]=\"\";STARTUPINFOA si;PROCESS_INFORMATION pi;DWORD code=1;int i;if(!GetModuleFileNameA(NULL,exe,sizeof(exe)))return 2;strcpy(dir,exe);strip_filename(dir);snprintf(support,sizeof(support),\"%%s%%s\",dir,SUPPORT_DIR);snprintf(runtime,sizeof(runtime),\"%%s\\\\main.exe\",support);append_quoted(cmd,runtime);append(cmd,\" -MLWpass MLWARGS -relaxed -load pervasive-test.img program.mo MLWARGS\");for(i=1;i<argc;i++){append(cmd,\" \");append_quoted(cmd,argv[i]);}memset(&si,0,sizeof(si));si.cb=sizeof(si);memset(&pi,0,sizeof(pi));if(!CreateProcessA(NULL,cmd,NULL,NULL,TRUE,0,NULL,support,&si,&pi)){fprintf(stderr,\"failed to launch MLWorks runtime: %%lu\\n\",GetLastError());return 127;}WaitForSingleObject(pi.hProcess,INFINITE);GetExitCodeProcess(pi.hProcess,&code);CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return (int)code;}\n");
  fclose(f);
}

static void write_launcher_build_script(const char *script, const char *source, const char *out_exe)
{
  FILE *f = fopen(script, "w");
  if (f == NULL) {
    die("could not write launcher build script");
  }
  fprintf(f, "@echo off\r\n");
  fprintf(f, "where cl >nul 2>nul\r\n");
  fprintf(f, "if errorlevel 1 (\r\n");
  fprintf(f, "  if exist \"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat\" call \"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat\" x86 >nul\r\n");
  fprintf(f, ")\r\n");
  fprintf(f, "cl /nologo /W3 /O2 /MT /Fe:\"%s\" \"%s\"\r\n", out_exe, source);
  fprintf(f, "exit /b %%ERRORLEVEL%%\r\n");
  fclose(f);
}

static void default_exe_path(const char *input, char *out, size_t out_size)
{
  char dir[MAX_PATH];
  char stem[MAX_PATH];
  dirname_of(input, dir, sizeof(dir));
  stem_of(input, stem, sizeof(stem));
  if (snprintf(out, out_size, "%s\\%s.exe", dir, stem) >= (int)out_size) {
    die("path too long");
  }
}

static int make_exe_launcher(const char *root, const char *object, const char *out_exe)
{
  char out_dir[MAX_PATH];
  char out_stem[MAX_PATH];
  char support_name[MAX_PATH];
  char support_dir[MAX_PATH];
  char runner_src[MAX_PATH];
  char runner_bat[MAX_PATH];
  char copied_main[MAX_PATH];
  char copied_dll[MAX_PATH];
  char copied_image[MAX_PATH];
  char copied_object[MAX_PATH];
  char src_main[MAX_PATH];
  char src_dll[MAX_PATH];
  char src_image[MAX_PATH];
  char cmd[MAX_CMD] = "";

  dirname_of(out_exe, out_dir, sizeof(out_dir));
  stem_of(out_exe, out_stem, sizeof(out_stem));
  if (snprintf(support_name, sizeof(support_name), "%s.mlwrt", out_stem) >= (int)sizeof(support_name)) {
    die("path too long");
  }
  join_path(out_dir, support_name, support_dir, sizeof(support_dir));
  mkdir_if_needed(support_dir);

  snprintf(src_main, sizeof(src_main), "%sbin\\I386\\NT\\main.exe", root);
  snprintf(src_dll, sizeof(src_dll), "%sbin\\I386\\NT\\libmlw.dll", root);
  snprintf(src_image, sizeof(src_image), "%simages\\I386\\NT\\pervasive-test.img", root);
  join_path(support_dir, "main.exe", copied_main, sizeof(copied_main));
  join_path(support_dir, "libmlw.dll", copied_dll, sizeof(copied_dll));
  join_path(support_dir, "pervasive-test.img", copied_image, sizeof(copied_image));
  join_path(support_dir, "program.mo", copied_object, sizeof(copied_object));
  join_path(support_dir, "mlw-runner.c", runner_src, sizeof(runner_src));
  join_path(support_dir, "build-runner.bat", runner_bat, sizeof(runner_bat));

  copy_file_or_die(src_main, copied_main);
  copy_file_or_die(src_dll, copied_dll);
  copy_file_or_die(src_image, copied_image);
  copy_file_or_die(object, copied_object);
  write_launcher_source(runner_src, support_name);
  write_launcher_build_script(runner_bat, runner_src, out_exe);

  append_quoted(cmd, sizeof(cmd), runner_bat);
  return run_command(cmd, root);
}

static void usage(void)
{
  fputs("Usage:\n", stderr);
  fputs("  mlw build <file.sml>\n", stderr);
  fputs("  mlw build <file.mlb>\n", stderr);
  fputs("  mlw run <file.sml> [program-args...]\n", stderr);
  fputs("  mlw run <file.mlb> [program-args...]\n", stderr);
  fputs("  mlw run <file.mo> [program-args...]\n", stderr);
  fputs("  mlw exe <file.sml|file.mlb|file.mo> [out.exe]\n", stderr);
  fputs("\nEnvironment:\n", stderr);
  fputs("  MLWORKS_PERVASIVE_DIR overrides the compiler pervasive object directory.\n", stderr);
  fputs("  MLWORKS_RUN_IMAGE overrides the runtime image used for running .mo files.\n", stderr);
}

int main(int argc, char **argv)
{
  char root[MAX_PATH];
  char object[MAX_PATH];
  int code;

  if (GetModuleFileNameA(NULL, root, sizeof(root)) == 0) {
    die("could not locate executable");
  }
  strip_filename(root);

  if (argc < 3) {
    usage();
    return 2;
  }

  if (_stricmp(argv[1], "build") == 0) {
    char source[MAX_PATH];
    if (!ends_with_sml(argv[2]) && !ends_with_mlb(argv[2])) {
      die("build expects a .sml source file or .mlb basis file");
    }
    if (GetFullPathNameA(argv[2], sizeof(source), source, NULL) == 0) {
      die("could not resolve source path");
    }
    if (ends_with_mlb(source)) {
      return build_mlb(root, source, object, sizeof(object));
    }
    return build_source(root, source, object, sizeof(object));
  }

  if (_stricmp(argv[1], "run") == 0) {
    if (ends_with_sml(argv[2])) {
      char source[MAX_PATH];
      if (GetFullPathNameA(argv[2], sizeof(source), source, NULL) == 0) {
        die("could not resolve source path");
      }
      code = build_source(root, source, object, sizeof(object));
      if (code == 0) {
        return run_object(root, object, argc - 3, argv + 3);
      }
      return code;
    }
    if (ends_with_mlb(argv[2])) {
      char source[MAX_PATH];
      if (GetFullPathNameA(argv[2], sizeof(source), source, NULL) == 0) {
        die("could not resolve MLB path");
      }
      code = build_mlb(root, source, object, sizeof(object));
      if (code == 0) {
        return run_object(root, object, argc - 3, argv + 3);
      }
      return code;
    }
    if (ends_with_mo(argv[2])) {
      char object_path[MAX_PATH];
      if (GetFullPathNameA(argv[2], sizeof(object_path), object_path, NULL) == 0) {
        die("could not resolve object path");
      }
      return run_object(root, object_path, argc - 3, argv + 3);
    }
    die("run expects a .sml source file or .mo object file");
  }

  if (_stricmp(argv[1], "exe") == 0) {
    char input[MAX_PATH];
    char out_exe[MAX_PATH];
    if (!ends_with_sml(argv[2]) && !ends_with_mlb(argv[2]) && !ends_with_mo(argv[2])) {
      die("exe expects a .sml source, .mlb basis, or .mo object file");
    }
    if (GetFullPathNameA(argv[2], sizeof(input), input, NULL) == 0) {
      die("could not resolve input path");
    }
    if (argc >= 4) {
      if (GetFullPathNameA(argv[3], sizeof(out_exe), out_exe, NULL) == 0) {
        die("could not resolve output executable path");
      }
    } else {
      default_exe_path(input, out_exe, sizeof(out_exe));
    }
    if (ends_with_sml(input)) {
      code = build_source(root, input, object, sizeof(object));
      if (code != 0) {
        return code;
      }
    } else if (ends_with_mlb(input)) {
      code = build_mlb(root, input, object, sizeof(object));
      if (code != 0) {
        return code;
      }
    } else {
      snprintf(object, sizeof(object), "%s", input);
    }
    return make_exe_launcher(root, object, out_exe);
  }

  usage();
  return 2;
}
