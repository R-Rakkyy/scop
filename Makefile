# ============================================================================
#                               scop
#
#  Quick start:
#    make                     build the binary
#    make strict debug        flags combine freely, in any order, on one line
#
#  See `make help` for the full flag/target reference.
# ============================================================================

-include .make-config/config.mk
-include .make-config/colors.mk
export COLOR_RESET COLOR_BOLD COLOR_SUCCESS COLOR_INFO COLOR_WARN \
       COLOR_ERROR COLOR_CYAN COLOR_MAG COLOR_BLUE OK_MARK WARN_MARK ERR_MARK

NAME       		:= scop

.DEFAULT_GOAL 	:= all

SRCDIR     		:= srcs
INCDIR     		:= includes
OBJDIR     		:= .obj
MAKECFGDIR 		:= .make-config
DEPSDIR    		?= .deps

CC         		?= cc
CXX        		?= c++
PKG_CONFIG 		?= pkg-config

# ----------------------------------------------------------------------------
#                               Sources
# ----------------------------------------------------------------------------

SRCS_CXX := $(addprefix $(SRCDIR)/, \
    main.cpp \
    engine/Engine.cpp \
    app/Application.cpp \
    engine/input/InputManager.cpp \
    engine/scene/Scene.cpp \
    engine/scene/mesh_transform.cpp \
    engine/renderer/Renderer.cpp \
    engine/renderer/renderer_state.cpp \
    engine/renderer/renderer_upload.cpp \
    engine/renderer/renderer_draw.cpp \
    engine/renderer/renderer_debug_info.cpp \
    utils/file_management.cpp \
    utils/obj/obj_loader.cpp \
    utils/obj/obj_parse_utils.cpp \
    utils/obj/obj_face_parser.cpp \
    utils/obj/obj_geometry_parser.cpp \
    utils/obj/obj_directive_parser.cpp \
    utils/obj/obj_primitive_parser.cpp \
    utils/obj/obj_normal_smoothing.cpp \
    utils/obj/obj_finalize.cpp \
    utils/obj/mtl_loader.cpp \
    utils/math/matrix_utils.cpp \
    engine/ui/ui_font.cpp \
    engine/ui/ui_batch.cpp \
    engine/ui/ui_overlay.cpp \
)

SRCS_C := $(addprefix $(SRCDIR)/, \
    glfw/window.c \
    engine/vk/context.c \
    engine/vk/render.c \
    engine/vk/device/instance.c \
    engine/vk/device/debug_messenger.c \
    engine/vk/device/validation.c \
    engine/vk/device/physical_device.c \
    engine/vk/device/logical_device.c \
    engine/vk/device/utils_device.c \
    engine/vk/device/queues.c \
    engine/vk/swapchain/images.c \
    engine/vk/swapchain/frames.c \
    engine/vk/swapchain/swapchain.c \
    engine/vk/swapchain/swapchain_recreate.c \
    engine/vk/swapchain/command.c \
    engine/vk/swapchain/depth.c \
    engine/vk/swapchain/msaa.c \
    engine/vk/pipeline/shader.c \
    engine/vk/pipeline/pipeline.c \
    engine/vk/pipeline/pipeline_common.c \
    engine/vk/pipeline/grid_pipeline.c \
    engine/vk/pipeline/ui_pipeline.c \
    engine/vk/resources/buffer.c \
    engine/vk/resources/mesh.c \
    engine/vk/resources/texture.c \
    engine/vk/resources/texture_mipmap.c \
    engine/vk/resources/descriptors.c \
    utils/stb_image_impl.c \
    utils/stats/system_stats.c \
)

OBJS := $(patsubst %.cpp,$(OBJDIR)/%.o,$(SRCS_CXX)) \
        $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS_C))
DEPS := $(OBJS:.o=.d)

SHADERS_GLSLDIR := shaders/glsl
SHADERS_SPVDIR  := shaders/spv

SHADERS_GLSL := $(addprefix $(SHADERS_GLSLDIR)/, \
    vert.glsl \
    frag.glsl \
    grid/vert.glsl \
    grid/frag.glsl \
    ui/vert.glsl \
    ui/frag.glsl \
    palette/frag.glsl \
)
SHADERS_SPV  := $(patsubst $(SHADERS_GLSLDIR)/%.glsl,$(SHADERS_SPVDIR)/%.spv,$(SHADERS_GLSL))

GLSLANG ?= glslangValidator

# ----------------------------------------------------------------------------
#                       External libs: GLFW / Vulkan
#
# pkg-config first; if a lib isn't found on the system, it's cloned and
# built locally under $(DEPSDIR) instead. 
# Vulkan additionally needs a real GPU driver (ICD) present.
# ----------------------------------------------------------------------------


GLFW_DIR     		:= $(DEPSDIR)/glfw
GLFW_LIBFILE 		:= $(GLFW_DIR)/build/src/libglfw3.a
GLFW_PC_OK   		:= $(shell $(PKG_CONFIG) --exists glfw3 2>/dev/null && echo 1)

ifeq ($(GLFW_PC_OK),1)
    GLFW_CFLAGS 	:= $(shell $(PKG_CONFIG) --cflags glfw3)
    GLFW_LIBS   	:= $(shell $(PKG_CONFIG) --libs glfw3)
    GLFW_DEP    	:=
else
    GLFW_CFLAGS 	:= -I$(GLFW_DIR)/include
    GLFW_LIBS   	:= -L$(GLFW_DIR)/build/src -lglfw3
    GLFW_DEP    	:= $(GLFW_LIBFILE)
endif

$(GLFW_LIBFILE):
	@$(MAKECFGDIR)/vendor_glfw.sh $(GLFW_DIR)


VULKAN_HDR_DIR     	:= $(DEPSDIR)/vulkan-headers
VULKAN_LOADER_DIR  	:= $(DEPSDIR)/vulkan-loader
VULKAN_LIBFILE     	:= $(VULKAN_LOADER_DIR)/build/loader/libvulkan.so
VULKAN_PC_OK       	:= $(shell $(PKG_CONFIG) --exists vulkan 2>/dev/null && echo 1)

ifeq ($(VULKAN_PC_OK),1)
    VULKAN_CFLAGS 	:= $(shell $(PKG_CONFIG) --cflags vulkan)
    VULKAN_LIBS   	:= $(shell $(PKG_CONFIG) --libs vulkan)
    VULKAN_DEP    	:=
else
    VULKAN_CFLAGS 	:= -I$(VULKAN_HDR_DIR)/include
    VULKAN_LIBS   	:= -L$(VULKAN_LOADER_DIR)/build/loader -lvulkan -Wl,-rpath,$(abspath $(VULKAN_LOADER_DIR)/build/loader)
    VULKAN_DEP    	:= $(VULKAN_LIBFILE)
endif

$(VULKAN_LIBFILE):
	@$(MAKECFGDIR)/vendor_vulkan.sh $(VULKAN_HDR_DIR) $(VULKAN_LOADER_DIR)

# ----------------------------------------------------------------------------
#                       Internal lib: libftpp
#
# Always fresh in $(DEPSDIR).
# If not found, the script downloads and builds it.
# ----------------------------------------------------------------------------

LIBFTPP_DIR    		:= $(DEPSDIR)/libftpp
LIBFTPP_LIB    		:= $(LIBFTPP_DIR)/libftpp.a
LIBFTPP_CFLAGS 		:= -I$(LIBFTPP_DIR)/includes
LIBFTPP_LIBS   		:= -L$(LIBFTPP_DIR) -lftpp

$(LIBFTPP_LIB):
	@$(MAKECFGDIR)/vendor_libftpp.sh $(LIBFTPP_DIR)

INCLUDES 			:= -I$(INCDIR) -isystem $(INCDIR)/vendor $(GLFW_CFLAGS) $(VULKAN_CFLAGS) $(LIBFTPP_CFLAGS)

# ----------------------------------------------------------------------------
#                               Flags
#
# Modifier targets (debug, strict, g3, lite, ...) do NOT recurse into a
# sub-make. They just flip variables below, straight from MAKECMDGOALS.
# ----------------------------------------------------------------------------

DEBUG       ?= 0
STRICT      ?= 0
USE_G3      ?= 0
LITE        ?= 0
NO_PROGRESS ?= 0
SANITIZE    ?=
MAKE_JOBS   ?= $(shell nproc)

ifneq ($(filter debug full-debug,$(MAKECMDGOALS)),)
    DEBUG := 1
endif
ifneq ($(filter strict,$(MAKECMDGOALS)),)
    STRICT := 1
endif
ifneq ($(filter g3 full-debug,$(MAKECMDGOALS)),)
    USE_G3 := 1
endif
ifneq ($(filter lite,$(MAKECMDGOALS)),)
    LITE := 1
endif

# ASan (memory/leak) and TSan (thread races) instrument the binary in
# incompatible ways and cannot be linked together.
ifneq ($(filter asan-mem,$(MAKECMDGOALS)),)
    DEBUG    := 1
    SANITIZE := address
endif
ifneq ($(filter asan-thread,$(MAKECMDGOALS)),)
    DEBUG    := 1
    SANITIZE := thread
endif
ifeq ($(filter asan-mem,$(MAKECMDGOALS))$(filter asan-thread,$(MAKECMDGOALS)),asan-memasan-thread)
    $(error asan-mem and asan-thread are mutually exclusive: AddressSanitizer and ThreadSanitizer cannot be linked into the same binary)
endif

# lite (fastest+lightest release build) strips away debug info and
# instrumentation, so it cannot coexist with flags that add exactly that.
LITE_CONFLICTS := debug full-debug g3 asan-mem asan-thread
ifneq ($(filter lite,$(MAKECMDGOALS)),)
    ifneq ($(filter $(LITE_CONFLICTS),$(MAKECMDGOALS)),)
        $(error lite is incompatible with $(filter $(LITE_CONFLICTS),$(MAKECMDGOALS)): lite strips debug info/instrumentation that these add)
    endif
endif

WARN_FLAGS := -Wall -Wextra -Werror

# C++: -std=c++20.
CXXFLAGS ?= $(WARN_FLAGS) -std=c++20 -MMD -MP $(EXTRA_CXXFLAGS)
# C: -std=c17. (Vulkan backend relies on C99)
CFLAGS   ?= $(WARN_FLAGS) -std=c17 -MMD -MP $(EXTRA_CFLAGS)

ifeq ($(DEBUG),1)
    CXXFLAGS += -g -O0
    CFLAGS   += -g -O0
endif

ifeq ($(SANITIZE),address)
    CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
    CFLAGS   += -fsanitize=address -fno-omit-frame-pointer
else ifeq ($(SANITIZE),thread)
    CXXFLAGS += -fsanitize=thread -fno-omit-frame-pointer
    CFLAGS   += -fsanitize=thread -fno-omit-frame-pointer
endif

ifeq ($(USE_G3),1)
    CXXFLAGS += -g3
    CFLAGS   += -g3
endif

# Portable GCC warnings, valid for both C and C++.
STRICT_COMMON := -pedantic -Wconversion -Wsign-conversion -Wshadow=local \
                  -Wundef -Wcast-align -Wwrite-strings \
                  -Wredundant-decls -Wmissing-declarations -Wlogical-op \
                  -Wuninitialized -fvisibility=hidden -Wdouble-promotion \
                  -Wduplicated-cond -Wrestrict -Wnull-dereference \
                  -Wstack-usage=4096 -Walloc-zero

ifeq ($(STRICT),1)
    CXXFLAGS += $(STRICT_COMMON) -Wold-style-cast
    CFLAGS   += $(STRICT_COMMON)
endif

# LITE builds the fastest binary first, lightest second.
ifeq ($(LITE),1)
    CXXFLAGS := $(filter-out -O% -g% -fsanitize=%,$(CXXFLAGS))
    CXXFLAGS += -O3 -march=native -flto -fomit-frame-pointer \
                -ffunction-sections -fdata-sections -pipe -s
    CFLAGS   := $(filter-out -O% -g% -fsanitize=%,$(CFLAGS))
    CFLAGS   += -O3 -march=native -flto -fomit-frame-pointer \
                -ffunction-sections -fdata-sections -pipe -s
    NO_PROGRESS := 1
endif

# std::thread/std::atomic in Engine need -pthread; -ldl is needed by the (which maybe will be replaced one day)
# GLFW/Vulkan loader for their runtime dlopen() of the platform backend/ICD.
LDFLAGS := $(GLFW_LIBS) $(VULKAN_LIBS) $(LIBFTPP_LIBS) -ldl -pthread
ifeq ($(LITE),1)
    LDFLAGS += -flto -Wl,--gc-sections -Wl,-O1
endif

# ----------------------------------------------------------------------------
#                           Flags fingerprint
#
# .make-config/$(NAME).flags tracks the flags an object was last built with.
# It is synced *now*, at parse time (not as a recipe), so that by the time
# make decides which objects are stale, the fingerprint on disk already
# reflects this invocation & the pending-object count below is accurate for
# the progress bar.
# ----------------------------------------------------------------------------

FLAGS_FILE        := $(MAKECFGDIR)/$(NAME).flags
FLAGS_FINGERPRINT := CC=$(CC) CXX=$(CXX) CFLAGS=$(CFLAGS) CXXFLAGS=$(CXXFLAGS) LDFLAGS=$(LDFLAGS)

$(shell $(MAKECFGDIR)/sync-flags.sh $(FLAGS_FILE) "$(FLAGS_FINGERPRINT)")

# ----------------------------------------------------------------------------
#                       Progress bar bookkeeping
# ----------------------------------------------------------------------------

PAD     := $(shell for f in $(SRCS_CXX) $(SRCS_C); do basename $$f; done | awk '{ if (length>m) m=length } END { print m+2 }')
COUNTER := $(MAKECFGDIR)/.progress.count

$(shell mkdir -p $(MAKECFGDIR); printf '0' >$(COUNTER))

NEED_OBJS := $(shell $(MAKECFGDIR)/count_pending.sh $(FLAGS_FILE) $(OBJS))

# ----------------------------------------------------------------------------
#                           Build rules
# ----------------------------------------------------------------------------

all: $(NAME)

-include $(DEPS)

$(OBJDIR)/%.o: %.cpp $(FLAGS_FILE) $(GLFW_DEP) $(VULKAN_DEP) $(LIBFTPP_LIB)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
	@if [ "$(NO_PROGRESS)" = "1" ]; then \
		printf "%b" "$(COLOR_INFO)[ $(COLOR_SUCCESS)$(notdir $<) $(COLOR_INFO)]: $(OK_MARK)\n"; \
	else \
		$(MAKECFGDIR)/progress.sh "$<" "$(PAD)" "$(COUNTER)" "$(NEED_OBJS)"; \
	fi

$(OBJDIR)/%.o: %.c $(FLAGS_FILE) $(GLFW_DEP) $(VULKAN_DEP) $(LIBFTPP_LIB)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
	@if [ "$(NO_PROGRESS)" = "1" ]; then \
		printf "%b" "$(COLOR_INFO)[ $(COLOR_SUCCESS)$(notdir $<) $(COLOR_INFO)]: $(OK_MARK)\n"; \
	else \
		$(MAKECFGDIR)/progress.sh "$<" "$(PAD)" "$(COUNTER)" "$(NEED_OBJS)"; \
	fi

$(SHADERS_SPVDIR)/%.spv: $(SHADERS_GLSLDIR)/%.glsl
	@mkdir -p $(dir $@)
	@command -v $(GLSLANG) >/dev/null 2>&1 || { printf "%b\n" "$(COLOR_ERROR)$(GLSLANG) not found; install glslang to compile shaders$(COLOR_RESET)" >&2; exit 1; }
	@stage=$$(case "$(notdir $<)" in vert.*) echo vert ;; frag.*) echo frag ;; comp.*) echo comp ;; geom.*) echo geom ;; tesc.*) echo tesc ;; tese.*) echo tese ;; *) printf "%b\n" "$(COLOR_ERROR)cannot infer shader stage from $<$(COLOR_RESET)" >&2; exit 1 ;; esac); \
	$(GLSLANG) -V -S $$stage -o $@ $<
	@printf "%b" "$(COLOR_INFO)[ $(COLOR_SUCCESS)$(notdir $<) $(COLOR_INFO)]: $(OK_MARK)\n"

$(NAME): $(FLAGS_FILE) $(OBJS) $(SHADERS_SPV) $(GLFW_DEP) $(VULKAN_DEP) $(LIBFTPP_LIB)
	@printf "\n$(SEP)\n"
	@printf "$(COLOR_INFO) ... Building $(COLOR_BOLD)$(NAME)$(COLOR_RESET) ...$(COLOR_RESET)\n\n"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(OBJS) $(LDFLAGS) -o $(NAME)
	@printf "\n$(COLOR_INFO)[ $(COLOR_SUCCESS)$(NAME) $(COLOR_INFO)]: $(OK_MARK)\n"
	@printf "\033[0m\n"

shaders: $(SHADERS_SPV)

# ----------------------------------------------------------------------------
# Modifier targets: flip a variable above, then fall through to `all`.
# Combine freely on one command line: `make strict debug`.
# ----------------------------------------------------------------------------

debug strict g3 full-debug: all
	@:

# ASan and TSan are separate targets (not composable with each other); each
# builds then immediately runs the binary.
asan-mem: all
	@printf "\n$(COLOR_INFO)Running $(COLOR_BOLD)$(NAME)$(COLOR_RESET) $(COLOR_INFO)under ASan/LSan with suppressions from asan_suppressions.txt$(COLOR_RESET)\n"
	@ASAN_OPTIONS="detect_leaks=1:verbosity=1:symbolize=1" \
	LSAN_OPTIONS="suppressions=asan_suppressions.txt:verbosity=1" \
	./$(NAME)

asan-thread: all
	@printf "\n$(COLOR_INFO)Running $(COLOR_BOLD)$(NAME)$(COLOR_RESET) $(COLOR_INFO)under ThreadSanitizer with suppressions from tsan_suppressions.txt$(COLOR_RESET)\n"
	@TSAN_OPTIONS="suppressions=tsan_suppressions.txt" \
	./$(NAME)

# lite is a standalone, silent, parallel release build; it recurses on
# purpose to pick up -j$(MAKE_JOBS), which a modifier flag can't do mid-run.
lite:
	@$(MAKE) -j$(MAKE_JOBS) LITE=1 all

clean:
	@echo "$(COLOR_INFO) Cleaning object files...$(COLOR_RESET)"
	@rm -rf $(OBJDIR)

fclean: clean
	@echo "$(COLOR_INFO) Removing binary $(NAME)...$(COLOR_RESET)"
	@rm -f $(NAME)
	@rm -f $(FLAGS_FILE) $(COUNTER)
	@rm -f $(SHADERS_SPV)

re:
	@$(MAKE) fclean
	@$(MAKE) all

help:
	@printf '%b\n' "$(COLOR_INFO)Usage: $(COLOR_SUCCESS)make$(COLOR_INFO) [$(COLOR_MAG)target$(COLOR_INFO)...] [$(COLOR_CYAN)VARIABLE=value$(COLOR_INFO)...]$(COLOR_RESET)"
	@printf '%b\n' ""
	@printf '%b\n' "  $(COLOR_INFO)Targets:"
	@printf '%b\n' "    $(COLOR_MAG)all$(COLOR_INFO)          build the binary (default)"
	@printf '%b\n' "    $(COLOR_MAG)debug$(COLOR_INFO)        -g -O0, no sanitizer"
	@printf '%b\n' "    $(COLOR_MAG)asan-mem$(COLOR_INFO)     build + run under AddressSanitizer/LeakSanitizer"
	@printf '%b\n' "    $(COLOR_MAG)asan-thread$(COLOR_INFO)  build + run under ThreadSanitizer"
	@printf '%b\n' "                 $(COLOR_WARN)asan-mem and asan-thread are mutually exclusive$(COLOR_INFO)"
	@printf '%b\n' "    $(COLOR_MAG)strict$(COLOR_INFO)       extra pedantic warnings (-Wconversion, -Wshadow, ...)"
	@printf '%b\n' "    $(COLOR_MAG)g3$(COLOR_INFO)           heavier debug info (-g3)"
	@printf '%b\n' "    $(COLOR_MAG)full-debug$(COLOR_INFO)   debug + g3"
	@printf '%b\n' "    $(COLOR_MAG)lite$(COLOR_INFO)         fastest+lightest release build (-O3, LTO, stripped)"
	@printf '%b\n' "                 $(COLOR_WARN)lite is incompatible with debug/full-debug/g3/asan-mem/asan-thread$(COLOR_INFO)"
	@printf '%b\n' "    $(COLOR_MAG)clean$(COLOR_INFO), $(COLOR_MAG)fclean$(COLOR_INFO), $(COLOR_MAG)re$(COLOR_INFO)   cleanup / rebuild"
	@printf '%b\n' ""
	@printf '%b\n' "  $(COLOR_INFO)Targets combine freely on one line and apply together:"
	@printf '%b\n' "    $(COLOR_SUCCESS)make $(COLOR_MAG)strict debug$(COLOR_INFO)"
	@printf '%b\n' "    $(COLOR_SUCCESS)make $(COLOR_MAG)strict asan-mem$(COLOR_INFO)"
	@printf '%b\n' ""
	@printf '%b\n' "  $(COLOR_INFO)Variables: $(COLOR_CYAN)DEBUG=1$(COLOR_INFO), $(COLOR_CYAN)STRICT=1$(COLOR_INFO), $(COLOR_CYAN)USE_G3=1$(COLOR_INFO), $(COLOR_CYAN)SANITIZE=address|thread$(COLOR_INFO), $(COLOR_CYAN)NO_PROGRESS=1$(COLOR_INFO), $(COLOR_CYAN)CC$(COLOR_INFO), $(COLOR_CYAN)CXX$(COLOR_INFO), $(COLOR_CYAN)DEPSDIR$(COLOR_INFO)"
	@printf '%b\n' "  $(COLOR_INFO)Config files: $(COLOR_BLUE)$(MAKECFGDIR)/config.mk$(COLOR_INFO), $(COLOR_BLUE)$(MAKECFGDIR)/colors.mk$(COLOR_INFO), $(COLOR_BLUE)$(MAKECFGDIR)/*.sh$(COLOR_RESET)"
	@printf '%b\n' "  $(COLOR_INFO)Flags fingerprint (auto): $(COLOR_BLUE)$(FLAGS_FILE)$(COLOR_RESET)"
	@printf '%b\n' "  $(COLOR_INFO)External libs (GLFW/Vulkan) come from pkg-config, or are cloned+built under $(COLOR_BLUE)$(DEPSDIR)$(COLOR_RESET)$(COLOR_INFO) if missing."
	@printf '%b\n' "  $(COLOR_INFO)libftpp is always cloned+built under $(COLOR_BLUE)$(LIBFTPP_DIR)$(COLOR_RESET)$(COLOR_INFO)."

.PHONY: all debug strict g3 full-debug asan-mem asan-thread lite clean fclean re help shaders
