import subprocess

# generate optimized mlir files
def opt_file(file_path):
    file_path_no_ext = file_path.removesuffix(".mlir")

    # Create .eqsat.mlir file
    subprocess.run(["./build/bin/tangram-opt", "--allow-unregistered-dialect", "--mlir-disable-threading", "--eq-sat", file_path, "-o", f"{file_path_no_ext}.eqsat.mlir"])
    # subprocess.run(["./deps/egglog/target/debug/egglog", "--allow-unregistered-dialect", "--mlir-disable-threading", "--eq-sat", file_path, "-o", f"{file_path_no_ext}.eqsat.mlir"])

def main():
    # opt_file("bench/bitwise/bitwise.mlir")
    opt_file("bench/hash/hash.mlir")
    # opt_file("bench/hash2/hash.mlir")
    # opt_file("bench/table/table.mlir")
    # opt_file("bench/condition/condition.mlir")
    # opt_file("bench/branch/branch.mlir")

if __name__ == "__main__":
    main()