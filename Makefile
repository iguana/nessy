# NESsy - NES ROM Build Chain
# Requires: cc65 toolchain, Python 3

.PHONY: all clean run chr

# Toolchain
CC65  := cc65
CA65  := ca65
LD65  := ld65

# Directories
SRCDIR := src
CFGDIR := cfg
CHRDIR := chr
BLDDIR := build
TOOLDIR := tools

# Output
ROM := $(BLDDIR)/nessy.nes

# Sources
C_SRCS  := $(wildcard $(SRCDIR)/*.c)
S_SRCS  := $(SRCDIR)/crt0.s $(SRCDIR)/neslib.s

# Generated assembly from C
C_ASM   := $(patsubst $(SRCDIR)/%.c,$(BLDDIR)/%.s,$(C_SRCS))

# Object files
C_OBJS  := $(patsubst $(BLDDIR)/%.s,$(BLDDIR)/%.o,$(C_ASM))
S_OBJS  := $(patsubst $(SRCDIR)/%.s,$(BLDDIR)/%.o,$(S_SRCS))
OBJS    := $(S_OBJS) $(C_OBJS)

# Linker config
LDCFG := $(CFGDIR)/nes.cfg

# CHR data
CHRBIN := $(CHRDIR)/ascii.chr

# Flags
CC65FLAGS := -t none -Oirs --cpu 6502
CA65FLAGS := -t none --cpu 6502
# Find cc65 library path (Homebrew default)
CC65_LIB := $(shell dirname $(shell which cc65) 2>/dev/null)/../share/cc65/lib
LD65FLAGS := -C $(LDCFG) -L $(CC65_LIB)

# ── Check toolchain ──────────────────────────────────────────────

check_cc65:
	@which $(CC65) > /dev/null 2>&1 || { \
		echo "cc65 not found. Installing via Homebrew..."; \
		brew install cc65 || { echo "ERROR: Failed to install cc65. Install manually: brew install cc65"; exit 1; }; \
	}

# ── Default target ───────────────────────────────────────────────

all: check_cc65 $(CHRBIN) $(ROM)
	@echo "Built $(ROM) ($$(wc -c < $(ROM) | tr -d ' ') bytes)"

# ── CHR generation ───────────────────────────────────────────────

chr: $(CHRBIN)

$(CHRBIN): $(TOOLDIR)/chr_gen.py
	@echo "Generating CHR font data..."
	python3 $(TOOLDIR)/chr_gen.py $@

# ── Compile C → assembly ─────────────────────────────────────────

$(BLDDIR)/%.s: $(SRCDIR)/%.c $(SRCDIR)/neslib.h | $(BLDDIR)
	$(CC65) $(CC65FLAGS) -o $@ $<

# ── Assemble → object ────────────────────────────────────────────

$(BLDDIR)/%.o: $(BLDDIR)/%.s | $(BLDDIR)
	$(CA65) $(CA65FLAGS) -o $@ $<

$(BLDDIR)/%.o: $(SRCDIR)/%.s $(CHRBIN) | $(BLDDIR)
	$(CA65) $(CA65FLAGS) -o $@ $<

# ── Link → ROM ───────────────────────────────────────────────────

$(ROM): $(OBJS) $(LDCFG)
	$(LD65) $(LD65FLAGS) -o $@ $(OBJS) none.lib

# ── Directory creation ───────────────────────────────────────────

$(BLDDIR):
	mkdir -p $(BLDDIR)

# ── Run in emulator ──────────────────────────────────────────────

run: all
	@echo "Opening $(ROM) in emulator..."
	@open $(ROM) 2>/dev/null || xdg-open $(ROM) 2>/dev/null || echo "No emulator found. Open $(ROM) manually."

# ── Clean ────────────────────────────────────────────────────────

clean:
	rm -rf $(BLDDIR)
	rm -f $(CHRBIN)
	@echo "Cleaned build artifacts."
