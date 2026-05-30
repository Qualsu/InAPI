COMPILE = g++ src/main.cpp -g -o build/build.exe -Iheader -Llib -Iinclude -Iinclude/InAPI -lws2_32
RUN = build/build.exe
REMOVE = rm -f build/*.exe

default:
	${COMPILE}
	${RUN}

compile:
	${COMPILE}

run:
	${RUN}

clean:
	${REMOVE}
