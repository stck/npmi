set(SOURCES
        crypto/aes.spec.cpp
        crypto/rsa.spec.cpp
        crypto/sha256.spec.cpp
        format/package_lock.spec.cpp
        format/tar.spec.cpp
        util/regex.spec.cpp
        util/args.spec.cpp
        proto/tcp.spec.cpp
        proto/tls.spec.cpp
        )

foreach (_test ${SOURCES})
    string(FIND ${_test} / slashPos)
    string(FIND ${_test} .spec.cpp endPos)
    MATH(EXPR start ${slashPos}+1)
    MATH(EXPR length ${endPos}-${start})
    string(SUBSTRING ${_test} ${start} ${length} test)

    set(test_name ${test}_spec)
    add_definitions(-DUNITTEST)
    add_executable(${test_name} ${_test})
    add_test(${test_name} ${test_name})
endforeach ()
