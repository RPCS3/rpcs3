

CFLAGS  := -W -Wall -Wextra -g `sdl-config --cflags` -DSDL_NO_COMPAT
LDFLAGS := `sdl-config --libs`

# If it doesn't pick up defaults
#CFLAGS  := -D_GNU_SOURCE=1 -D_REENTRANT -I/usr/local/include/SDL
#LDFLAGS := -lm -ldl -lesd -lpthread

SRC         	:= testsdl.c \
						platform/platform.c \
						rect/rect.c \
						surface/surface.c \
						render/render.c \
						audio/audio.c
COMMON_SRC     := SDL_at.c common/common.c
COMMON_INCLUDE := SDL_at.h

RWOPS_SRC      := rwops/rwops.c rwops/TestSupportRWops_Generic.c

TESTS_ALL := testsdl \
	platform/platform \
	rwops/rwops \
	rect/rect \
	surface/surface \
	render/render \
	audio/audio


.PHONY: all clean test


all: $(TESTS_ALL)

test: all
	@./testsdl

testsdl: $(SRC) $(RWOPS_SRC) $(COMMON_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(RWOPS_SRC) $(COMMON_SRC)

platform/platform: platform/platform.c $(COMMON_INCLUDE) $(COMMON_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ platform/platform.c $(COMMON_SRC) -DTEST_STANDALONE

rwops/rwops: $(RWOPS_SRC) $(COMMON_INCLUDE) $(COMMON_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(RWOPS_SRC) $(COMMON_SRC) -DTEST_STANDALONE

rect/rect: rect/rect.c $(COMMON_INCLUDE) $(COMMON_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ rect/rect.c $(COMMON_SRC) -DTEST_STANDALONE

surface/surface: surface/surface.c $(COMMON_INCLUDE) $(COMMON_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ surface/surface.c $(COMMON_SRC) -DTEST_STANDALONE

render/render: render/render.c $(COMMON_INCLUDE) $(COMMON_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ render/render.c $(COMMON_SRC) -DTEST_STANDALONE

audio/audio: audio/audio.c $(COMMON_INCLUDE) $(COMMON_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ audio/audio.c $(COMMON_SRC) -DTEST_STANDALONE

clean:
	$(RM) $(TESTS_ALL)
