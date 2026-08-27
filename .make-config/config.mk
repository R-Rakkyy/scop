# ---------------------------------------------------------------------------
# User-editable defaults. Anything here can still be overridden on the
# command line, e.g. `make CXX=clang++`.
# ---------------------------------------------------------------------------

# Compilers
CC  ?= cc
CXX ?= c++

# Extra flags appended to the computed CXXFLAGS/CFLAGS, for local overrides
# that shouldn't live in the Makefile itself.
EXTRA_CFLAGS   ?=
EXTRA_CXXFLAGS ?=

# Where vendored dependencies (GLFW/Vulkan/libftpp) get cloned+built when a
# system copy isn't found via pkg-config.
DEPSDIR ?= .deps
