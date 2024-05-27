CC=g++ -std=c++11
CFLAGS=-O3 -c -Wall
LIBTOOL=libtool
LIBTOOL_FLAG=-static

CINCLUDE=-I include -I /usr/local/include

BOOST_STATIC_LIB := boost_system boost_thread-mt
OPENSSL_STATIC_LIB := ssl crypto
LINK_LIBZ=-lz

BUILD_DIR=build
# ProxyServer sources
SOURCES=${shell find src -name "*.cpp"}
OBJ=$(subst src, build, $(SOURCES))
OBJECTS=$(OBJ:.cpp=.o)

#LIB_OBJ=libProxyServer_.a
LIB_NAME=ProxyServer

all: libProxyServer

libProxyServer: $(BUILD_DIR) clean $(SOURCES) $(LIB_NAME)
	mkdir -p $(BUILD_DIR)/include
	mkdir -p $(BUILD_DIR)/include/mock
	mkdir -p $(BUILD_DIR)/include/service
	cp include/gzip.h $(BUILD_DIR)/include/
	cp include/ProxyServer.h $(BUILD_DIR)/include/
	cp include/mock/mock.h $(BUILD_DIR)/include/mock/
	cp include/service/httpdef.h $(BUILD_DIR)/include/service
	cp -r $(BOOST_INCLUDE)/boost $(BUILD_DIR)/include/

.PHONY: logfile
logfile:
	@echo $(SOURCES)
	@echo $(OBJECTS)

.PHONY: example
example: $(BUILD_DIR)/lib/lib$(LIB_NAME).dylib
	mkdir -p $(BUILD_DIR)/example
	$(CC) $(CFLAGS) example/main.cpp $(CINCLUDE) -I $(RAPIDJSON_INCLUDE) -o $(BUILD_DIR)/example/main.o
	$(CC) $(BUILD_DIR)/example/main.o -L $(BUILD_DIR)/lib -lProxyServer $(LINK_LIBZ) -o build/example/example_main

$(BUILD_DIR)/lib$(LIB_NAME).dylib: libProxyServer
$(BUILD_DIR)/lib$(LIB_NAME).a: libProxyServer

$(BUILD_DIR):
	@if [ ! -d "$(BUILD_DIR)" ]; then mkdir -p $(BUILD_DIR); fi

$(LIB_NAME): $(OBJECTS)
	mkdir $(BUILD_DIR)/lib
	@set static_link_flag :=
	$(eval static_link_flag += $(foreach lib, $(OPENSSL_STATIC_LIB), $(OPENSSL_LIB)/lib$(lib).a ))
	$(eval static_link_flag += $(foreach lib, $(BOOST_STATIC_LIB), $(BOOST_LIB)/lib$(lib).a ))
	$(LIBTOOL) $(LIBTOOL_FLAG) -o $(BUILD_DIR)/lib/lib$@.a $(OBJECTS) $(static_link_flag)
	$(CC) -shared $(OBJECTS) $(static_link_flag) -o lib$@.dylib
	mv lib$@.dylib $(BUILD_DIR)/lib/

$(BUILD_DIR)/%.o: src/%.cpp
	@if [ ! -d "$(dir $@)" ]; then mkdir -p $(dir $@); fi
	$(CC) $(CFLAGS) $< $(CINCLUDE) -o $@

.PHONY: clean
clean:
	rm -rf build/*