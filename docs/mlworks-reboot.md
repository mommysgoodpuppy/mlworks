# MLWorks Reboot Notes

This repo currently has a working Windows command-line path for the old
Harlequin MLWorks compiler. The IDE is not part of this path.

## Pieces

The useful files live under `MLWorks-reboot`:

- `mlw.exe`: small launcher for the modern workflow.
- `compiler/mlw-compiler.exe`: batch compiler image saved as an executable.
- `compiler/libmlw.dll`: rebuilt runtime DLL required by the compiler image.
- `bin/I386/NT/main.exe`: rebuilt runtime launcher, used for running `.mo`
  files.
- `images/I386/NT/pervasive-test.img`: minimal image used for running compiled
  objects.

`mlw.bat` is the old wrapper. It still works by loading `batch.img`, but the new
launcher avoids making users type the runtime/image incantation.

## Quick Start

From `MLWorks-reboot`:

```bat
mlw.exe run examples\hi.mlb
mlw.exe run examples\basis.mlb 40
mlw.exe run examples\modules\main.mlb
```

That builds the program and then runs the generated object file. The separate
steps are:

```bat
mlw.exe build examples\hi.sml
mlw.exe run examples\.mlw\objects\i386\nt\release\hi.mo
```

The build command:

1. writes a minimal `.mlw\mlw-generated.mlp` project under the source directory;
2. invokes `compiler\mlw-compiler.exe` to build the target.

The generated project and object files are written under `.mlw` in the source
directory:

```text
<source-dir>\.mlw\mlw-generated.mlp
<source-dir>\.mlw\objects\i386\nt\release\<stem>.mo
```

For example:

```text
examples\.mlw\objects\i386\nt\release\hi.mo
```

## Commands

Build a source file:

```bat
mlw.exe build path\to\main.sml
```

Build from a small MLB manifest:

```bat
mlw.exe build path\to\main.mlb
```

Run an already-built object file:

```bat
mlw.exe run path\to\main.mo
```

Build and immediately run a source file or MLB manifest:

```bat
mlw.exe run path\to\main.sml
mlw.exe run path\to\main.mlb
```

For generated source/MLB projects, the wrapper removes the expected `.mo` files
for the current file list before invoking MLWorks. That makes the CLI path
force recompilation even though MLWorks prints `Up to date` after a successful
project build.

Build a Windows executable launcher:

```bat
mlw.exe exe path\to\main.sml
mlw.exe exe path\to\main.mlb path\to\program.exe
```

The generated executable is a native launcher around the rebooted MLWorks
runtime. It creates a sibling `<name>.mlwrt` directory containing:

```text
main.exe
libmlw.dll
pervasive-test.img
compiled project objects
```

Keep that directory next to the `.exe`.

Build a smaller heap-delivered executable:

```bat
mlw.exe deliver path\to\main.sml path\to\program.exe
mlw.exe deliver path\to\main.mlb path\to\program.exe
```

`deliver` expects the input project to define:

```sml
val main : unit -> 'a
```

The launcher writes a private `.mlw\mlw-main.sml` copy of the entry source,
generates `.mlw\mlw-deliver.sml`, compiles that wrapper as the target, runs the
wrapper once, and calls the historical:

```sml
MLWorks.Deliver.deliver (out, fn () => (ignore (main ()); ()), MLWorks.Deliver.CONSOLE)
```

That path writes a Windows executable with the ML heap embedded in the PE
image. It is more self-contained than `mlw.exe exe`: the delivered executable
only needs `libmlw.dll` beside it, because this reboot links the RTS as a DLL.
It does not need `main.exe`, `pervasive-test.img`, `.mo` files, or a `.mlwrt`
directory.

For script-style files, the private entry copy strips a single-line top-level
launcher of this exact shape:

```sml
val _ = main ()
```

This lets the same file work with `mlw run file.sml` without running `main`
while `mlw deliver file.sml` is packaging the executable.

Program arguments after the file are passed to the loaded SML program:

```bat
mlw.exe run path\to\main.sml arg1 arg2
```

## Environment

`MLWORKS_PERVASIVE_DIR` overrides the compiler pervasive object directory.
By default the launcher uses:

```text
..\src\pervasive
```

relative to `MLWorks-reboot`.

`MLWORKS_RUN_IMAGE` overrides the image used to run object files. By default the
launcher uses:

```text
images\I386\NT\pervasive-test.img
```

`MLWORKS_NO_BASIS` disables the launcher's automatic Basis project and runtime
object-list wiring. This is mainly useful when debugging the raw compiler
project behavior.

## Project Files

MLWorks batch compilation is project based. The historical single-file
`-compile` mode is not useful yet in this reboot: it can fail with messages like
`no such unit exists`.

The launcher works around that by generating a minimal
`.mlw\mlw-generated.mlp` under the source directory. It includes all `.sml`
files in that directory and sets the requested source file as the target.

Generated projects include `src\basis.mlp` as a subproject by default. That
makes normal Basis structures such as `Int`, `List`, `Array`, `TextIO`,
`CommandLine`, `OS`, and `Timer` visible to small standalone programs without
hand-written MLWorks `require` declarations.

Runtime loading has a matching Basis step. `src\basis\require_all.sml` is not a
self-contained object; it still references objects like `__option.mo` and
`__int.mo`. The wrapper therefore asks the batch compiler to dump the ordered
Basis dependency list:

```bat
mlw.exe basis
```

This writes:

```text
MLWorks-reboot\basis-objects.txt
```

Automatic `build`, `run`, and `exe` commands create this cache only when it is
missing. The explicit `mlw.exe basis` command refreshes it.

`mlw.exe run` then writes a transient `MLWorks-reboot\mlw-runtime-objects.txt`
containing the absolute Basis object paths followed by the generated project
object list, and runs the RTS with:

```text
-from mlw-runtime-objects.txt
```

`mlw.exe exe` copies those Basis objects into the generated `.mlwrt\basis`
directory and writes a local `modules.txt`, so the launcher executable has the
same load order. For generated source/MLB projects, the batch compiler also
writes:

```text
<source-dir>\.mlw\mlw-project-objects.txt
```

That list is now used for `run`, `exe`, and `deliver`, so multi-unit projects
with `require` dependencies load all compiled project objects instead of only
the final target object.

The generated project model is intentionally simple. It is enough for small
programs and local experiments. Larger programs may still want explicit
MLWorks `require` declarations or a hand-maintained project file for precise
dependency boundaries.

## Splitting Code

MLWorks uses source units and explicit `require` declarations. The unit name is
the source filename without `.sml`.

For example, `greeting.sml` can define a structure:

```sml
structure Greeting =
  struct
    fun line name =
      "Hello, " ^ name ^ " from a required unit\n"
  end
```

Then `main.sml` can depend on it:

```sml
require "greeting";

fun main () =
  print (Greeting.line "MLWorks")

val _ = main ()
```

With an MLB manifest, list the dependency first and the target last:

```sml
greeting.sml
main.sml
```

`mlw.exe run`, `mlw.exe exe`, and `mlw.exe deliver` use the compiler's dumped
project object list, so required project units are loaded before the target
unit at runtime.

## Compile Paths

The batch compiler has three nearby paths in `src/main/_batch.sml`:

- `-project ... -build` calls `TopLevel.build`.
- `-compile file.sml` calls `TopLevel.recompile_file`.
- `-compile-file file.sml` calls `TopLevel.compile_file`.

Only the project path is fully wired in the rebooted image. `TopLevel.build`
calls `Project.fromFileInfo`, which turns the loaded `.mlp` state into a
project containing source files, targets, object directory, mode, configuration,
subprojects, and library path.

The two single-file paths start from `Project.initialize`, which creates an
empty project plus the pervasive units. `-compile` then asks that empty project
to find the requested unit and fails with:

```text
<batch compiler:recompile-file>: error: In project: , no such unit exists: hi
```

`-compile-file` gets closer, but `Project.read_object_dependencies` also looks
for source files in the project file list, not in the global `-source-path`.
Since that file list is empty, `TopLevel.compile_file''` reaches compilation
without source metadata and fails with:

```text
unknown location: compiler fault: No source info while compiling `hi'
```

This means `.mlp` is not merely IDE decoration for the currently working path;
it is how the batch compiler receives its project model. A good future repair
would be to make the batch single-file paths synthesize a minimal in-memory
project from the filenames, object path, source path, and default mode instead
of requiring a physical `.mlp`.

There is also an incremental compiler path documented as `mlworks -tty` or the
GUI image. That route exposes `Shell.Project.*` and compile commands through
the interactive environment, but it still uses the same project machinery under
the hood and this reboot package does not currently include a usable `gui.img`.

## MLB Manifest Subset

`mlw.exe build file.mlb` supports the narrow MLB shape used by simple editor
and LSP workflows:

```sml
(* program files *)
file1.sml
file2.sml
main.sml
```

The wrapper strips `(* ... *)` comments, reads whitespace-separated `.sml`
entries, recursively flattens `.mlb` entries, writes `.mlw\mlw-generated.mlp`, and
uses the last `.sml` entry as the target.

This is intentionally not full MLton MLB semantics. `basis`, `local`, `in`,
and `end` are accepted as structural words, but MLWorks does not enforce MLB
export/import filtering, basis scoping, module renaming, annotations, or path
variables. Unsupported tokens are ignored with a warning.

## Rebuilding The Launcher

Use the checked-in launcher recipe instead of typing the MSVC command by hand:

```bat
cd /d C:\GIT\mlworks
tools\reboot-mlw.cmd
```

That runs two phases:

```text
build    rebuild MLWorks-reboot\mlw.exe from MLWorks-reboot\mlw.c
smoke    run hello-world and Basis smoke tests
```

Individual phases are available:

```bat
tools\reboot-mlw.cmd build
tools\reboot-mlw.cmd smoke
```

Useful environment override:

```bat
set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat
```

The script centralizes:

- 32-bit MSVC environment via `vcvarsall.bat x86`.
- `cl /nologo /W3 /O2 /MT /Fe:mlw.exe mlw.c shlwapi.lib`.
- removal of disposable `mlw.obj`.
- smoke tests for `examples\hi.mlb` and `examples\basis.mlb`.

## Packaging The Windows Reboot

Use the checked-in package recipe to create a portable Windows folder and zip:

```bat
cd /d C:\GIT\mlworks
tools\package-mlw.cmd
```

The default output is:

```text
dist\mlworks-reboot-win32\
dist\mlworks-reboot-win32.zip
```

The package contains:

```text
mlw.exe
mlw.bat
README.md
basis-objects.txt
foreign-objects.txt
bin\I386\NT\main.exe
bin\I386\NT\libmlw.dll
compiler\mlw-compiler.exe
compiler\libmlw.dll
images\I386\NT\pervasive-test.img
examples\...
lib\src\pervasive\...
lib\src\basis\...
lib\src\foreign\...
lib\src\win_nt\...
lib\src\rts\gen\...
lib\objects\i386\nt\release\...
```

`mlw.exe` prefers the packaged `lib\src` and `lib\objects` paths when present.
When those paths are absent, it falls back to the repository layout:

```text
..\src
..\objects\i386\nt\release
```

This keeps development in the source checkout working while allowing the zip to
build and run programs without a full checkout beside it. The package is still
Windows/i386-specific and delivered executables still need `libmlw.dll` beside
them.

## FFI Smoke Demo

`C:\GIT\mlw-testing\ffi-demo` contains a small Win32 foreign-interface demo.
It builds a C DLL with an exported MLWorks stub initializer, then runs a small
SML program with Foreign Interface support enabled:

```powershell
cd C:\GIT\mlw-testing\ffi-demo
.\build-run.ps1
```

Expected output:

```text
25 + 17 = 42
```

The demo uses the newer dynamic-library API rather than the older manual
`ForeignInterface.Store` API. The C side registers an ML-callable function via
`mlw_ci_register_function`, and the SML side binds it with:

```sml
val add : c_int * c_int -> c_int = MLWorksDynamicLibrary.bind "ffi_add"
```

The launcher does not load the Foreign Interface by default. Opt in with:

```bat
set MLWORKS_FOREIGN=1
mlw.exe run path\to\simple.mlb
```

or refresh the foreign support cache explicitly:

```bat
set MLWORKS_FOREIGN=1
mlw.exe foreign
```

With `MLWORKS_FOREIGN=1`, generated projects include
`src\foreign\foreign.mlp`, and runtime loading includes the foreign support
objects before the program object.

The DLL is linked with:

```text
/BASE:0x19000000 /FIXED
```

This is currently necessary for the same reason the rebooted RTS DLL was moved:
MLWorks stores C function addresses in tagged ML values, so foreign stub code
must load below the practical pointer ceiling. `libmlw.dll` currently uses
`0x18000000`, and the demo stub DLL uses `0x19000000`.

## Rebuilding The RTS

The Windows RTS currently builds with MSVC for the linker/resource side and
GnuWin32/MSYS2 tools for the old makefile flow. The working local setup is:

- Visual Studio 2022 Community with the x86 C/C++ toolchain.
- GnuWin32 `make` on `PATH`.
- MSYS2 `mingw32` and `usr\bin` on `PATH` for Unix-like helper tools.
- A local `src\rts\afxres.h` shim containing:

```c
#include <winres.h>
```

That shim is needed because `runtime.rc` includes the old MFC `afxres.h`, but
the current build does not otherwise need MFC.

Use the checked-in build recipe instead of typing the make flags by hand:

```bat
cd /d C:\GIT\mlworks
tools\reboot-rts.cmd
```

That runs three phases:

```text
build    rebuild src\rts\libmlw.dll and src\rts\bin\I386\NT\main.exe
package  copy the rebuilt artifacts into MLWorks-reboot
smoke    run MLWorks-reboot\mlw.exe run examples\hi.mlb
```

Individual phases are available:

```bat
tools\reboot-rts.cmd build
tools\reboot-rts.cmd package
tools\reboot-rts.cmd smoke
```

Useful environment overrides:

```bat
set DLLBASE=0x18000000
set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat
set GNUWIN32_BIN=C:\Program Files (x86)\GnuWin32\bin
set MSYS2_MINGW32_BIN=C:\msys64\mingw32\bin
set MSYS2_USR_BIN=C:\msys64\usr\bin
```

The script centralizes the old makefile details:

- 32-bit MSVC environment via `vcvarsall.bat x86`.
- GnuWin32/MSYS2 helper tools on `PATH`.
- `ARCH=I386 OS=NT runtime`.
- `-m32 -std=gnu89` plus the NT/Win32 include and define set.
- `DLLLIBRARIES=""` to avoid stale original makefile assumptions.
- the Win32 import libraries needed by the old runtime build.
- `/FORCE:MULTIPLE`, currently needed because MLWorks defines `_msize` and the
  modern UCRT also provides one.
- `DLLBASE`, defaulting to `0x18000000`.

Successful output artifacts include:

```text
src\rts\libmlw.dll
src\rts\bin\I386\NT\main.exe
```

To refresh the reboot package after a manual rebuild, either run:

```bat
tools\reboot-rts.cmd package
```

or copy the files directly:

```powershell
Copy-Item C:\GIT\mlworks\src\rts\libmlw.dll C:\GIT\mlworks\MLWorks-reboot\compiler\libmlw.dll -Force
Copy-Item C:\GIT\mlworks\src\rts\libmlw.dll C:\GIT\mlworks\MLWorks-reboot\bin\I386\NT\libmlw.dll -Force
Copy-Item C:\GIT\mlworks\src\rts\bin\I386\NT\main.exe C:\GIT\mlworks\MLWorks-reboot\bin\I386\NT\main.exe -Force
```

Then smoke-test the packaged path:

```bat
cd /d C:\GIT\mlworks\MLWorks-reboot
mlw.exe run examples\hi.mlb
```

Expected output includes:

```text
Hello, world
```

## Rebuilding The Compiler Image

`MLWorks-reboot\compiler\mlw-compiler.exe` is not a normal C executable built
from compiler sources directly. It is a saved MLWorks image based on
`images\I386\NT\batch.img`.

The RTS patch in `src\rts\src\mlw_start.c` is important here:

```c
if(image_continuation != MLUNIT && !option_save_exec.specified) {
```

Without the `!option_save_exec.specified` guard, loading `batch.img` immediately
runs the batch compiler continuation instead of letting the runtime package it
as an executable.

To regenerate the compiler executable from the batch image:

```bat
cd /d C:\GIT\mlworks\MLWorks-reboot\bin\I386\NT
main.exe -MLWpass MLWargs -load ..\..\..\images\I386\NT\batch.img -save-exec ..\..\..\compiler\mlw-compiler.exe MLWargs
```

After regenerating it, copy the rebuilt `libmlw.dll` beside it:

```powershell
Copy-Item C:\GIT\mlworks\src\rts\libmlw.dll C:\GIT\mlworks\MLWorks-reboot\compiler\libmlw.dll -Force
```

Then verify the compiler image through the launcher:

```bat
cd /d C:\GIT\mlworks\MLWorks-reboot
mlw.exe build examples\hi.mlb
```

## RTS Stabilization

The old Windows RTS linked `libmlw.dll` as a fixed DLL at `0x800000` and did
not include a relocation table. On modern Windows that address is not reliably
free, so startup could fail before `main()` with `0xC0000018`
(`STATUS_CONFLICTING_ADDRESSES`). This looked like random silent failure from
the outside.

The RTS makefile now has a `DLLBASE` setting and defaults it to `0x18000000`.
That keeps the fixed-address assumption but moves the DLL out of the range that
was colliding in testing. A sweep gave these useful results:

```text
0x800000    intermittent: 18/30 success, 12/30 address conflicts
0x10000000  mostly stable: 29/30 success
0x18000000  stable in the sweep: 30/30 success
```

After packaging the rebuilt DLL and launcher into `MLWorks-reboot`, the
user-facing path passed 50 consecutive build-and-run iterations:

```bat
cd /d C:\GIT\mlworks\MLWorks-reboot
mlw.exe build examples\hi.sml
mlw.exe run examples\.mlw\objects\i386\nt\release\hi.mo
```

The RTS also has an optional `MLW_RTS_TRACE=1` path for scheduler/startup
diagnostics. Leave it off for normal use.

## Current Caveats

- `compiler\mlw-compiler.exe` still requires the rebuilt `compiler\libmlw.dll`
  next to it.
- `mlw.exe` is a launcher, not a changed MLWorks compiler image.
- The runner currently uses `pervasive-test.img`; this is enough for the hello
  world object we tested, but needs more validation with larger programs.
- `mlw.exe run file.sml` and `mlw.exe run file.mlb` build and then run the
  generated object file, similar to `zig run`.
- `mlw.exe deliver` requires a `main : unit -> 'a` binding. It strips the
  common single-line `val _ = main ()` entry call from the private delivery
  copy, but other top-level effects still run while producing the delivered
  executable. Keep deliverable programs behind `main` where possible.
- The generated `.mlw\mlw-generated.mlp` is shared per source directory. Avoid
  running multiple `mlw` commands against the same directory at the same time.
- The old runtime filename parser does not handle quoted paths reliably. Keep
  source paths free of spaces for now.
- The old registry warning is harmless for this workflow:

```text
Software/Harlequin/MLWorks/Pervasive Path value not set in registry.
```

## Known Working Test

```bat
cd /d C:\GIT\mlworks\MLWorks-reboot
mlw.exe build examples\hi.sml
mlw.exe run examples\.mlw\objects\i386\nt\release\hi.mo
```

Expected output includes:

```text
Hello, world
```
