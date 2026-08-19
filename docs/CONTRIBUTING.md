# Contributing to augra-log

Contributions are welcome. This document covers the basics.

## AI usage

Contributions are judged on their merit. If you used AI tools in the
process, mention it in the PR description. No `.claude/` rules or AI
configuration files should be included in contributions.

However, it is required to state the used model and version, as well 
as that you as a human developer read through it, understood it, 
reviewed and tested it. Additional, it is required that you declare that
you take full responsibility for the generated output.

## Bug reports

Use the bug report issue template. Include the version you are using,
your OS, and a minimal reproducer if possible. Compiler and CMake version
help narrow things down.

## Contributing code

Build the project with tests enabled and make sure everything passes
before submitting:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DAUGRA_LOG_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Follow the existing code style. The project compiles with
`-Wall -Wextra -Wpedantic -Werror=return-type` and your code should
too, without warnings. C++17.

New functionality should come with tests. If you are fixing a bug, add
a test that would have caught it.

Commit messages: terse subject line saying what changed, optional short
body explaining why. Keep commits focused on one thing.

To submit: fork the repository, create a branch, and open a pull request
against `main`.

## Documentation

Fixing typos, improving examples, and clarifying explanations are all
valuable contributions. The docs live in the `docs/` folder.
