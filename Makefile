$(filter order-only, $(.FETAURES))

# ��� �������
#-------------------------------------------------------------------------------
OUT_DIR := $(subst \,/,$(OUT_DIR))
TARGET  := $(OUT_DIR)\$(PROJECT_NAME)

OBJDIR = ./$(PLATFORM)/$(BUILD)

# ���� � CMSIS, HAL Lib
#-------------------------------------------------------------------------------
CMSIS_PATH   := ./Drivers/CMSIS
FREERTOS_PATH := ./FreeRTOS

DEFINES += STM32F103xB

# startup ����
#-------------------------------------------------------------------------------
STARTUP := $(CMSIS_PATH)/Source/startup_stm32f103xb.s

# �������
#-------------------------------------------------------------------------------
DEFINES += 

# �����������
#-------------------------------------------------------------------------------
ARCH = -mthumb -march=armv7-m
PREFIX = arm-none-eabi

AS = $(PREFIX)-gcc
CC = $(PREFIX)-gcc
LD = $(PREFIX)-gcc
CP = $(PREFIX)-objcopy
SZ = $(PREFIX)-size
RM = rm

STLINK = "D:/Tools/STLinkV2/ST-LINK Utility/ST-LINK_CLI.exe"

# ���� ������ �������� ������
#-------------------------------------------------------------------------------
SOURCEDIRS := ./Source
#SOURCEDIRS += ./Drivers/ili9341/Source

SOURCEDIRS += $(CMSIS_PATH)/Source
SOURCEDIRS += $(FREERTOS_PATH)/Source

# ���� ������ �������
#-------------------------------------------------------------------------------
INCLUDES += ./Include
#INCLUDES += ./Drivers/ili9341/Include

INCLUDES += $(SOURCEDIRS)
INCLUDES += $(CMSIS_PATH)/Include
INCLUDES += $(FREERTOS_PATH)/Include

# ��������� ����������
#-------------------------------------------------------------------------------
LIB_PATH += ./Library/ili9341

SOURCEDIRS += $(addsuffix /Source, $(LIB_PATH))
INCLUDES += $(addsuffix /Include, $(LIB_PATH))

# ����������
#-------------------------------------------------------------------------------
LIBPATH +=
LIBS    += -lstdc++ -lsupc++ -lc -lrdimon

# �����, ��� � � �++, ���������
#-------------------------------------------------------------------------------
FLAGS += -mcpu=cortex-m3 $(ARCH) # ����������� � ������� �������
ifeq ($(BUILD),Debug)
FLAGS += -g3 -O0                 # ������������ ���������� ����������
else
FLAGS += -Os -flto
FLAGS += -Wall -pedantic         # �������� ��� ��������������
endif
FLAGS += -fno-builtin
FLAGS += -ffunction-sections
FLAGS += -fdata-sections
FLAGS += -mfloat-abi=soft
FLAGS += -ffreestanding
FLAGS += --specs=nano.specs
# ��������� �������������
#-------------------------------------------------------------------------------
ifeq ($(BUILD),Debug)
DEFINES += DEBUG
else
DEFINES += RELEASE NDEBUG
endif
CPPFLAGS += $(addprefix -I, $(INCLUDES))
CPPFLAGS += $(addprefix -D, $(DEFINES))

# ��������� ����������� C
#-------------------------------------------------------------------------------
CFLAGS += -std=gnu99              # �������� ����� �

# ��������� ����������� C++
#-------------------------------------------------------------------------------
CXXFLAGS += -fpermissive -fno-rtti -fno-exceptions  -fno-use-cxa-atexit -fno-threadsafe-statics

# ������ �������
#-------------------------------------------------------------------------------
LDSCR_PATH = $(CMSIS_PATH)/
LDSCRIPT   = stm32f103c8.ld

# ��������� �������
#-------------------------------------------------------------------------------
LDFLAGS += -Wl,-Map=$(TARGET).map,--cref,--gc-sections
LDFLAGS += -fno-builtin -fno-rtti --specs=nano.specs --specs=rdimon.specs
LDFLAGS += $(ARCH)
LDFLAGS += -L$(LDSCR_PATH)
LDFLAGS += -T$(LDSCR_PATH)/$(LDSCRIPT)
LDFLAGS += $(addprefix -L, $(LIBPATH))
LDFLAGS += $(LIBS)
# ��������� ����������
#-------------------------------------------------------------------------------
AFLAGS += $(ARCH)

# ������ ��������� ������
#-------------------------------------------------------------------------------
OBJS += $(patsubst %.s, %.o, $(STARTUP))
OBJS += $(patsubst %.c, %.o, $(wildcard  $(addsuffix /*.c, $(SOURCEDIRS))))
OBJS += $(patsubst %.cpp, %.o, $(wildcard  $(addsuffix /*.cpp, $(SOURCEDIRS))))
OUT_OBJ = $(addprefix $(OBJDIR)/, $(notdir $(OBJS)))

# ���� ������ make
#-------------------------------------------------------------------------------
VPATH := $(SOURCEDIRS)

# ������� ���
#-------------------------------------------------------------------------------
all: $(TARGET).hex $(TARGET).bin size
rebuild: clean all

# �������
#-------------------------------------------------------------------------------
clean:
	@echo Cleaning...
	-@$(RM) -f $(OUT_DIR)/*.elf $(OUT_DIR)/*.bin $(OUT_DIR)/*.hex $(OUT_DIR)/*.map $(OUT_DIR)/*.o $(OUT_DIR)/*.d

# �������� � ������� STLink V2
program: $(TARGET).bin
	@echo Programming with STLink V2
	-@$(STLINK) -c SWD Freq=4000 -Q -TVolt -P $(TARGET).bin 0x08000000 -V -Rst 

# �������� .hex �����
#-------------------------------------------------------------------------------
$(TARGET).hex: $(TARGET).elf
	@echo Generating $(notdir $@)...
	@$(CP) -Oihex $(TARGET).elf $(TARGET).hex

# �������� .bin �����
#-------------------------------------------------------------------------------
$(TARGET).bin: $(TARGET).elf size
	@echo Generating $(notdir $@)...
	@$(CP) -S -O binary $< $@

# ���������� ������
#-------------------------------------------------------------------------------
size:
	@echo ---------------------------------------------------
	@echo Code size:
	@$(SZ) $(TARGET).elf

# ��������
#------------------------------------------------------------------------------- 
$(TARGET).elf: $(OUT_OBJ)
	@echo Linking $(notdir $@)...
	@$(LD) $(LDFLAGS) $^ -o $@ 2>&1 | sed -e "s/c:\([0-9]*\)/c(\1)/;/^ /d"

# ����������
#------------------------------------------------------------------------------- 
$(OBJDIR)/%.o: %.c
	@echo Compiling $(notdir $<)...
	@$(CC) $(FLAGS) $(CFLAGS) $(CPPFLAGS) -MD -c $< -o $@ 2>&1 | sed -e "s/c:\([0-9]*\)/c(\1)/;/^ /d"

$(OBJDIR)/%.o: %.cpp
	@echo Compiling $(notdir $<)...
	@$(CC) $(FLAGS) $(CXXFLAGS) $(CPPFLAGS) -MD -c $< -o $@ 2>&1 | sed -e "s/cpp:\([0-9]*\)/cpp(\1)/;/^ /d"

$(OBJDIR)/%.o: %.s
	@echo Translating $(notdir $<)...
	@$(AS) $(AFLAGS) -c $< -o $@ 2>&1 | sed -e "s/s:\([0-9]*\)/s(\1)/;/^ /d"

# ��������������� gcc �����������
#-------------------------------------------------------------------------------
include $(wildcart *.d)