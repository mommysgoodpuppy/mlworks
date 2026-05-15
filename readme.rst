MLWorks Reboot Fork
===================

This fork is a personal software-archeology project around Harlequin
MLWorks, a Standard ML compiler and development system from the 1990s.

The practical goal is narrower than restoring the whole historical product:
make MLWorks useful as a small, portable Windows SML compiler for hobby
projects, experiments, and exploring an old industrial-strength ML system.
The IDE is not the focus.

Why this fork exists
--------------------

Standard ML is a nice language for small programs, language experiments, and
compiler work, but installing an SML compiler on Windows often drags in more
infrastructure than the project deserves: Unix toolchains, MSYS/Cygwin, MSVC,
Clang, package-manager bootstraps, or a full compiler build environment.

The aim here is closer to:

* download a Windows package;
* run ``mlw.exe run hello.sml``;
* build or deliver a small executable;
* use real SML without dedicating the machine to an SML environment.

MLWorks is interesting for this because it already had a native-code SML
compiler, a runtime system, a batch compiler image, a Basis Library
implementation, a foreign interface, and executable delivery. Much of it still
works if the old assumptions are made explicit and wrapped in a modern command
line.

What works now
--------------

The current useful entry point is ``MLWorks-reboot\mlw.exe``.

From ``MLWorks-reboot``:

::

    mlw.exe run examples\hi.mlb
    mlw.exe run examples\basis.mlb 40
    mlw.exe run examples\modules\main.mlb

Supported workflow commands:

::

    mlw.exe build path\to\main.sml
    mlw.exe build path\to\main.mlb
    mlw.exe run path\to\main.sml
    mlw.exe run path\to\main.mlb
    mlw.exe run path\to\main.mo
    mlw.exe exe path\to\main.mlb path\to\program.exe
    mlw.exe deliver path\to\main.mlb path\to\program.exe
    mlw.exe basis
    mlw.exe foreign

The wrapper currently provides:

* a ``zig run``-style build-and-run path for ``.sml`` and a small subset of
  MLton-style ``.mlb`` manifests;
* automatic generated MLWorks project files under ``.mlw`` instead of filling
  the source directory with compiler outputs;
* automatic Basis Library wiring for common structures such as ``Int``,
  ``List``, ``Array``, ``TextIO``, ``CommandLine``, ``OS``, and ``Timer``;
* multi-file project support through MLWorks ``require "unit"`` declarations;
* launcher-style executable creation with ``mlw.exe exe``;
* historical MLWorks heap delivery with ``mlw.exe deliver``;
* opt-in Foreign Interface support via ``MLWORKS_FOREIGN=1``;
* rebuild scripts for the launcher and runtime;
* a packaging script for a portable-ish Windows distribution.

Portable Windows build
----------------------

The most user-facing artifact is a Windows/i386 reboot package built by:

::

    tools\package-mlw.cmd

That writes:

::

    dist\mlworks-reboot-win32\
    dist\mlworks-reboot-win32.zip

The package contains the launcher, compiler image, runtime DLLs, runtime
images, examples, Basis/Foreign sources, and prebuilt object cache. It is meant
to be usable without a full checkout beside it.

This is still an old Windows/i386 system. Delivered executables are more
self-contained than loose object/image launches, but they still need
``libmlw.dll`` next to the generated ``.exe`` because the rebooted Windows RTS
is DLL-based.

How the reboot is architected
-----------------------------

The historical compiler path is image based. The important pieces are:

``MLWorks-reboot\mlw.exe``
    A small C launcher for the modern workflow. It generates temporary
    ``.mlp`` projects, invokes the batch compiler image, writes runtime object
    lists, and runs or packages the result.

``MLWorks-reboot\compiler\mlw-compiler.exe``
    A saved MLWorks batch compiler image. This is not a C compiler executable
    built directly from the SML compiler sources; it is an MLWorks image saved
    as an executable.

``MLWorks-reboot\bin\I386\NT\main.exe`` and ``libmlw.dll``
    The rebuilt runtime launcher and runtime DLL used to run compiled objects.

``MLWorks-reboot\images\I386\NT\pervasive-test.img``
    The small runtime image used by the reboot command-line path.

``src\basis.mlp`` and ``objects\i386\nt\release``
    The Basis Library project and compiled objects. The portable package copies
    these under ``MLWorks-reboot\lib`` so the package can stand alone.

The old batch compiler is project-oriented. Rather than trying to revive the
broken ``-compile`` single-file path, ``mlw.exe`` generates a minimal MLWorks
project and drives the compiler through the project build path. For MLB files,
the current support is intentionally small: comments, plain ``.sml`` entries,
and nested ``.mlb`` entries, with the final ``.sml`` file used as the target.

Why ``mlw``?
------------

``mlw`` is the pragmatic compatibility layer between a 1990s image-based
compiler and the way a small compiler CLI should feel today.

It hides details that are useful for archaeology but unpleasant for daily use:

* saved images;
* pervasive paths;
* generated ``.mlp`` files;
* Basis object load order;
* runtime ``-load`` and ``-from`` arguments;
* support directories for launcher executables;
* heap-delivery wrapper code.

The goal is not to disguise MLWorks as a new compiler. The goal is to keep the
old system recognizable while making the common path short enough to actually
use.

Building from source
--------------------

For normal use, prefer the packaged Windows build.

For development, the current checked-in recipes are:

::

    tools\reboot-mlw.cmd
    tools\reboot-rts.cmd
    tools\package-mlw.cmd

``reboot-mlw.cmd`` rebuilds ``MLWorks-reboot\mlw.exe`` and runs smoke tests.
``reboot-rts.cmd`` rebuilds and packages the Windows RTS artifacts.
``package-mlw.cmd`` creates the portable Windows folder and zip.

The RTS build is still archaeology. It currently uses MSVC plus the old
makefile flow and helper Unix-like tools. The point of the packaged reboot is
that users should not need any of that just to use SML.

Current caveats
---------------

* Windows/i386 only for the reboot package.
* The IDE is out of scope.
* ``mlw.exe deliver`` expects a ``main : unit -> 'a`` binding. It strips a
  simple top-level ``val _ = main ()`` launcher from its private delivery copy,
  but arbitrary top-level effects can still run while packaging.
* Do not run multiple ``mlw`` commands concurrently against the same source
  directory; they share the generated ``.mlw`` project state.
* Paths with spaces are still risky because parts of the old runtime argument
  parser predate modern quoting expectations.
* The old registry warning is harmless for this workflow:

::

    Software/Harlequin/MLWorks/Pervasive Path value not set in registry.

Upstream
--------

MLWorks was developed by Harlequin in the 1990s. Harlequin broke up in 1998,
and MLWorks later became property of Xanalys Limited. Ravenbrook Limited,
whose directors included members of the original MLWorks team, acquired the
rights to MLWorks in 2013 and open sourced the project.

The original Ravenbrook project page is:

    http://www.ravenbrook.com/project/mlworks/

This fork builds on that open-source release with a narrower practical goal:
make the rebooted compiler easy to try and useful for small Windows SML work.
