\page develCodingConvention Coding conventions for Radium

[TOC]

Mainly inspired by https://google-styleguide.googlecode.com/svn/trunk/cppguide.html


# Code style
Please follow the scripts/clang-format coding style (tested `with clang-format 9.0`).
We also provide a pre commit hook that checks the committed files are correctly formatted.
To install both hooks and clang-format, simply run `./scripts/install-scripts-linux.sh` on linux, or adapt to your OS.

* Indentation style : 4-spaces
* Brace style : keep it consistent across files.
* Case style : CamelCase
* Only classes have their first letter capitalized. Functions and variables don't.
* Class members have the `m_` prefix. Other prefixes (apart from `g_` for globals, and `s_` for static classe members) are discouraged.
* no ifs / for / while one-liners. Braces everywhere. Even for cases in a switch.
* separate different clauses in a boolean expression with parens.
`( (a || b) && ( c+d < 0) )`
* use c++ style casts unless converting a value (e.g. `int x = int(f))`. Avoid `const_cast` if possible.
* use prefix increments (`++i` and not `i++`)
* use `const` everywhere possible. use `constexpr` for const values.
* line length should be kept at 80 (soft limit) and not exceed 120 (hard limit)
* no need for () for a return statement.

# Headers #

* Every .cpp must have an associated .hpp file
* use .inl files for inline functions definitions and include it in the .hpp
* Two types of include guards are accepted (modern is preferred):
   * Legacy: `#ifndef HEADER_NAME_HPP_`
   * Modern: `#pragma once`
* Every class should have its own header.
* Always use < > in include directives and never " "
* Keep headers in order : Class header, system libraries, other libraries, other headers from project.
* forward declare as much as you can

# Functions
* use const reference or value to pass input parameters
* use references for passing output parameters
* output parameters appear last.

# Scope and names
* use anonymous namespaces for file local variables / functions. no classes with only static functions !
* Use `namespace Ra { }` for all radium engine code, plus a sub-namespace for each module (e.g. `Ra::Physics`)
* never use  the `using` instruction for a whole namespace
* put all non member functions in a namespace (no top level global functions)
* declare pointers and ref attached to their type : `void* a`, `const Foo& bar`
* macros and defined constants should be in capitals.

# Variables
* declarations should always be made on separate lines; (no `int a, b, c;`)
* initialize variables on the line of declaration whenever possible
* no global variables. If really necessary, prefix with `g_`
* use C++11 `nullptr` for null pointers.
* use `auto` only when it helps readability.

# Scalar types
* Radium defines a default type `Scalar`, set either as `float` or `double` depending on the cmake
option `RADIUM_WITH_DOUBLE_PRECISION`
* Always use `Scalar` type to represent floating point numbers, except when interfacing with external
libraries using a fixed floating point type (e.g. Qt).
* Radium offers operators to create `Scalar` from integer and floating point numbers: always
use `Scalar()` or `_ra` suffix when defining numbers from literals (e.g. `auto a = .5_ra;`,
`Scalar b = a * Scalar( 2 );` or `Scalar c = a / 2_ra;`).
* Equality between Scalar values needs to be computed using `Ra::almost_equals` (see CoreMacros.hpp).

# Class design
* Constructors should be trival. All complex work goes in an `init()` function
* Try to order class members by size (biggest to smallest)
* Any class containing a fixed-size `Ra::Core::Vector` or `Matrix`member must declare `RA_CORE_ALIGNED_NEW`
* Try to declare these fixed-size vectors all together, and preferably first in the class.
* Never pass a fixed-size vector by value to a function (only ref and const ref). Using them as return value is fine.
* use `explicit` for all one-arguments constructors
* use `= delete` for preventing copy constructor and other compiler-generated functions
* use `= default` for default compiler-generated function (copy constructor,  assignment operator)
* use `struct` for trivial PODs where all data is public and `class` for
everything else.
* no other inheritance than public
* if multiple inheritance, all parent classes but one have to be interfaces (= pure virtual classes)
* composition is better than inheritance
* if the class has virtual methods then the destructor is virtual
* use `override` when overriding a virtual method.
* use operator overloading only if the meaning is non-ambiguous (only for arithmetic-like syntax).
* be consistent with operators (i.e. if you overload + and = also overload +=).
* declare : public functions (constructors first), private functions, public variables, private variables.
* declare nested types and enums before functions
* do not abuse function overloading.
* avoid `mutable`
* getters and setter should have a consistent name with the variable.

# Non-negociable
* Exceptions are forbidden, except when mimicking or using `std` containers (e.g., by throwing `std::out_of_range`).
In any cases, exceptions must be used in exceptional cases, and should not be used as an event system.
* No gotos.
