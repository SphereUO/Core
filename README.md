# SphereUO
[![Build](https://github.com/SphereUO/Core/actions/workflows/build.yml/badge.svg)](https://github.com/SphereUO/Core/actions/workflows/build.yml)
[![GitHub issues](https://img.shields.io/github/issues/SphereUO/Core.svg)](https://github.com/SphereUO/Core/issues)
<br>
[![GitHub stars](https://img.shields.io/github/stars/SphereUO/Core?logo=github)](https://github.com/SphereUO/Core/stargazers)
[![GitHub repo size](https://img.shields.io/github/repo-size/SphereUO/Core.svg)](https://github.com/SphereUO/Core/)
[![GitHub last commit](https://img.shields.io/github/last-commit/SphereUO/Core.svg)](https://github.com/SphereUO/Core/)

SphereUO aims to enhance Ultima Online mechanics across different eras while providing extensive customization through its scripting language (SCP).

Join to our Discord server
<br>
[![Discord Shield](https://discordapp.com/api/guilds/354358315373035542/widget.png?style=shield)](https://discord.gg/YkwwzwJGam)

### Downloads
+ <a href="https://github.com/SphereUO/Scripts-X">Scripts</a>

## Key Changes
+ Core changes based on ERA or custom.
+ Bug fixes and substantial internal behavior improvements.
+ Refactoring of some behaviors to be more UO standards based on era

## Getting Started

### Required Libraries

+ **Windows:** `libmariadb.dll` (MariaDB Client v10.* package), found in `lib/bin/*cpu_architecture*/mariadb/libmariadb.dll`.
+ **Linux:** MariaDB Client library. Install it from [MariaDB website](https://mariadb.com/docs/skysql/connect/clients/mariadb-client/) or your distribution's repositories.
+ **MacOS:** Install MariaDB Client library via `brew install mariadb-connector-c`.

### Building

Ensure you have a recent compiler (Visual Studio 2015 Update 3 or later, GCC 7.1 or later, Clang 6 or greater) as C++17 features are used. Build Makefiles or Ninja files with CMake for both Linux (GCC) and Windows (MSVC and MinGW).

Example for building makefiles on Linux for a 64-bit Nightly version:

```bash
mkdir build
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/Linux-GNU-x86_64.cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Nightly" -B ./build -S ./
```

## Compiling
### Linux
Use make -jX to speed up compilation with X threads.

```bash
make -jX -C build
```

### Sanitizers

Enable Address Sanitizer (ASan) and Undefined Behaviour Sanitizer (UBSan) with the ENABLE_SANITIZERS checkbox via GUI or -DENABLE_SANITIZERS=true via CLI.

Retrieve ASan output by launching Sphere from cmd or shell:

```bash
SphereSvrX64_nightly > Sphere_ASan_log.txt 2>&1
```

## Coding Guidelines
- Test code before committing.
- Prefer rebasing over pulling to avoid unnecessary "merge branch master" commits.
- Preserve backwards compatibility when removing/changing/adding features.
- Comment code for better understanding.
- Use Sphere's custom datatypes and string formatting macros.
- Prefer C-style casts for numeric types.
- Use signed 64-bit integers for values intended to be printed or retrieved by scripts.

## Naming Conventions
- Prefix pointer variables with "p".
- Prefix unsigned variables with "u".
- Prefix boolean variables with "f" (flag).
- Classes should have uppercase first letters and prefix "C".
- Private/protected methods and members of a class or struct should have the prefix "_".
- Constants should have the prefix "k" (static const class members).

## Variables meant to hold numerical values:
- Use prefixes: "i" (integer), "b", "w", "dw" for byte, word, and dword respectively.
- Use prefixes "r" for float and double.

## Variables meant to hold characters (strings):

- Use prefixes "c", "wc", "tc" for char, wchar, tchar.
- Prefer "lpstr", "lpcstr", "lpwstr", "lpcwstr", "lptstr", "lpctstr" aliases.
- Use "s" or "ps" (if pointer) when using CString or std::string.

## Coding Style
- Indent with spaces of size 4.
- Use Allman indentation style.

## Thanks
+ Sphereserver team and community
