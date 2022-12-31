# C_PNGDecoder
Simple implementation of PNG Decoder in C based on [this article](https://pyokagan.name/blog/2019-10-14-png/)

# Usage

Build the executable using `build.bat` (it require clang.exe)

## Command with clang.exe
```
clang -o bin\main.exe src\main.c -I include -I include\SDL -l zlibwapi -l linkedlist -l SDL2 -L library
```
N.B.<br>
It requires that the `bin` folder exists
Copy `zlibwapi.dll`, `linkedlist.dll` and `SDL2.dll` from `library` folder to `bin` folder

## Run it
Run the main.exe with relative path to your png
```
bin\main.exe demo.png
```
