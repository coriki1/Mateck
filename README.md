# Mateck

A small symbolic math engine in C++17. It parses expressions written in everyday math notation, simplifies and expands them, differentiates them symbolically and evaluates them with substituted values — with its own lexer, parser and expression tree. The math engine depends only on the standard library; the terminal UI uses [FTXUI](https://github.com/ArthurSonzogni/FTXUI).

```
╭ Expression ──────────────────────────────────────────────────────────────────────────────────────╮
│> x^3 - 2x                                                                                        │
╰──────────────────────────────────────────────────────────────────────────────────────────────────╯
╭ Derivative ────────────────────────────────────╮╭ Tree   Plot ───────────────────────────────────╮
│by  x    order  1   2   3                       ││⡀                       ⡇                      ⢀│
│                                                ││⠘⡄                      ⡇                     ⢀⠎│
│f              x^3 - 2x                         ││ ⠘⢄                     ⡇                    ⢠⠊ │
│d/dx           3x^2 - 2                         ││  ⠈⢆                    ⡇                   ⡠⠃  │
│                                                ││    ⠣⡀                  ⡇                  ⡔⠁  ⢠│
│value          -1                               ││     ⠑⡄                 ⡇                ⢀⠎   ⡠⠃│
│d/dx value     1                                ││      ⠈⢢                ⡇               ⡰⠁   ⡰⠁ │
╰────────────────────────────────────────────────╯│        ⠑⢄              ⡇             ⢠⠊   ⢀⠜   │
╭ Variables ─────────────────────────────────────╮│         ⠈⠢⡀            ⡇           ⢀⠔⠁   ⡠⠊    │
│x = 1                                           ││           ⠈⠢⢄          ⡇         ⡠⠔⠁   ⡠⠊      │
╰────────────────────────────────────────────────╯│⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣤⣒⣒⣙⣫⣉⣉⣑⣒⣒⣢⣤⣄⣇⣀⣀⣀⣀⣀⣀⣀⣔⣊⣀⣀⣀⣠⣔⣊⣀⣀⣀⣀⣤⣤⣤⣤│
╭ History ───────────────────────────────────────╮│       ⡠⠔⠉       ⠉⠒⠤⢄⣀⣀⣀⣇⣈⣉⣩⠶⠶⠿⠶⠒⠺⠒⠛⠉⠉⠉⠉        │
│                                                ││     ⢠⠊⢀⣀⣀⣀⡠⠤⠤⠤⠔⠒⠒⠒⠊⠉⠉⠉⠁⡇                       │
│                                                ││⠒⠒⠒⠊⡝⠉⠉⠁                ⡇                       │
│                                                ││  ⢀⠎                    ⡇                       │
│                                                ││ ⢠⠃                     ⡇                       │
│                                                ││⢠⠃                      ⡇                       │
│                                                ││⠁                       ⡇                       │
│                                                ││x: -2.5 .. 2.5   y: -11.99 .. 18.12             │
│                                                ││── f  ── d/dx  ── tangent                       │
╰────────────────────────────────────────────────╯╰────────────────────────────────────────────────╯
 Tab focus   ←→ choose   ↑↓ history   F2 tree/plot   PgUp/PgDn zoom   Esc menu                 v0.1
```

The terminal UI opens with a main menu leading to two screens, **Evaluation and expansion** and **Symbolic differentiation**. Both update while you type: results on the left, variable values below them, and on the right the expression tree or a plot (F2). The differentiation screen picks the variable and the order (up to 3), shows the gradient of multivariable expressions and draws the function, its derivative and the tangent at the given point.

Without a terminal, or with `--plain`, Mateck runs a line-based mode instead:

```
$ ./mateck --plain
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

After each expression it asks for the value of every variable; an empty answer leaves the variable in place.

## Features

- **Natural input syntax** — implicit multiplication (`2x`, `x(x + 1)`), function calls without parentheses (`sin x^2`), powers of functions (`sin^2 x`), constants `pi` and `e`
- **Simplification** — merges like terms and factors across nested expressions (`x + 1 - x` → `1`, `3/(3x)` → `1/x`), keeps exact fractions (`1/2 + 1/3` → `5/6`, `0.5x` → `x/2`) and brings equal expressions to the same canonical form, ordered like a polynomial (`x^3 - 2x`)
- **Expansion** — multiplies out products and powers of sums (`(x + 1)(x - 1)` → `x^2 - 1`), with a limit so `(x + 1)^1000` stops instead of running for minutes
- **Symbolic differentiation** — sum, product, quotient, power and chain rules for every supported function
- **Substitution** — numbers or variable-free expressions (`pi/2`, `sqrt 2`) for some or all variables; a partial substitution gives a simplified expression
- **Readable output** — compact notation (`2x cos(x^2)`, `5x/6`) that can always be pasted back as input
- **Error messages with positions** — `Expected ')' but found end of input at column 7`, marked with a `^` under the input in the terminal UI
- **Terminal UI** — live results, the expression tree, braille plots with zoom, higher derivatives, gradients and tangents, input history

## Building and running

Requirements: a C++17 compiler (g++), `make` and FTXUI 7, found through `pkg-config`. On Fedora:

```bash
sudo dnf install ftxui-devel
make                # builds ./mateck
./mateck            # terminal UI, quit with Esc on the main menu
./mateck --plain    # line-based mode, quit with Ctrl+D
make clean          # removes build output
```

### Tests

The tests use [GoogleTest](https://github.com/google/googletest), found through `pkg-config`. On Fedora:

```bash
sudo dnf install gtest-devel
make test                                     # builds and runs all tests
./mateck_tests --gtest_filter='Simplify.*'    # run one group
```

`make` alone works without GoogleTest installed, and the tests do not need FTXUI.

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
| Output | `src/format.cpp`, `src/tree.cpp` | user-facing notation; `toString()` gives a fully explicit form for debugging; `drawTree()` draws the expression tree |
| Session | `src/session.cpp` | everything a UI screen shows, computed from the typed text: results, derivatives, gradient, plot samples; free of FTXUI so it is unit-tested |
| Terminal UI | `src/tui/` | FTXUI screens that only draw what the session layer computed |
| Line-based mode | `src/repl.cpp` | reads from any `std::istream`, so the whole dialog is unit-tested |

## Testing

126 GoogleTest tests cover the lexer, parser, every node type, simplification, expansion, formatting, the tree drawing, the session layer behind the terminal UI and the line-based dialog. Besides fixed examples, randomized tests check properties over thousands of generated expressions:

- the formatted output parses back to the same expression,
- `simplify()` and `expand()` keep the value,
- simplifying twice changes nothing,
- `compare()` is a strict total order consistent with structural equality.

## Limitations

- Simplification is algebraic only: it knows no trigonometric or logarithm identities, so `sin^2 x + cos^2 x` does not become `1`.
- `sqrt(x)` and `x^(1/2)` are treated as different expressions.
- Decimals become exact fractions only for small denominators (`0.25` → `1/4`, but `9.81` stays).
- Identities are applied without checking the domain, like most computer algebra systems do: `x/x` becomes `1` although it is undefined at `x = 0`.
- The line-based mode differentiates by `x` only; the terminal UI can choose the variable.

## Roadmap

- [x] Terminal UI with menus, the expression tree and function plots
- [ ] Numerical integration (already in the main menu as "coming soon")
- [ ] Command-line mode with flags (`mateck -d x "x^3 - 2x"`) for scripts

## License

[GPL-3.0](LICENSE)
