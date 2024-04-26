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
LIB=libProxyServer.a

all: ProxyServer

ProxyServer: $(BUILD_DIR) clean $(SOURCES) $(LIB)
	mkdir $(BUILD_DIR)/include
	cp include/outheader/*.h build/include/

.PHONY: example
example:
	$(CC) $(CFLAGS) example/main.cpp $(CINCLUDE) -I $(RAPIDJSON_INCLUDE) -o build/main.o
	@set static_link :=
	$(eval static_link += $(foreach lib,$(BOOST_STATIC_LIB),-l$(lib)))
	$(CC) build/main.o -L $(BOOST_LIB) -L $(BUILD_DIR) -lProxyServer $(static_link) $(LINK_LIBZ) -o build/example_main

$(BUILD_DIR):
	@if [ ! -d "$(BUILD_DIR)" ]; then mkdir -p $(BUILD_DIR); fi

$(LIB): $(OBJECTS)
	@set static_link_flag :=
	$(eval static_link_flag += $(foreach lib, $(OPENSSL_STATIC_LIB), $(OPENSSL_LIB)/lib$(lib).a ))
	$(LIBTOOL) $(LIBTOOL_FLAG) -o $(BUILD_DIR)/$@ $(OBJECTS) $(static_link_flag)
#$(eval static_link_flag += $(foreach lib, $(BOOST_STATIC_LIB), $(BOOST_LIB)/lib$(lib).a ))
$(BUILD_DIR)/%.o: src/%.cpp
	@if [ ! -d "$(dir $@)" ]; then mkdir -p $(dir $@); fi
	$(CC) $(CFLAGS) $< $(CINCLUDE) -o $@

.PHONY: clean
clean:
	rm -rf build/*