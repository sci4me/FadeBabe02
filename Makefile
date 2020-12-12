INCLUDE_DIR:=src
HDR_DIR:=$(INCLUDE_DIR)
ASMINC_DIR:=$(INCLUDE_DIR)
C_RUNTIME_DIR:=c_runtime
SRC_DIR:=src

CL65?=$(shell PATH="$(PATH)" which cl65)
ifeq ($(CL65),)
$(error The cl65 tool was not found. Try setting the CC65_DIR variable.)
endif

AR65?=$(shell PATH="$(PATH)" which ar65)
ifeq ($(AR65),)
$(error The ar65 tool was not found. Try setting the CC65_DIR variable.)
endif

ROM_SRCS+=$(wildcard $(ASMINC_DIR)/*.inc)
ROM_SRCS+=$(wildcard $(HDR_DIR)/*.h)
ROM_SRCS+=\
	$(wildcard $(SRC_DIR)/*/*.[cs])	\
	$(wildcard $(SRC_DIR)/*.[cs])
C_RUNTIME_SRCS+=$(wildcard $(C_RUNTIME_DIR)/*.s)

OBJECTS+=$(patsubst %.c,%.o,$(filter %.c,$(ROM_SRCS)))
OBJECTS+=$(patsubst %.s,%.o,$(filter %.s,$(ROM_SRCS)))
C_RUNTIME_OBJECTS+=$(patsubst %.s,%.o,$(filter %.s,$(C_RUNTIME_SRCS)))
CLEAN_FILES+=$(OBJECTS) $(C_RUNTIME_OBJECTS)

INC_FILES=$(filter %.inc,$(ROM_SRCS))
HDR_FILES=$(filter %.h,$(ROM_SRCS))

LINK_CONFIG := ld.cfg

CL65_FLAGS+=--cpu 65C02
CL65_FLAGS+=-O
CL65_FLAGS+=--standard cc65
CL65_FLAGS+=-t none
CL65_FLAGS+=-C "$(LINK_CONFIG)"
CL65_FLAGS+=--asm-include-dir $(ASMINC_DIR) -I $(HDR_DIR)

ROM_NAME:=FadeBabe
ROM_BIN:=$(ROM_NAME).bin
ROM_MAP:=$(ROM_NAME).map
CLEAN_FILES+=$(ROM_BIN)
CLEAN_FILES+=$(ROM_MAP)

CRT_LIB:=crt.lib
CLEAN_FILES+=$(CRT_LIB)

TTYUSB=/dev/ttyUSB0

.PHONY: all clean upload reupload xrb

all: $(ROM_BIN)
	@stat -c "%n %s" $(ROM_BIN)

clean:
	rm -f $(CLEAN_FILES)

upload: all
	socat exec:'sx -b $(ROM_BIN)' FILE:$(TTYUSB),b19200,raw

reupload: all xrb upload

xrb:
	echo -ne '$$XBOOT$$' > $(TTYUSB)

$(ROM_BIN): $(OBJECTS) $(CRT_LIB) $(LINK_CONFIG)
	$(CL65) $(CL65_FLAGS) -m $(ROM_MAP) -o "$@" $(OBJECTS) crt.lib

$(CRT_LIB): $(C_RUNTIME_OBJECTS)
	rm -f "$(CRT_LIB)"
	$(AR65) a "$(CRT_LIB)" $(C_RUNTIME_OBJECTS)

$(C_RUNTIME_DIR)/%.o: $(C_RUNTIME_DIR)/%.s $(LINK_CONFIG)
	$(CL65) $(CL65_FLAGS) -c -o "$@" "$<"

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(HDR_FILES) $(LINK_CONFIG)
	$(CL65) $(CL65_FLAGS) -l "$@".lst -c -o "$@" "$<"

$(SRC_DIR)/%.s: $(SRC_DIR)/%.c $(HDR_FILES) $(LINK_CONFIG)
	$(CL65) $(CL65_FLAGS) -S -o "$@" "$<"

$(SRC_DIR)/%.o: $(SRC_DIR)/%.s $(INC_FILES) $(LINK_CONFIG) src/main.fb
	$(CL65) $(CL65_FLAGS) -c -o "$@" "$<"
