cd third_party/llvm-project
cmake -S llvm -B build-release -G Ninja -DLLVM_ENABLE_PROJECTS="mlir" -DCMAKE_BUILD_TYPE=Release -DLLVM_USE_LINKER=lld -DCMAKE_INSTALL_PREFIX=/local/yihan/tangram-compiler/install
cmake --build build-release --target install
