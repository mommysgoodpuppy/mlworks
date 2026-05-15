# MLWorks Reboot CLI

This directory contains the current Windows command-line reboot experiment.

## Use

```bat
mlw.exe run examples\hi.mlb
mlw.exe run examples\basis.mlb 40
mlw.exe run examples\modules\main.mlb
```

Build a Windows executable launcher:

```bat
mlw.exe exe examples\hi.mlb
examples\hi.exe
```

Build a smaller delivered executable:

```bat
mlw.exe deliver path\to\program.mlb path\to\program.exe
program.exe
```

`deliver` expects the program to define `main : unit -> 'a`; any return value is
discarded. It uses the historical MLWorks heap-delivery path and copies
`libmlw.dll` beside the generated executable.

or, as separate build and run steps:

```bat
mlw.exe build examples\hi.sml
mlw.exe run examples\.mlw\objects\i386\nt\release\hi.mo
```

Useful commands:

```bat
mlw.exe build path\to\main.sml
mlw.exe build path\to\main.mlb
mlw.exe run path\to\main.sml
mlw.exe run path\to\main.mlb
mlw.exe run path\to\main.mo
mlw.exe exe path\to\main.mlb
mlw.exe exe path\to\main.sml path\to\program.exe
mlw.exe deliver path\to\main.mlb path\to\program.exe
mlw.exe basis
mlw.exe foreign
```

`mlw.exe build file.sml` generates a small `.mlw\mlw-generated.mlp` project
under the source directory, then builds it with `compiler\mlw-compiler.exe`.

`mlw.exe build file.mlb` supports a small MLB manifest subset: comments,
plain `.sml` entries, and nested `.mlb` entries. It flattens those files into
the generated MLWorks project and uses the last `.sml` entry as the target.

Split programs work through MLWorks unit dependencies. Put a `require` at the
top of the file that needs another source unit:

```sml
require "greeting";

val _ = print (Greeting.line "MLWorks")
```

where `greeting.sml` defines `structure Greeting = ...`. For an MLB, list the
required units before the target:

```sml
greeting.sml
main.sml
```

Generated projects include the repository Basis project by default, so normal
Basis structures such as `Int`, `List`, `Array`, `TextIO`, and `CommandLine`
are visible without hand-written MLWorks `require` lines. Runtime execution
loads the ordered Basis object list before the generated project object list.
`mlw.exe basis` refreshes that generated Basis object list.

Foreign Interface support is opt-in:

```bat
set MLWORKS_FOREIGN=1
mlw.exe run path\to\ffi-program.mlb
```

With `MLWORKS_FOREIGN=1`, generated projects include the repository Foreign
Interface project and runtime execution loads the foreign support objects before
the program object. `mlw.exe foreign` refreshes that generated object list.

`build` and source/MLB `run` remove the generated object files for the current
source list before invoking MLWorks, so they force recompilation. MLWorks still
prints `Up to date` after a successful project build.

Generated MLWorks clutter is kept under `.mlw` in the source directory:

```text
.mlw\mlw-generated.mlp
.mlw\mlw-project-objects.txt
.mlw\objects\i386\nt\release\<stem>.mo
.mlw\objects\i386\nt\DEPEND\...
```

`mlw.exe exe` creates a small native launcher executable and a sibling
`<name>.mlwrt` runtime directory containing `main.exe`, `libmlw.dll`, the
runtime image, and the compiled project objects. Keep the `.mlwrt` directory
next to the generated `.exe`.

`mlw.exe deliver` is more self-contained. It generates a tiny delivery wrapper
under `.mlw`, compiles the project, runs the wrapper once, and lets
`MLWorks.Deliver.deliver` write a heap-embedded executable. For script-style
files, the private delivery copy strips a single-line `val _ = main ()` launcher
so packaging does not run `main`. The result still needs `libmlw.dll` next to it
because the rebooted runtime is DLL-based, but it does not need the `.mlwrt`
directory, image file, `main.exe`, or sidecar object files.

## Files

- `mlw.exe`: ergonomic launcher.
- `mlw.c`: launcher source.
- `compiler\mlw-compiler.exe`: saved batch compiler image.
- `compiler\libmlw.dll`: rebuilt runtime DLL required by the compiler image.
- `bin\I386\NT\main.exe`: rebuilt runtime used to run object files.

The packaged RTS uses a rebuilt `libmlw.dll` based at `0x18000000`. The old
`0x800000` fixed DLL base could randomly collide with modern Windows process
layout and fail before the runtime reached `main()`.

More detail is in `..\docs\mlworks-reboot.md`.

That document includes the launcher and RTS rebuild recipes. The short forms
from the repo root are:

```bat
tools\reboot-mlw.cmd
tools\reboot-rts.cmd
tools\package-mlw.cmd
```

`reboot-mlw.cmd` rebuilds `MLWorks-reboot\mlw.exe` and smoke-tests hello-world
plus Basis loading. `reboot-rts.cmd` rebuilds the RTS, refreshes packaged
`main.exe`/`libmlw.dll`, and runs the smoke test. `package-mlw.cmd` writes a
portable Windows zip under `dist\mlworks-reboot-win32.zip`.
