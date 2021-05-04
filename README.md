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
    int const a = section["a"] | 0;
    double const b = section["b"] | 0.0;
    std::string const c = section["c"] | "";
    std::string const d = section["d"] | "";
    bool const e = section["e"] | false;
    return 0;
}
```

### Read arrays

```cpp
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
    std::string const data = section["data"];
    if(!data.is_array())
        return -1;
    std::vector<int> vector;
    for(std::size_t i = 0; i != data.size(); ++i)
        vector.push_back(data[i] | 0);
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
    if(!anubis.is_table())
        return -1;
    std::string const name = anubis["name"] | "";
    std::string const sex = anubis["sex"] | "";
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
