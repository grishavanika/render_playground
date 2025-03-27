set VCPKG_ROOT=C:\Users\grish\dev\vcpkg
call cmake -S . -B build ^
	-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
call cmake -S . -B build_clang ^
	-T ClangCL ^
	-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake

:: call cmake --build build --config Release
:: call cmake --build build_clang --config Release
