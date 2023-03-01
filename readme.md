<div align="center">
    <h1>NPMCI</h1>
	<p>
		<b>Utils to quick install node_modules</b>
	</p>
	<br>
	<br>
	<br>
</div>

| Platform | Build status |
| --- | :----- |
| Windows |![Windows](https://github.com/stck/npmi/workflows/Windows/badge.svg)|
| MacOS |![Macos](https://github.com/stck/npmi/workflows/Macos/badge.svg)|
| Ubuntu |![Ubuntu](https://github.com/stck/npmi/workflows/Ubuntu/badge.svg)|
| Docker Alpine|![Docker Alpine](https://github.com/stck/npmi/workflows/Docker%20Alpine/badge.svg)|
| Docker Debian|![Docker Debian](https://github.com/stck/npmi/workflows/Docker%20Debian/badge.svg)|
| Docker Centos|![Docker Centos](https://github.com/stck/npmi/workflows/Docker%20Centos/badge.svg)|

## Dependencies
There are no dependencies for this project

## Features
* In-memory gzip extraction (no IOPS required)
* Built-in file filter
* Concurrent install

## Current limitations
* node-gyp won't work
* postinstall actions won't be called
* `package-lock.json` handling only

## Roadmap
* Keep-alive for connections
* yarn.lock
* `package-lock.json` (version 2)


## CLI flags
`--verbose` - Verbose output to stdout

`--dev` - Install dev dependencies

`--optional` - Install optional dependencies

By default, dev & optional dependencies are omitted.

## Ignore file
Binary uses .pkgignore to find file globs that will be omitted by inflate. 

```
# Wildcards
license*
# Extension
*.md
# JS/TS
*.?s
```

If no `.pkgignore` is present - default config is used

```
"licen?e*
authors*
readme*
funding*
changelog*
*.m?d
*.markdown
*.workflow
/.*
/__mocks__*
*.spec.js
*.html
*.txt
```

## How to build
|Requirement|TLSVersion|
|---|---|
|CMake|>=3.10|
|gcc|~8.10|

Use with Unix/MacOS:
```
cmake  -G "Unix Makefiles" -S "." -B "./build"
cmake --build ./build --target all
```

Use with Windows:
```
cmake -G "MinGW Makefiles" -S "." -B "./build"
cmake --build ./build --target all
```

## Tests
Tests are written using **CTest**, that comes with cmake.

To run tests:
```
ctest
```
