GENNUM_DIR="../gennum_project"
VERNUM="./vernum"
FILES=("inty.txt" "floaty.txt" "binary.txt")
LIBS=("int_lib" "float_lib" "bin_lib")
echo "=== ZAČIATOK TESTOVANIA ==="
for lib in "${LIBS[@]}"; do
    echo ""
    echo ">>> Testujem knižnicu: $lib"
    export LD_LIBRARY_PATH="./$lib"
    for file in "${FILES[@]}"; do
        echo "--- Súbor: $file ---"
        $VERNUM < "$GENNUM_DIR/$file"
        echo ""
    done
done
echo "=== TESTOVANIE DOKONČENÉ ==="