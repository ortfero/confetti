# confetti

C++17 one-header library to parse ini files with some toml extensions.


## Info

* Support for arrays, tables, inline tables
* Keys and section names are case insensitive (downcased)
* Keys are ASCII-only, but values can be UTF-8
* Zero allocation parser
* No dependencies


## Snippets

### Open config

```cpp
#include <cstdio>
#include <confetti/confetti.hpp>

int main() {
    confetti::result const parsed = confetti::parse("example.ini");
    if(!parsed) {
        std::printf("Error at line %d: %s\n",
                    parsed.line_no,
                    parsed.error_code.message().data());
        return -1;
    }

    return 0;
}
```

### Read basic properties

```cpp
#include <optional>
#include <confetti/confetti.hpp>

int main() {
    confetti::result const parsed = confetti::parse_text(
        "a = -1\n"
        "b = 3.14\n"
        "c = value\n"
        "d = 'string value'\n"
        "e = false\n");
    if(!parsed)
        return -1;
    confetti::value const& section = parsed.config["default"];
    std::optional<int> const a = section["a"] | 0;
    std::optional<double> const b = section["b"] | 0.0;
    std::optional<std::string> const c = section["c"] | "";
    std::optional<std::string> const d = section["d"] | "";
    std::optional<bool> const e = section["e"] | false;
    return 0;
}
```

### Read arrays

```cpp
#include <optional>
#include <vector>
#include <confetti/confetti.hpp>

int main() {
    confetti::result const parsed = confetti::parse_text(
        "[section]\n"
        "data = [1, 2, 3, 4]\n"
    );
    if(!parsed)
        return -1;
    confetti::value const& section = parsed.config["section"];
    std::optional<std::vector<int>> const data = section["data"] | std::vector<int>{};
    return 0;
}
```

### Read tables

```cpp
#include <vector>
#include <confetti/confetti.hpp>

int main() {
    confetti::result const parsed = confetti::parse_text(
        "[gods]\n"
        "anubis = {name = 'Anubis', sex = male}\n"
    );
    if(!parsed)
        return -1;
    confetti::value const& gods = parsed.config["gods"];
    confetti::value const& anubis = gods["anubis"];
    std::optional<std::string> const name = anubis["name"] | "";
    std::optional<std::string> const sex = anubis["sex"] | "";
    return 0;
}
```

### Check section contains property

```cpp
#include <confetti/confetti.hpp>

int main() {
    confetti::result const parsed = confetti::parse_text("");
    if(!parsed)
        return -1;
    bool const key_exists = parsed.config["default"].contains("key");
    return 0;
}
```

## Tests

To build tests:

```shell
cd confetti
mkdir build
cd build
meson ../tests
ninja
```

## Installation

Drop `confetti/*` somewhere at include path.


## Supported platforms and compilers

confetti requires C++ 17 compiler.


## License

confetti licensed under [MIT license](https://opensource.org/licenses/MIT).
