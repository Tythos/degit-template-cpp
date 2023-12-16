## Background

There's a saying in political circles: "Libertarians agree on only two things: First, that there is only one true libertarian. Second, that it's them." (I suppose they have that in common with cats.)

I program in a *lot* of different languages, and this always reminds me of C/C++. What does a project look like in these languages? No one knows! Everyone has their own standard and expectations, and everyone agrees that everyone else's approach is unreasonable and messy.

* https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1204r0.html

* https://hiltmon.com/blog/2013/07/03/a-simple-c-plus-plus-project-structure/

* https://softwareengineering.stackexchange.com/questions/81899/how-should-i-organize-my-source-tree

* https://cliutils.gitlab.io/modern-cmake/chapters/basics/structure.html

![everyone wants to be an exception](https://dev-to-uploads.s3.amazonaws.com/uploads/articles/r0o3lqza45u3ftlzu9dn.jpeg)

..and what might be the most cursed idea of them all, courtesy of a Stack Overflow comment I will leave anonymous:

![Java build structures](https://dev-to-uploads.s3.amazonaws.com/uploads/articles/ymmmifw8g2n8jl4dv35a.png)

No! Don't do it! Look away before it's too late!

(And less we give C/C++ a bad wrap--have you seen the Python packaging standards lately? Talk about downhill. There's always been something particularly grating about having a folder within a folder with the same names just to convince the `import` statements to work without hacks. There's a `$PYTHONPATH` people, use it already! Thank God for [editable pip-installs](https://setuptools.pypa.io/en/latest/userguide/development_mode.html).)

Add to this problem the perennial "how do we build this again?" question (particular with cross-platform concerns!) and you've got a real mess on your hands. To say nothing of packaging and dependency resolution (without buying into Conan, VCPKG, or similar).

Let's make it messier! Here's a few heuristics I've found that *greatly* help consolidate my C/C++ projects into a flexibly-buildable stack of code. It's worked out pretty well for me, maybe it will help you too.

## Heuristics

Here's our priorities:

1. There should be an upper limit on what goes into a project. Tier upon tier of subfolder structures make `#include` statements nearly impossible to decipher w/o some serious CMake magic.

2. Talk trash about Python, but do you remember when you realized there was a cognitive isomorphism between modules/packages on one hand, and files/folders on the other?

3. Namespaces are great. We should use those. Preferably in combination with the above.

4. Platform-agnostic builds are a must. Seamless support for the Big Three compiler systems (gcc, clang, msvc) drives us to CMake. BUT: CMake is an incredibly complex ecosystem (practically a language to itself). We want to use the bare minimum subset (10% gets you 90%, honestly) because we're here to code in C/C++.

5. We need an agnostic way to track dependencies. We'll use git submodules here. Not only does this let us offload *a lot* of our metadata (version specification / locking, recursive clones, etc.), we can combine this with some clever CMake lines to build arbitrary hierarchies of dependencies. And not needing to worry about a third-party tool (we're using git already... right?) is *NICE*.

6. We'd like to avoid sensitivities to other context, like environmental variables for resolving `#include` (beyond system libraries of course). Self-contained as much as possible (particularly possible with #5 above!).

7. Redundant information is horrific. You're asking for trouble if you introduce a failure mode from (for example) mismatched project names, etc. Whenever possible, a project property should have a single authoritative definition that other references can use.

![python logical filestructure isomorphisms](https://dev-to-uploads.s3.amazonaws.com/uploads/articles/yho6bf5vyp42jxwtbp5c.jpeg)

## Getting Started

Okay, so where does this leave us? I've written about a similar approach before, so let's use this as a scaffolding.

https://dev.to/tythos/basic-c-unit-testing-with-gtest-cmake-and-submodules-4767

But before we dive in too deep, let's clarify two specific use cases:

* Building a top-level application (e.g., .EXE)

* Building a static library (e.g., .LIB)

(Forgive me for using Windows file extensions here--but if you're on another operating system you probably know what the equivalents are already!)

We'll focus on the second use case. Let's assume for the time begin that top-level applications will primarily consist of argument parsing, STDIN/STDOUT/STDERR handling, and setting up specific calls to a runtime library of some kind. The second use case is considerably more complicated (in terms of the scale of a project), so if we tackle that one first, the second one will be trivial.

So, let's create a few things, using an empty folder `mylibary` (can you tell what our project name is?) to begin from scratch. And while we're focusing on a library, we'll toss in a placeholder `main.cpp` just to get us going. That means three placeholder files for our happy little MVP:

* `.gitignore`, which just has a single line ("`build/`") to prevent us from committing our CMake artifacts to version control.

* `CMakeLists.txt`, which defines a minimum version; project name; and our test entry point as an executable. The initial content will look something like this:

```cmake
cmake_minimum_required(VERSION 3.14)
project(mylibrary)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(${PROJECT_NAME} "main.cpp")
```

Note the use of `${PROJECT_NAME}`, consistent with heuristic #7 from above. Ideally, we'll parse this from the folder name at some point. It's also useful to specify a C++ standard straight out--let developers know what the *tooling* dependencies are, and what features they can use/expect.

* `main.cpp`, which just holds a basic `cout` to ensure our environment is resolving system libraries.

```cpp
/**
 * main.cpp
 */

#include <iostream>

int main(int nArgs, char** vArgs) {
    std::cout << "Farewell, cruel world!" << std::endl;
    return 0;
}
```

_(Note: I find a top-level comment letting you know what context the file exists in can be invaluable.)_

This is enough for us to try a CMake build and make sure our tooling is all set up correctly:

```sh
$ cmake -S . -B build
$ cmake --build build
$ build/Debug/mylibrary.exe
```

(Your specific output artifact path may change with platform; check your build artifacts.)

And honestly, this is already enough for us to have a nicely reusable bootstrap-into-empty-CPP project template. But let's keep going.

## Dependencies

Let's add a dependency. Specifically, since we want testing anyway, let's cite `googletests` so the build structure is self-contained (see Heuristic #6). Since we're using git submodules for dependencies, this is actually pretty straightforward. First, let's add and initialize the dependency:

```sh
$ git submodule add https://github.com/google/googletest.git
$ git submodule update --init --recursive
```

Adding this to our build is a little redundant with the previous article I wrote, so let's walk through a minimal usage. First, we need to setup the test hooks within our CMakeLists.txt file:

```cmake
# one-time test configuraiton
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory("googletest/" EXCLUDE_FROM_ALL)
enable_testing()
```

Let's add a basic test in a "tests/" subfolder. It's just going to assert that `2 + 2 = 4`, but there's nothing that will prevent it from including and testing top-level project artifacts.

```cpp
/**
 * tests/test_addition.cpp
*/

#include "gtest/gtest.h"

namespace mylibrary {
    namespace tests {
        namespace test_addition {
            TEST(TestAddition, BasicTest) {
                ASSERT_TRUE(2 + 2 == 4);
            }
        }
    }
}

int main(int nArgs, char** vArgs) {
    ::testing::InitGoogleTest(&nArgs, vArgs);
    return RUN_ALL_TESTS();
}
```

The `main()` definition here is included to help us run individual tests from their executables should the package-level tests lack sufficient detail for us to resolve any potential issues.

But most importantly: Namespaces are the key to Heuristic #2. From a top-level project namespace, we define a namespaces for each "module" (file) and "subpackage" (folder). So, in this case, we know within the code that referencing `mylibrary::tests::test_addition` maps to symbols defined within the `mylibrary` package, where there's a `tests` subfolder that has a `test_addition.cpp` file with those contents. I can't tell you how this will change your C/C++ programming life (well, I guess just C++ now since we're using namespaces). I swear (even Rust hasn't caught onto this!), the gap between logical and filestructure relationships is such a huge cognitive pain.

## Adding Tests

Let's revisit our CMakeLists.txt file now. We want a way to define the build of our tests--but there's a lot of repetition for multiple tests (and each test module should probably evaluate the implementation of a specific source module, so there could be a lot of test modules). Each one has to be:

* Added as an executable to the build output

* Given include directories

* Linked against the test framework (and any other dependencies)

* Passed to the `add_test()` method from gtest

So, let's define a function for adding these test hooks in our `CMakeLists.txt` file:

```cmake
# define test hooks
function(add_gtest test_name test_source)
    add_executable(${test_name} ${test_source})
    target_include_directories(${test_name} PUBLIC ${CMAKE_SOURCE_DIR})
    target_link_libraries(${test_name} gtest gtest_main)
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()
```

This turns out to be a *nicely* reusable approach. For our sample test from above, for example, this just needs an extra line after the function is defined:

```cmake
# add individual test modules
add_gtest(test_equations "tests/test_addition.cpp")
```

You can now test your tests! It's pretty slick, actually:

```sh
$ cmake -S . -B build
$ cmake --build build
$ ctest --test-dir build -C Debug
```

## Structuring the Library

Now for the heavy lifting. Let's define (for purposes of multiple implementations) a "Food" class and a "Cookies" class. BUT: we're going to use a single header file. Implementations can be broken up as modules in our logical structure, but have you ever tried to resolve the right `#include` path from another library that has dozens (if not hundreds) of headers? The header (especially if we are avoiding an overuse of preprocessing) should be a streamlined declaration of interface and other key symbols in our package. So, we'll define a single header that declares our two symbols / modules:

```cpp
/**
 * mylibrary.hpp
 */

#pragma once

#include <string>

namespace mylibrary {
    namespace food {
        class Food {
            private:
            std::string makesSound;

            protected:
            public:
                Food();
                void eatMe();
        };
    }

    namespace cookie {
        class Cookie : public mylibrary::food::Food {
            private:
            protected:
            public:
                Cookie();
                void eatMe();
        };
    }
}
```

Note that we distinguish between `food` the namespace (or module) and `Food` the class, defined within that module. It's entirely conceivable that a module could contain/export multiple classes! We just chose a very basic example here.

With our header file defined, it's easy to add a single include in both our tests and our individual source files. Let's flesh out basic implementations for our classes now:

```cpp
/**
 * food.cpp
*/

#include "mylibrary.hpp"

mylibrary::food::Food::Food() {
    this->makesSound = "food";
}

void mylibrary::food::Food::eatMe() {
    std::cout << this->makesSound << ", " << this->makesSound << std::endl;
}
```

```cpp
/**
 * cookie.cpp
 */

#include "mylibrary.hpp"

mylibrary::cookie::Cookie::Cookie() {
    this->makesSound = "crunch";
}

void mylibrary::cookie::Cookie::eatMe() {
    std::cout << this->makesSound << ", " << this->makesSound << ", " << this->makesSound << "... Can I have some more!?" << std::endl;
}
```

## Top-Level Build

Now it's time to return to our CMakeLists.txt file, with one short detour first. Eventually we're going to adjust this project to target a static library. But in the meantime, there's no sense writing source without making sure we can run it! So we'll adjust our placeholder `main.cpp`:

```cpp
/**
 * main.cpp
 */

#include "mylibrary.hpp"

int main(int nArgs, char** vArgs) {
    mylibrary::food::Food f;
    mylibrary::cookie::Cookie c;
    f.eatMe();
    c.eatMe();
    return 0;
}
```

This should be enough to make sure our header is appropriately structured, and that we are correctly linking our source. Now let's set up the CMakeLists.txt file to build this program--specifically, by updating the `add_executable()` directive:

```cmake
add_executable(${PROJECT_NAME}
    "main.cpp"
    "food.cpp"
    "cookie.cpp"
)
```

Note a few useful habits:

* First, while it's possible to aggregate source files using pattern matching or some other implicit behavior, I appreciate it when we explicitly declare what the contents of the build artifact are. There are fewer surprises and it's much easier for other people to understand clearly what transforms are taking place.

* You don't technically need quotation marks in CMake; you could just list the filenames. I like to avoid this because it helps explicitly identify string values (declarative languages like CMake struggle with this factor of readability!) as opposed to variables. Particularly with default syntax highlighting, it's easy to glance at a CMake file and know--*RIGHT AWAY*--exactly what references the filesystem or other string values dependent on external context:

![quotation marks in cmake help identify strings and filesystem paths](https://dev-to-uploads.s3.amazonaws.com/uploads/articles/qbbry5vc8bxoqjbkzf5p.png)

* Lastly, it will be *very* easy to simply adjust this block when we switch to a static library target. Factored this way, it's obvious (even if you don't know a lot of CMake) exactly what's going on: A build artifact is identified and consists of the following source files, which are easy to identify; add; and remove when they are on different lines. This is true whether the build target is an executable or a library.

You should be able to build and run our test program now!

```sh
$ cmake -S . -B build
$ cmake --build build
$ build/Debug/mylibrary.exe
food, food
crunch, crunch, crunch... Can I have some more!?
```

# Actually It's A Static Library

![Actually It's A Static Library](https://dev-to-uploads.s3.amazonaws.com/uploads/articles/q0krgdorrl29bef4xbyf.png)

Now that we've set our project contents up, it's surprisingly easy to refactor as a static library:

* We adjust the primary build target to use an `add_library()` directive

* We specify `STATIC` as the library type in that directive

* We remove the entry point `main.cpp` from the library contents

```cmake
# define primary build target
add_library(${PROJECT_NAME} STATIC
    "food.cpp"
    "cookie.cpp"
)
```

Let's do a quick git-clean and verify:

```sh
$ git clean -Xfd
$ cmake -S . -B build
$ cmake --build
$ ls build/Debug | grep lib
mylibrary.lib
```

Holy cow! That was easy!

## Other Artifacts et Le Fin

There's a lot more we could get into here:

* You might touch a README

* Toss a LICENSE in there

* Maybe support a doc-building pass

* Define installation for includes, libraries, and binaries

* Write a CI specification to support all the above

But what I'd like to do at this point to tie a bow on the whole thing, once you've made any customizations you feel necessary, is commit the whole thing as a [degit](https://github.com/Rich-Harris/degit) template. This will help you reuse your template across many happy projects in the future!

If you're interested in a more complex use case, there's a numerical mathematics library I've been porting/updating that uses this pattern to scale very nicely across a wide variety of modules and tests:

https://github.com/Tythos/cuben

You can also find this article on dev.to (if you're not reading it there already!):

https://dev.to/tythos/cmake-and-git-submodules-more-advanced-cases-2ka

## Source

The source for this project can be found under (of course) my own degit template:

https://github.com/Tythos/degit-template-cpp.git

This is intended to be used as a degit template, of course, but you should be able to clone and build directly at first if you just want to jump to the end and see something working (so long as you remember to initialize the submodules!). If you *do* degit the repository, again, make sure you set up the submodules, which degit has a habit of removing/ignoring. (If it's too onerous you can always `--depth 1`.)

```sh
$ degit https://github.com/Tythos/degit-template-cpp mylibrary
$ cd mylibrary
$ git init .
```

Lastly, looking at the above, it's only marginally related but I do love how GitHub lets you jump right to dependencies when it recognizes they cross-link to other GitHub projects:

![github submodule cross-referencing](https://dev-to-uploads.s3.amazonaws.com/uploads/articles/tm3tmsfrwpwlnlmssgdp.png)
