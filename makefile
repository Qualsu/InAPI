CXX ?= g++
SRC ?= src/main.cpp
BUILD_DIR = build
TARGET = $(BUILD_DIR)/build$(EXE)
TARGET_SSL = $(BUILD_DIR)/build_ssl$(EXE)

CXXFLAGS = -g -I header -I include -I include/InAPI
LDFLAGS = -L lib
LDLIBS =
SSL_CXXFLAGS = -D CPPHTTPLIB_OPENSSL_SUPPORT
SSL_LDLIBS = -lssl -lcrypto

ifeq ($(OS),Windows_NT)
EXE = .exe
CXXFLAGS += -D _WIN32_WINNT=0x0A00
LDLIBS += -lws2_32
MKDIR = cmd /C if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
REMOVE = cmd /C if exist "$(BUILD_DIR)\build.exe" del /Q "$(BUILD_DIR)\build.exe"
RUN = $(TARGET)
RUN_SSL = $(TARGET_SSL)
else
EXE =
MKDIR = mkdir -p $(BUILD_DIR)
REMOVE = rm -f $(BUILD_DIR)/build $(BUILD_DIR)/build_ssl
RUN = ./$(TARGET)
RUN_SSL = ./$(TARGET_SSL)
endif

COMPILE = $(CXX) $(SRC) $(CXXFLAGS) -o $(TARGET) $(LDFLAGS) $(LDLIBS)
COMPILE_SSL = $(CXX) $(SRC) $(CXXFLAGS) $(SSL_CXXFLAGS) -o $(TARGET_SSL) $(LDFLAGS) $(SSL_LDLIBS) $(LDLIBS)

default:
	$(MKDIR)
	$(REMOVE)
	${COMPILE}
	${RUN}

ssl:
	$(MKDIR)
	$(REMOVE)
	${COMPILE_SSL}
	${RUN_SSL}

compile:
	$(MKDIR)
	$(REMOVE)
	${COMPILE}

compile_ssl:
	$(MKDIR)
	$(REMOVE)
	${COMPILE_SSL}

run:
	${RUN}

clean:
	${REMOVE}
