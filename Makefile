CC     := gcc
RM     := rm -f

LDFLAGS := -pipe -flto

CFILES := $(wildcard src/*.c)

INCLUDES := -Isrc/ -Ibin/include/ -I/usr/include/ffmpeg/
FLAGS  := -Wall -Wextra $(INCLUDES) -pipe -O2 -flto -march=native -s -MMD -MP -ggdb
OBJDIR := bin
BINARY := bin/imgview
OBJS   := $(CFILES:%.c=$(OBJDIR)/%.o)
HEADER_DEPS := $(CFILES:%.c=$(OBJDIR)/%.d)

LIBRARIES := bin/libraylib.a -lm -lmupdf -lavcodec -lavutil -lavformat -lswresample -lswscale

OS := $(shell cat /etc/os-release | rg "Fedora Linux")
ifneq ($(OS),)
LIBRARIES := $(LIBRARIES) -lmupdf-third -ljpeg -lpng -ljbig2dec -lopenjp2 -lfreetype -lharfbuzz -lgumbo -lstdc++ -ltesseract -lleptonica -lz
endif

.PHONY: all install
all: $(OBJDIR) $(BINARY)

install: all
	cp $(BINARY) /home/oskar/.local/bin/
	cp imgview.desktop /home/oskar/.local/share/applications/

$(OBJDIR):
	mkdir -p $(OBJDIR)/src

$(BINARY): $(OBJS) | bin/libraylib.a
	$(CC) $(LDFLAGS) -o $(BINARY) $(OBJS) $(LIBRARIES)

bin/libraylib.a:
	git clone --depth=1 --branch 5.0 https://github.com/raysan5/raylib.git bin/raylib/
	cd bin/raylib && \
	git apply ../../raylib-options.patch && \
	mkdir build && \
	cd build && \
	cmake -DCUSTOMIZE_BUILD=ON ../ && \
	make -j20
	cp bin/raylib/build/raylib/libraylib.a bin/
	mkdir bin/include/
	cp bin/raylib/build/raylib/include/* bin/include/

-include $(HEADER_DEPS)
$(OBJDIR)/%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) $(BINARY)
	$(RM) $(OBJS)
	$(RM) $(HEADER_DEPS)

.PHONY:
clean-all:
	$(RM) -r $(OBJDIR)
