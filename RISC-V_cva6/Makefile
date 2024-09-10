RISCV_XLEN ?= 64
RISCV_LIB  ?= elf

TARGET=/opt/riscv/bin/riscv${RISCV_XLEN}-unknown-${RISCV_LIB}
#-----------------------------------------------------------
GCC		= $(TARGET)-gcc
OBJCOPY	= $(TARGET)-objcopy
OBJDUMP	= $(TARGET)-objdump
AR		= $(TARGET)-ar

GCCVER 	= $(shell $(GCC) --version | grep gcc | cut -d" " -f9)

#64-bit
#RISCV ?= ~/riscv_fpga  
#32-bit
#RISCV ?= ~/riscv
RISCV_CCPATH = ${CCPATH}

BUILD_DIR       = build
FREERTOS_SOURCE_DIR = $(abspath ../../../../Source)
FREERTOS_TCP_SOURCE_DIR = $(abspath ../../../../../FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/source)
FREERTOS_TCP_STUBS_SOURCE_DIR = $(abspath ../../../../../FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/test/cbmc/stubs)
DEMO_SOURCE_DIR = $(abspath ../../../Common/Minimal)

INCLUDES = \
	-I. \
	-I$(FREERTOS_SOURCE_DIR)/include \
	-I$(FREERTOS_TCP_SOURCE_DIR)/include \
	-I$(FREERTOS_TCP_SOURCE_DIR)/portable/Compiler/GCC \
	-I$(FREERTOS_TCP_STUBS_SOURCE_DIR)/../include \
	-I$(FREERTOS_SOURCE_DIR)/../Demo/Common/include \
	-I$(FREERTOS_SOURCE_DIR)/portable/GCC/RISC-V \
	-I./uart 
#-----------------------------------------------------------


# cva6
CPPFLAGS = 

CFLAGS = \
	-Wall \
	$(INCLUDES) \
	-fomit-frame-pointer \
	-fno-strict-aliasing \
	-fno-builtin-printf \
	-D__gracefulExit \
	-mcmodel=medany \
	-march=rv64imad \
	-mabi=lp64d \
	-I $(FREERTOS_SOURCE_DIR)/portable/GCC/RISC-V/chip_specific_extensions/RV32I_CLINT_no_extensions


LDFLAGS	 = -T link.ld \
			-nostartfiles \
			-static \
			-fno-builtin-printf \
			-Xlinker --defsym=__stack_size=1024
			#-Xlinker -Map=RTOSDemo.map # output file

LIBS	 = -L$(RISCV_CCPATH)/lib/gcc/$(TARGET)/$(GCCVER) \
		   -L$(RISCV_CCPATH)/$(TARGET)/lib \
		   -lc \
		   -lgcc


DEBUG = 1
ifeq ($(DEBUG), 1)
    CFLAGS += -Og -ggdb3
else
    CFLAGS += -O2
endif

#-----------------------------------------------------------

FREERTOS_SRC = \
	$(FREERTOS_SOURCE_DIR)/croutine.c \
	$(FREERTOS_SOURCE_DIR)/event_groups.c \
	$(FREERTOS_SOURCE_DIR)/list.c \
	$(FREERTOS_SOURCE_DIR)/queue.c \
	$(FREERTOS_SOURCE_DIR)/stream_buffer.c \
	$(FREERTOS_SOURCE_DIR)/tasks.c \
	$(FREERTOS_SOURCE_DIR)/timers.c \
	$(FREERTOS_SOURCE_DIR)/portable/MemMang/heap_4.c \
	$(FREERTOS_SOURCE_DIR)/portable/GCC/RISC-V/port.c

FREERTOS_TCP_SRC = \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_ARP.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_BitConfig.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_DHCP.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_DHCPv6.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_DNS.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_DNS_Cache.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_DNS_Callback.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_DNS_Networking.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_DNS_Parser.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_ICMP.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_IP.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_IP_Timers.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_IP_Utils.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_IPv4.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_IPv4_Sockets.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_IPv4_Utils.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_IPv6.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_IPv6_Sockets.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_IPv6_Utils.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_ND.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_RA.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_Routing.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_Sockets.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_Stream_Buffer.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_TCP_IP.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_TCP_IP_IPv4.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_TCP_IP_IPv6.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_TCP_Reception.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_TCP_State_Handling.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_TCP_State_Handling_IPv4.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_TCP_State_Handling_IPv6.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_TCP_Transmission.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_TCP_Transmission_IPv4.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_TCP_Transmission_IPv6.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_TCP_Utils.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_TCP_Utils_IPv4.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_TCP_Utils_IPv6.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_TCP_WIN.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_Tiny_TCP.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_UDP_IP.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_UDP_IPv4.c \
	$(FREERTOS_TCP_SOURCE_DIR)/FreeRTOS_UDP_IPv6.c \
	$(FREERTOS_TCP_SOURCE_DIR)/portable/BufferManagement/BufferAllocation_2.c \
	test.c

AAA= $(FREERTOS_TCP_STUBS_SOURCE_DIR)/freertos_api.c

DEMO_SRC =  \
	$(DEMO_SOURCE_DIR)/blocktim.c \
	$(DEMO_SOURCE_DIR)/countsem.c \
	$(DEMO_SOURCE_DIR)/recmutex.c \
	$(DEMO_SOURCE_DIR)/death.c \
	$(DEMO_SOURCE_DIR)/TimerDemo.c \
	$(DEMO_SOURCE_DIR)/EventGroupsDemo.c \
	$(DEMO_SOURCE_DIR)/TaskNotify.c \
	$(DEMO_SOURCE_DIR)/GenQTest.c \
	$(DEMO_SOURCE_DIR)/BlockQ.c \
	$(DEMO_SOURCE_DIR)/PollQ.c \
	$(DEMO_SOURCE_DIR)/QPeek.c \
	$(DEMO_SOURCE_DIR)/QueueOverwrite.c \
	$(DEMO_SOURCE_DIR)/AbortDelay.c \
	$(DEMO_SOURCE_DIR)/semtest.c \
	$(DEMO_SOURCE_DIR)/StreamBufferDemo.c \
	$(DEMO_SOURCE_DIR)/StreamBufferInterrupt.c \
	$(DEMO_SOURCE_DIR)/dynamic.c \
	$(DEMO_SOURCE_DIR)/integer.c \
	$(DEMO_SOURCE_DIR)/MessageBufferDemo.c 


ASM_SRC = start.S \
		  vector.S \
		  RegTest.S \
	      $(FREERTOS_SOURCE_DIR)/portable/GCC/RISC-V/portASM.S 

APP_SRC = \
	main.c \
	TestRunner.c \
	main_blinky.c \
	RegTests.c \
	riscv-virt.c \
	uart/uart.c
	

OBJS = $(FREERTOS_SRC:%.c=$(BUILD_DIR)/%.o) \
	$(FREERTOS_TCP_SRC:%.c=$(BUILD_DIR)/%.o) \
	   $(DEMO_SRC:%.c=$(BUILD_DIR)/%.o) \
	   $(ASM_SRC:%.S=$(BUILD_DIR)/%.o) \
	   $(APP_SRC:%.c=$(BUILD_DIR)/%.o)

DEPS = $(FREERTOS_SRC:%.c=$(BUILD_DIR)/%.d) \
	$(FREERTOS_TCP_SRC:%.c=$(BUILD_DIR)/%.d) \
		$(DEMO_SRC:%.c=$(BUILD_DIR)/%.d) \
		$(ASM_SRC:%.S=$(BUILD_DIR)/%.d) \
		$(APP_SRC:%.c=$(BUILD_DIR)/%.d)

$(BUILD_DIR)/RTOSDemo.elf: $(OBJS) link.ld Makefile
	$(GCC) $(LDFLAGS) $(LIBS) $(OBJS) -o $@

$(BUILD_DIR)/%.o: %.c Makefile
	@mkdir -p $(@D)
	$(GCC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/%.o: %.S Makefile
	@mkdir -p $(@D)
	$(GCC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)

