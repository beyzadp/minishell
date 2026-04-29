# pipex

`pipex` is a C project that recreates a basic shell pipeline.

It runs two commands in sequence, connecting them through a pipe, while handling
input/output redirection with files.

## Concept

Program call:

```sh
./build/pipex infile "cmd1" "cmd2" outfile
```

Shell equivalent:

```sh
< infile cmd1 | cmd2 > outfile
```

To implement this behavior, the project uses low-level Unix syscalls such as
`pipe`, `fork`, `dup2`, `execve`, and `waitpid`.

## Directory Structure

- `include/` headers and shared declarations
- `src/` source files
- `tests/` manual test files and scripts
- `build/` generated objects and executable

## Build And Run

From `pipex/`:

```sh
make
./build/pipex infile "cat" "wc -l" outfile
```

## Make Targets

- `make` or `make all` builds `build/pipex`
- `make test` builds and runs `./build/pipex`
- `make clean` removes all files inside `build/`
- `make fclean` runs `clean` and removes the binary
- `make re` rebuilds from scratch

## Notes

- This folder currently contains a working scaffold.
- Pipeline parsing, PATH resolution, command execution, and error handling should
  be implemented in `src/`.
