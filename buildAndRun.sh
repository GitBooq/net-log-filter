rm -rf build
mkdir build
cd build

cmake ..

make

echo ""
echo "Running CTest"
ctest

echo ""
echo "Running exe"
./main

cd ..
echo ""
echo "Done!"
