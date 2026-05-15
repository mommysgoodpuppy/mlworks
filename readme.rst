MLWorks Reboot Fork
===================

This fork is a personal software-archeology project around Harlequin
MLWorks, a Standard ML compiler and development system from the 1990s.

The working target is a simple Windows command-line SML system(maybe unix later):

* download a package;
* run ``mlw.exe run hello.sml``;
* split code across a few files;
* use the Standard ML Basis Library;
* build or deliver a small executable;
* explore MLWorks from the package.

The focus is a practical CLI for hobby SML programs and MLWorks exploration.

Why this fork exists
--------------------

Standard ML is fun for small tools, language experiments, and compiler
work. On Windows, trying an SML compiler can involve a lot of setup before the
first program runs: Unix-like shells, MSYS/Cygwin, MSVC, Clang, package-manager
bootstraps, or a full compiler build environment.

This fork aims for a small Windows compiler distribution. A user should be able
to unpack it and start writing SML.

MLWorks is a good candidate for this kind of project because it already has the
pieces of a complete implementation:

* a native-code SML compiler;
* a runtime system;
* a saved batch compiler image;
* a Standard ML Basis Library implementation;
* a foreign interface;
* executable delivery.

``mlw.exe`` builds a modern command-line interface on top of the MLWorks
compiler, runtime, image, Basis, and project infrastructure.

Quick start
-----------

Use ``MLWorks-reboot\mlw.exe``.

From ``MLWorks-reboot``:

::

    mlw.exe run examples\hi.mlb
    mlw.exe run examples\basis.mlb 40
    mlw.exe run examples\modules\main.mlb

Common commands:

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

What works
----------

``mlw.exe`` provides:

* a ``zig run``-style build-and-run path for ``.sml`` files;
* a small MLton-style ``.mlb`` manifest reader for simple project files;
* generated MLWorks project files under ``.mlw``;
* automatic Basis Library setup for structures such as ``Int``, ``List``,
  ``Array``, ``TextIO``, ``CommandLine``, ``OS``, and ``Timer``;
* multi-file projects through MLWorks ``require "unit"`` declarations;
* launcher executable creation with ``mlw.exe exe``;
* heap-delivered executable creation with ``mlw.exe deliver``;
* opt-in Foreign Interface support via ``MLWORKS_FOREIGN=1``;
* repeatable launcher, runtime, and package build recipes.

Differences from current SML systems
------------------------------------

MLWorks comes from a different generation of SML implementation than MLton,
SML/NJ, Moscow ML, Poly/ML, or modern package-oriented compiler workflows.
Some practical differences show up immediately:

* The reboot package is Windows/i386. Generated code and runtime assumptions
  are 32-bit.
* The source tree contains historical target support for several platform pairs,
  including I386/NT, I386/Win95, I386/Linux, SPARC/SunOS, SPARC/Solaris, and
  MIPS/Irix. This fork currently exercises the Windows/i386 path.
* The runtime is old C code with manual memory management, tagged values,
  fixed-address assumptions, direct pointer manipulation, and platform-specific
  object/image handling. Modern platforms make those assumptions more fragile
  than they were on the original target systems.
* MLWorks was built around projects, images, and an interactive development
  environment. The CLI still needs to manage project data such as ``.mlp``
  files, targets, object locations, modes, configurations, and dependency
  lists.
* The batch compiler is a saved MLWorks image. ``mlw-compiler.exe`` is an
  executable image containing compiler state, not a normal C ``main`` linked
  directly from compiler sources.
* Build products are MLWorks object files (``.mo``) plus runtime load lists.
  ``mlw.exe`` turns those into a direct ``run`` command, a launcher
  executable, or a delivered executable.
* The Basis Library exists as MLWorks project/source/object units. ``mlw.exe``
  wires those units into small projects automatically.
* Executable delivery saves an ML heap into a Windows executable. On Windows,
  delivered programs travel with ``libmlw.dll``.
* The current MLB support is a convenience manifest format for editor and
  small-project workflows. It reads file lists and nested manifests, then maps
  them onto MLWorks project builds.

Split programs
--------------

MLWorks source units use the source filename minus ``.sml`` as the unit name.
Use ``require`` when one unit depends on another.

Example ``greeting.sml``:

::

    structure Greeting =
      struct
        fun line name =
          "Hello, " ^ name ^ " from a required unit\n"
      end

Example ``main.sml``:

::

    require "greeting";

    fun main () =
      print (Greeting.line "MLWorks")

    val _ = main ()

Example ``main.mlb``:

::

    greeting.sml
    main.sml

The current MLB support reads comments, plain ``.sml`` entries, and nested
``.mlb`` entries. The final ``.sml`` entry becomes the target unit.

Portable Windows package
------------------------

Build the Windows/i386 package with:

::

    tools\package-mlw.cmd

The script writes:

::

    dist\mlworks-reboot-win32\
    dist\mlworks-reboot-win32.zip

The package contains:

* ``mlw.exe`` and ``mlw.bat``;
* the saved batch compiler executable;
* runtime DLLs and runtime images;
* examples;
* Basis and Foreign Interface sources;
* prebuilt Basis and Foreign object caches.

The package layout lets ``mlw.exe`` find its compiler support files inside the
unpacked folder.

Delivered executables
---------------------

``mlw.exe deliver`` uses MLWorks' executable delivery path. It saves the ML heap
into the generated Windows executable and copies ``libmlw.dll`` next to it.

Example:

::

    mlw.exe deliver examples\modules\main.mlb examples\modules\main.exe

The result is:

::

    main.exe
    libmlw.dll

The MLWorks Windows runtime is DLL-based, so ``libmlw.dll`` is part of the
delivered program's portable output.

Reboot architecture
-------------------

The compiler path is image based. The important pieces are:

``MLWorks-reboot\mlw.exe``
    A small C launcher for the modern workflow. It writes temporary ``.mlp``
    projects, invokes the batch compiler image, writes runtime object lists,
    and runs or packages the result.

``MLWorks-reboot\compiler\mlw-compiler.exe``
    A saved MLWorks batch compiler image. MLWorks loads compiler state from an
    image and saves that image as an executable.

``MLWorks-reboot\bin\I386\NT\main.exe`` and ``libmlw.dll``
    The runtime launcher and runtime DLL used to run compiled objects.

``MLWorks-reboot\images\I386\NT\pervasive-test.img``
    The small runtime image used by the command-line path.

``MLWorks-reboot\lib``
    The portable package's copy of Basis/Foreign sources and prebuilt object
    caches.

The batch compiler is project-oriented. ``mlw.exe`` generates a minimal
MLWorks project and drives the compiler through the project build path. That
matches the compiler's working model and gives the CLI enough information to
compile source units, resolve ``require`` dependencies, and dump the runtime
object load order.

Why ``mlw``?
------------

``mlw`` is a compatibility layer between the MLWorks runtime model and a small
everyday compiler command.

It gives names to the common operations:

* ``build``: compile a source file or MLB manifest;
* ``run``: compile and execute a program;
* ``exe``: create a launcher executable plus support directory;
* ``deliver``: create a heap-delivered executable plus ``libmlw.dll``;
* ``basis``: refresh the Basis object load list;
* ``foreign``: refresh Foreign Interface support objects.

It also keeps generated compiler state under ``.mlw``:

::

    .mlw\mlw-generated.mlp
    .mlw\mlw-project-objects.txt
    .mlw\objects\i386\nt\release\<unit>.mo

Building from source
--------------------

For development, the checked-in recipes are:

::

    tools\reboot-mlw.cmd
    tools\reboot-rts.cmd
    tools\package-mlw.cmd

``reboot-mlw.cmd`` rebuilds ``MLWorks-reboot\mlw.exe`` and runs smoke tests.
``reboot-rts.cmd`` rebuilds and packages the Windows RTS artifacts.
``package-mlw.cmd`` creates the portable Windows folder and zip.

The runtime rebuild currently uses MSVC plus the old makefile flow and helper
Unix-like tools. The packaged reboot gives users the compiled result directly,
so using SML starts with ``mlw.exe``.

Current status
--------------

* The reboot package targets Windows/i386.
* ``mlw.exe deliver`` expects a ``main : unit -> 'a`` binding.
* ``mlw.exe deliver`` strips a simple top-level ``val _ = main ()`` launcher
  from its private delivery copy. Other top-level effects run while the image
  is packaged.
* Run one ``mlw`` command at a time per source directory. Commands share the
  generated ``.mlw`` project state.
* Path-with-spaces support needs more testing in the old runtime argument
  parser.
* This registry warning is expected:

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

This fork builds on that open-source release and continues the reboot as a
portable Windows command-line SML system.
