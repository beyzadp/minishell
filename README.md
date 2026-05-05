# minishell

Minimal, educational shell implementation in C. Supports simple pipelines and
basic input/output redirection with a small custom tokenizer.

## Features
- Tokenizes a single input line into commands, arguments, and operators
- Executes pipelines using fork/exec and pipes
- Input redirection with `<` and output redirection with `>`
- Lightweight helpers provided by the `libc_mini` submodule

## Build
Requires a C compiler and POSIX environment.

```sh
make
```

The binary is produced at `build/minishell`.

## Run
```sh
./build/minishell
```

Enter a single command line when prompted.

## Example
```text
$> ls -l | grep "minishell" > out.txt
```

## Project Layout
- `src/` core shell logic
- `include/` public headers
- `libc_mini/` helper library (git submodule)
- `build/` build output

## Notes
- This project runs an input loop until EOF or Ctrl+C.
- Redirection uses standard shell permissions (0644) for created files.

## License
MIT. See `LICENSE`.
