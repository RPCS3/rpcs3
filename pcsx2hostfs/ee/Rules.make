
# Include directories
EE_INCS := -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include -I. $(EE_INCS)

# C compiler flags
EE_CFLAGS := -D_EE -O2 -G0 -Wall $(EE_CFLAGS)

# C++ compiler flags
EE_CXXFLAGS := -D_EE -O2 -G0 -Wall $(EE_CXXFLAGS)

# Linker flags
EE_LDFLAGS := -L$(PS2SDK)/ee/lib $(EE_LDFLAGS)

# Assembler flags
EE_ASFLAGS := -G0 $(EE_ASFLAGS)

# Link with following libraries.  This is a special case, and instead of
# allowing the user to override the library order, we always make sure
# libkernel is the last library to be linked.
EE_LIBS += -lc -lkernel

# Externally defined variables: EE_BIN, EE_OBJS, EE_LIB

# These macros can be used to simplify certain build rules.
EE_C_COMPILE = $(EE_CC) $(EE_CFLAGS) $(EE_INCS)
EE_CXX_COMPILE = $(EE_CC) $(EE_CXXFLAGS) $(EE_INCS)

%.o : %.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

%.o : %.cpp
	$(EE_CXX) $(EE_CXXFLAGS) $(EE_INCS) -c $< -o $@

%.o : %.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

%.o : %.s
	$(EE_AS) $(EE_ASFLAGS) $< -o $@

$(EE_BIN) : $(EE_OBJS) $(PS2SDK)/ee/startup/crt0.o
	$(EE_CC) -nostartfiles -T$(PS2SDK)/ee/startup/linkfile $(EE_LDFLAGS) \
		-o $(EE_BIN) $(PS2SDK)/ee/startup/crt0.o $(EE_OBJS) $(EE_LIBS)

$(EE_LIB) : $(EE_OBJS)
	$(EE_AR) cru $(EE_LIB) $(EE_OBJS)
