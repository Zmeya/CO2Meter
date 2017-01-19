$(filter order-only, $(.FETAURES))

# Имя проекта
#-------------------------------------------------------------------------------
OUT_DIR := $(subst \,/,$(OUT_DIR))
TARGET  := $(OUT_DIR)\$(PROJECT_NAME)

OBJDIR = ./$(PLATFORM)/$(BUILD)

# Пути к CMSIS, HAL Lib
#-------------------------------------------------------------------------------
CMSIS_PATH   := ./Drivers/CMSIS
FREERTOS_PATH := ./FreeRTOS

DEFINES += STM32F103xB

# startup файл
#-------------------------------------------------------------------------------
STARTUP := $(CMSIS_PATH)/Source/startup_stm32f103xb.s

# Дефайны
#-------------------------------------------------------------------------------
DEFINES += 

# Инструменты
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

# Пути поиска исходных файлов
#-------------------------------------------------------------------------------
SOURCEDIRS := ./Source
#SOURCEDIRS += ./Drivers/ili9341/Source

SOURCEDIRS += $(CMSIS_PATH)/Source
SOURCEDIRS += $(FREERTOS_PATH)/Source

# Пути поиска хидеров
#-------------------------------------------------------------------------------
INCLUDES += ./Include
#INCLUDES += ./Drivers/ili9341/Include

INCLUDES += $(SOURCEDIRS)
INCLUDES += $(CMSIS_PATH)/Include
INCLUDES += $(FREERTOS_PATH)/Include

# Сторонние библиотеки
#-------------------------------------------------------------------------------
LIB_PATH += ./Library/ili9341

SOURCEDIRS += $(addsuffix /Source, $(LIB_PATH))
INCLUDES += $(addsuffix /Include, $(LIB_PATH))

# Библиотеки
#-------------------------------------------------------------------------------
LIBPATH +=
LIBS    += -lstdc++ -lsupc++ -lc -lrdimon

# Общие, для С и С++, настройки
#-------------------------------------------------------------------------------
FLAGS += -mcpu=cortex-m3 $(ARCH) # архитектура и система комманд
ifeq ($(BUILD),Debug)
FLAGS += -g3 -O0                 # Генерировать отладочную информацию
else
FLAGS += -Os -flto
FLAGS += -Wall -pedantic         # Выводить все предупреждения
endif
FLAGS += -fno-builtin
FLAGS += -ffunction-sections
FLAGS += -fdata-sections
FLAGS += -mfloat-abi=soft
FLAGS += -ffreestanding
FLAGS += --specs=nano.specs
# Настройки препроцессора
#-------------------------------------------------------------------------------
ifeq ($(BUILD),Debug)
DEFINES += DEBUG
else
DEFINES += RELEASE NDEBUG
endif
CPPFLAGS += $(addprefix -I, $(INCLUDES))
CPPFLAGS += $(addprefix -D, $(DEFINES))

# Настройки компилятора C
#-------------------------------------------------------------------------------
CFLAGS += -std=gnu99              # стандарт языка С

# Настройки компилятора C++
#-------------------------------------------------------------------------------
CXXFLAGS += -fpermissive -fno-rtti -fno-exceptions  -fno-use-cxa-atexit -fno-threadsafe-statics

# Скрипт линкера
#-------------------------------------------------------------------------------
LDSCR_PATH = $(CMSIS_PATH)/
LDSCRIPT   = stm32f103c8.ld

# Настройки линкера
#-------------------------------------------------------------------------------
LDFLAGS += -Wl,-Map=$(TARGET).map,--cref,--gc-sections
LDFLAGS += -fno-builtin -fno-rtti --specs=nano.specs --specs=rdimon.specs
LDFLAGS += $(ARCH)
LDFLAGS += -L$(LDSCR_PATH)
LDFLAGS += -T$(LDSCR_PATH)/$(LDSCRIPT)
LDFLAGS += $(addprefix -L, $(LIBPATH))
LDFLAGS += $(LIBS)
# Настройки ассемблера
#-------------------------------------------------------------------------------
AFLAGS += $(ARCH)

# Список объектных файлов
#-------------------------------------------------------------------------------
OBJS += $(patsubst %.s, %.o, $(STARTUP))
OBJS += $(patsubst %.c, %.o, $(wildcard  $(addsuffix /*.c, $(SOURCEDIRS))))
OBJS += $(patsubst %.cpp, %.o, $(wildcard  $(addsuffix /*.cpp, $(SOURCEDIRS))))
OUT_OBJ = $(addprefix $(OBJDIR)/, $(notdir $(OBJS)))

# Пути поиска make
#-------------------------------------------------------------------------------
VPATH := $(SOURCEDIRS)

# Собрать все
#-------------------------------------------------------------------------------
all: $(TARGET).hex $(TARGET).bin size
rebuild: clean all

# Очистка
#-------------------------------------------------------------------------------
clean:
	@echo Cleaning...
	-@$(RM) -f $(OUT_DIR)/*.elf $(OUT_DIR)/*.bin $(OUT_DIR)/*.hex $(OUT_DIR)/*.map $(OUT_DIR)/*.o $(OUT_DIR)/*.d

# Прошивка с помощью STLink V2
program: $(TARGET).bin
	@echo Programming with STLink V2
	-@$(STLINK) -c SWD Freq=4000 -Q -TVolt -P $(TARGET).bin 0x08000000 -V -Rst 

# Создание .hex файла
#-------------------------------------------------------------------------------
$(TARGET).hex: $(TARGET).elf
	@echo Generating $(notdir $@)...
	@$(CP) -Oihex $(TARGET).elf $(TARGET).hex

# Создание .bin файла
#-------------------------------------------------------------------------------
$(TARGET).bin: $(TARGET).elf size
	@echo Generating $(notdir $@)...
	@$(CP) -S -O binary $< $@

# Показываем размер
#-------------------------------------------------------------------------------
size:
	@echo ---------------------------------------------------
	@echo Code size:
	@$(SZ) $(TARGET).elf

# Линковка
#------------------------------------------------------------------------------- 
$(TARGET).elf: $(OUT_OBJ)
	@echo Linking $(notdir $@)...
	@$(LD) $(LDFLAGS) $^ -o $@ 2>&1 | sed -e "s/c:\([0-9]*\)/c(\1)/;/^ /d"

# Компиляция
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

# Сгенерированные gcc зависимости
#-------------------------------------------------------------------------------
include $(wildcart *.d)