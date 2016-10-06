# MemoryScanner

Cross-platform memory scanning tool that runs on Windows, Linux and macOS. 
Right now it uses a temporary command-line interface but will hopefully switch to a graphical user interface in the near future. (famous last words)

## How to Compile
Compile all non-system .cpp files and the specific system_\<platform>.cpp for your system.

### Windows
Build the Visual Studio solution file.

### Linux
```
clang++ main.cpp memory.cpp scanner.cpp system_linux.cpp -o memscan -O3 -fexceptions -std=c++14
```

### macOS
```
clang++ main.cpp memory.cpp scanner.cpp system_macos.cpp -o memscan -O3 -fexceptions -std=c++14
```

## Note
On Linux and macOS make sure to run as root to have access to other processes.
macOS will only allow you to scan non-system applications.
