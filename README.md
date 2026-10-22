# Mateck

A small symbolic math engine in C++17. It parses expressions written in everyday math notation, simplifies and expands them, differentiates them symbolically and evaluates them with substituted values — with its own lexer, parser and expression tree, and no dependencies outside the standard library.

```
$ ./mateck
> x^3 - 2x
  parsed:           x^3 - 2x
  simplified:       x^3 - 2x
  expanded:         x^3 - 2x
  d/dx:             3x^2 - 2
  x = 2
  value:            4
  d/dx value:       10
> (x + 1)^2
  parsed:           (x + 1)^2
  simplified:       (x + 1)^2
  expanded:         x^2 + 2x + 1
  d/dx:             2(x + 1)
  x =
> sin(x + y)
  ...
  x = 2
  y =
  substituted:      sin(y + 2)
  d/dx substituted: cos(y + 2)
```

After each expression the program asks for the value of every variable; an empty answer leaves the variable in place.

## Features

- **Natural input syntax** — implicit multiplication (`2x`, `x(x + 1)`), function calls without parentheses (`sin x^2`), powers of functions (`sin^2 x`), constants `pi` and `e`
- **Simplification** — merges like terms and factors across nested expressions (`x + 1 - x` → `1`, `3/(3x)` → `1/x`), keeps exact fractions (`1/2 + 1/3` → `5/6`, `0.5x` → `x/2`) and brings equal expressions to the same canonical form, ordered like a polynomial (`x^3 - 2x`)
- **Expansion** — multiplies out products and powers of sums (`(x + 1)(x - 1)` → `x^2 - 1`), with a limit so `(x + 1)^1000` stops instead of running for minutes
- **Symbolic differentiation** — sum, product, quotient, power and chain rules for every supported function
- **Substitution** — numbers or variable-free expressions (`pi/2`, `sqrt 2`) for some or all variables; a partial substitution gives a simplified expression
- **Readable output** — compact notation (`2x cos(x^2)`, `5x/6`) that can always be pasted back as input
- **Error messages with positions** — `Expected ')' but found end of input at column 7`

## Building and running

Requirements: a C++17 compiler (g++) and `make`. Only the tests need an extra library, GoogleTest.

```bash
make            # builds ./mateck
./mateck        # interactive mode, quit with Ctrl+D
make clean      # removes build output
```

### Tests

The tests use [GoogleTest](https://github.com/google/googletest), found through `pkg-config`. On Fedora:

```bash
sudo dnf install gtest-devel
make test                                     # builds and runs all tests
./mateck_tests --gtest_filter='Simplify.*'    # run one group
```

`make` alone works without GoogleTest installed.

## Input syntax

| Syntax | Meaning |
|---|---|
| `+ - * / ^` | `^` binds tightest and is right-associative: `2^3^2` = `2^(3^2)` |
| `-x^2` | `-(x^2)` |
| `2x`, `xy`, `2sin(x)`, `(x + 1)(x - 1)` | implicit multiplication, same precedence as `*`, so `1/2x` = `(1/2)x` |
| `sin(x)`, `sin x` | function call; without parentheses the argument runs to the next `+ - * /` or function: `sin x^2` = `sin(x^2)`, `sin x cos x` = `sin(x) cos(x)` |
| `sin (x + 1)^2` | parentheses right after the name are the argument: `sin(x + 1)^2` |
| `sin^2 x` | `sin(x)^2`; `sin^-1 x` is rejected as ambiguous (use `arcsin x`) |
| `pi`, `e` | constants, kept symbolic (`2pi` stays `2pi`) |
| `.5`, `3.14` | numbers; no scientific notation, so `1e5` means `1 · e · 5` |

Variables are single letters, so `xy` is `x · y`.

**Functions:** `sin cos tan cot`, `arcsin arccos arctan arccot`, `sinh cosh tanh coth`, `asinh acosh atanh acoth`, `ln log sqrt abs`, `floor ceil round` (the last three can be evaluated but not differentiated).

## How it works

```
"2x sin x^2"
     │  Lexer      → NUMBER(2) VARIABLE(x) FUNCTION(sin) VARIABLE(x) OPERATOR(^) NUMBER(2) END
     │  Parser     → expression tree:        *
     ▼                                      / \
  Node tree                                *   sin
     │                                    / \    \
     │                                   2   x    ^
     │                                           / \
     │                                          x   2
     ├─ simplify()        canonical form
     ├─ expand()          multiplied out
     ├─ differentiate()   symbolic derivative
     ├─ substitute()      variables replaced
     └─ evaluate()        number
     │
     ▼  format()          "2x sin(x^2)"
```

| Part | Files | Notes |
|---|---|---|
| Lexer | `src/lexer.cpp` | splits letter runs into variables, longest match for function and constant names (`sinhx` → `sinh x`) |
| Parser | `src/parser.cpp` | recursive descent, one function per precedence level; the grammar is documented in `include/parser.h` |
| Expression tree | `src/node_*.cpp` | numbers, variables, constants, unary and binary operators, functions; each node can evaluate, differentiate, substitute and print itself |
| Simplification | `src/simplify.cpp`, `src/compare.cpp` | flattens `+`/`−` and `*`/`/` chains into lists, merges equal terms and factors, then rebuilds them in the order defined by `compare()` |
| Expansion | `src/expand.cpp` | distributes products over sums, merging terms after every step |
| Output | `src/format.cpp` | user-facing notation; `toString()` gives a fully explicit form for debugging |
| Interactive mode | `src/repl.cpp` | reads from any `std::istream`, so the whole dialog is unit-tested |

## Testing

101 GoogleTest tests cover the lexer, parser, every node type, simplification, expansion, formatting and the interactive dialog. Besides fixed examples, randomized tests check properties over thousands of generated expressions:

- the formatted output parses back to the same expression,
- `simplify()` and `expand()` keep the value,
- simplifying twice changes nothing,
- `compare()` is a strict total order consistent with structural equality.

## Limitations

- Simplification is algebraic only: it knows no trigonometric or logarithm identities, so `sin^2 x + cos^2 x` does not become `1`.
- `sqrt(x)` and `x^(1/2)` are treated as different expressions.
- Decimals become exact fractions only for small denominators (`0.25` → `1/4`, but `9.81` stays).
- Identities are applied without checking the domain, like most computer algebra systems do: `x/x` becomes `1` although it is undefined at `x = 0`.
- The interactive mode differentiates by `x` only.

## Roadmap

- [ ] Command-line mode with flags (`mateck -d x "x^3 - 2x"`) for scripts
- [ ] Numerical integration
- [ ] Terminal UI with menus and function plots

## License

[GPL-3.0](LICENSE)
