# MLWorks Reboot CLI

This directory contains the current Windows command-line reboot experiment.

## Use

```bat
mlw.exe run examples\hi.mlb
```

Build a Windows executable launcher:

```bat
mlw.exe exe examples\hi.mlb
examples\hi.exe
```

or, as separate build and run steps:

```bat
mlw.exe build examples\hi.sml
mlw.exe run examples\objects\i386\nt\release\hi.mo
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
```

`mlw.exe build file.sml` generates a small `mlw-generated.mlp` project beside
the source, then builds it with `compiler\mlw-compiler.exe`.

`mlw.exe build file.mlb` supports a small MLB manifest subset: comments,
plain `.sml` entries, and nested `.mlb` entries. It flattens those files into
the generated MLWorks project and uses the last `.sml` entry as the target.

`build` and source/MLB `run` remove the generated object files for the current
source list before invoking MLWorks, so they force recompilation. MLWorks still
prints `Up to date` after a successful project build.

`mlw.exe exe` creates a small native launcher executable and a sibling
`<name>.mlwrt` runtime directory containing `main.exe`, `libmlw.dll`, the
runtime image, and the compiled `program.mo`. Keep the `.mlwrt` directory next
to the generated `.exe`.

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

That document includes the RTS rebuild recipe. The short form from the repo
root is:

```bat
tools\reboot-rts.cmd
```

It rebuilds the RTS, refreshes packaged `main.exe`/`libmlw.dll`, and runs the
hello-world smoke test.
