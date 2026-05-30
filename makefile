COMPILE = g++ src/main.cpp -g -o build/build.exe -Iheader -Llib -Iinclude -Iinclude/InAPI -lws2_32
COMPILE_SSL = g++ src/main.cpp -g -o build/build_ssl.exe -Iheader -Llib -Iinclude -Iinclude/InAPI -DCPPHTTPLIB_OPENSSL_SUPPORT -lssl -lcrypto -lws2_32
RUN = build/build.exe
REMOVE = rm -f build/*.exe

default:
	${COMPILE}
	${RUN}

ssl:
	${COMPILE_SSL}
	${RUN}

compile:
	${COMPILE}

compile_ssl:
	${COMPILE}

run:
	${RUN}

clean:
	${REMOVE}
