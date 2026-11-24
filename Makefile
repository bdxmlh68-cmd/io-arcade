CXX := arm-none-eabi-g++
LD := arm-none-eabi-gcc
STRIP := arm-none-eabi-strip
OBJDUMP := arm-none-eabi-objdump

EADK_INC := $(shell find /opt /usr /home -name "eadk.h" 2>/dev/null | head -n 1 | xargs -r dirname)

CXXFLAGS := -std=c++11 -fno-exceptions -fno-rtti -Os -Wall $(if $(EADK_INC),-I$(EADK_INC),)
LDFLAGS  := -Wl,--relocatable -nostartfiles --specs=nano.specs -T eadk.ld -Wl,-e,main -Wl,-u,eadk_app_name -Wl,-u,eadk_app_icon -Wl,-u,eadk_api_level

all: target/io-arcade.nwa

target/main.o: src/main.cpp
	mkdir -p target
	$(CXX) $(CXXFLAGS) -c $< -o $@

target/io-arcade.nwa: target/main.o eadk.ld
	$(LD) $(LDFLAGS) target/main.o -o $@
	$(STRIP) --strip-debug $@
	$(OBJDUMP) -h $@ | grep eadk || true

clean:
	rm -rf target
