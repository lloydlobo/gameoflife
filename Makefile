# Program / executable name.
PROGN := gameoflife

# Compiler C
CC := clang

# Flags for the build target.
#	-g Compiler flags for debugging.
CFLAGS := -g -Wall

# Include directories for SDL2.
SDL2_INCLUDE := -I/usr/local/include/SDL2
SDL2_TTF_INCLUDE := -I/usr/local/include/SDL2/SDL2_ttf

# Libraries for SDL2, SDL2_ttf, and the math library.
SDL2_LIBS := -L/usr/local/lib -lSDL2
SDL2_TTF_LIBS := -L/usr/local/lib -lSDL2_ttf
MATH_LIB := -lm

# Source files and headers.
SRCS := main.c
HEADERS :=

# Consolidate 3rd party dependencies.
INCLUDE_DIRS := $(SDL2_INCLUDE) $(SDL2_TTF_INCLUDE)
LIBRARIES := $(SDL2_LIBS) $(SDL2_TTF_LIBS) $(MATH_LIB)

# arg after make cmd(build:) -> (SRCS) are prerequisites,
# and make checks if it's modified.
#	$^ expands to this arg.
# 	$(CC) -o $@ $(CFLAGS) $(INCLUDE_DIRS) $^ $(LIBRARIES)

build: $(SRCS)
	$(CC) -o $(PROGN) $(CFLAGS) $(INCLUDE_DIRS) $(SRCS) $(LIBRARIES)

clean:
	rm -f $(PROGN) *.o

test: CFLAGS += -DVERBOSE_FLAG

test: build
	-(./$(PROGN) --mode terminal)

# TODO: SDL2
# cmake_minimum_required(VERSION 3.20)
#
# project(tictactoe)
#
# find_package(SDL2 REQUIRED)
# include_directories(${SDL2_INCLUDE_DIRS})
#
# find_package(PkgConfig)
# pkg_check_modules(SDL2_GFX SDL2_gfx SDL2_ttf)
# include_directories(${SDL2_GFX_INCLUDE_DIRS})
#
# SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -Wall -Werror -fdump-rtl-expand")
#
# set(SRCS
#   main.c
#   logic.c
#   render.c
#   font.c
# )
# set(SRCS_HEADERS
#   logic.h
#   render.h
#   game.h
#   font.h
# )
#
# add_executable(tictactoe ${SRCS} ${SRCS_HEADERS})
# target_link_libraries(tictactoe ${SDL2_LIBRARIES} ${SDL2_GFX_LIBRARIES})
#
