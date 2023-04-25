build_dirs += helloworld_m0
build_dirs += d0_spl

CHIP ?= bl808

all: $(build_dirs)

$(build_dirs):
	make -C $@

.PHONY: $(build_dirs) clean

clean::
	make -C helloworld_m0 clean
	make -C d0_spl clean

BL_SDK_BASE = ./..

export BL_SDK_BASE

include $(BL_SDK_BASE)/project.build

