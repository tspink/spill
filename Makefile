#
# spill
#

# Directories
q	:= @
top-dir := $(CURDIR)
src-dir := $(top-dir)/src
lib-dir := $(top-dir)/lib
inc-dir := $(top-dir)/inc
bin-dir := $(top-dir)/bin

# Tools
CC	:= gcc
CXX	:= g++

# Main Targets
lib-target		:= libspill.so
lib-target-cflags	:= -std=c++11 -I$(inc-dir) -g -Wall -fPIC -D__IN_LIB__
lib-target-ldflags	:=
src-target		:= spill
src-target-cflags	:= -std=c++11 -I$(inc-dir) -g -Wall
src-target-ldflags	:=
targets			:= $(lib-target) $(src-target)

# Objects
lib-objs	:= ctx.o model/assembly.o model/method.o model/image.o model/metadata.o
src-objs	:= main.o

# Qualified Paths
real-lib-target	:= $(bin-dir)/$(lib-target)
real-src-target := $(bin-dir)/$(src-target)
real-targets := $(patsubst %,$(bin-dir)/%,$(targets))
real-lib-objs := $(patsubst %,$(lib-dir)/%,$(lib-objs))
real-src-objs := $(patsubst %,$(src-dir)/%,$(src-objs))

# Auto-variables
cflags = $(if $(in-lib),$(lib-target-cflags),$(src-target-cflags))

all: $(real-targets)

clean: .FORCE
	rm -f $(real-targets)
	rm -f $(real-lib-objs)
	rm -f $(real-lib-objs:.o=.d)
	rm -f $(real-src-objs)
	rm -f $(real-src-objs:.o=.d)

$(real-lib-objs) : in-lib := y
$(real-src-objs) : in-lib :=

$(real-src-target): $(real-src-objs) $(real-lib-target)
	@echo "  LD    $(notdir $@)"
	$(q)g++ -o $@ $(src-target-ldflags) $(real-src-objs) -L$(bin-dir) -lspill

$(real-lib-target): $(real-lib-objs)
	@echo "  LD    $(notdir $@)"
	$(q)g++ -shared -o $@ $(lib-target-ldflags) $(real-lib-objs)

%.o: %.cpp
	@echo "  C++   $(notdir $@)"
	$(q)g++ -MD -c -o $@ $(cflags) $<
	
-include $(real-lib-objs:.o=.d)
-include $(real-src-objs:.o=.d)

.PHONY: .FORCE
