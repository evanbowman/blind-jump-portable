
For GAMEBOY builds:

```
cmake -DCMAKE_TOOLCHAIN_FILE=$(pwd)/devkitarm.cmake .
```

If you have all of the proper tooling installed (grit, python, etc.) and you want to auto-build the image files, you can enable the option during configuration:

```
cmake -DGBA_AUTOBUILD_IMG=ON -DCMAKE_TOOLCHAIN_FILE=$(pwd)/devkitarm.cmake .
```