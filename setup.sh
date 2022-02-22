if [ -d "build" ]; then
	rm -rf build/*
else
	mkdir build
fi

cd build

type cmake > /dev/null 2>&1 || ( echo cmake is required! && exit -1 )

cmake .. && make
cd ..

echo Done! Run build/server to start slite.


