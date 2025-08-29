#*******************************************************************************************#
# 该部分内容需要根据实际情况修改 Begin

# 更换BSP需要修改的部分
set(BOARD_NAME MC_Board)
set(CHIP_NAME STM32F446xx)
set(CMAKE_SYSTEM_PROCESSOR cortex-m4)

# FPU设置
add_compile_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
add_compile_definitions(ARM_MATH_CM4;ARM_MATH_MATRIX_CHECK;ARM_MATH_ROUNDING)

# 该部分内容需要根据实际情况修改 End
#*******************************************************************************************#
# 1. 定义干净的、全局的包含路径 (无 POSIX)

file(GLOB BSP_INCLUDE_DIRS
        "FineMote_BSP"
        "Drivers/CMSIS/Include"
        "Drivers/STM32F*xx_HAL_Driver/Inc"
        "Drivers/STM32F*xx_HAL_Driver/Inc/Legacy"
        "Drivers/CMSIS/Device/ST/STM32F*xx/Include"
        "Core/Inc"
        "Middlewares/Third_Party/FreeRTOS/Source/include"
        "Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS"
        "Middlewares/Third_Party/FreeRTOS/Source/portable/RVDS/ARM_CM4F"
)
include_directories(. ${BSP_INCLUDE_DIRS})


#*******************************************************************************************#
# 2. 定义源文件分组

# 组 A: 干净的源文件 (不依赖 POSIX)
file(GLOB_RECURSE CLEAN_SOURCES
        ${FINEMOTE_COMMON_SOURCE_FILES} # 来自顶层
        "Drivers/*.*"
        "Core/*.*"
        "MDK-ARM/startup_*.s"
        "./*.cpp"
        "Middlewares/*.*"
        # "lib/*.*" 这一行已被移除，因为 POSIX 源文件将由专门的目标处理
)

# 组 B: 依赖 POSIX 的源文件 (micro-ROS 相关的)
file(GLOB_RECURSE POSIX_CLIENT_SOURCES
        "Core/Src/custom_memory_manager.c"
        "Core/Src/allocators.c"
        "Core/Src/microros_transports.c"
)

# 组 C: POSIX 实现自身的源文件
file(GLOB_RECURSE FREERTOS_POSIX_SRCS
        "lib/FreeRTOS-Plus-POSIX/source/*.c"
)

# 从 CLEAN_SOURCES 中剔除干扰文件
file(GLOB_RECURSE EXCLUDE_SRCS
        "Core/Startup/*.*"
        "Core/Src/syscalls.c"
        "Core/Src/sysmem.c"
        "STM32*_FLASH.ld"
        "STM32*_RAM.ld"
)
list(REMOVE_ITEM CLEAN_SOURCES ${EXCLUDE_SRCS})
list(REMOVE_ITEM CLEAN_SOURCES ${POSIX_CLIENT_SOURCES})
# [新添加的修复代码]
# ------------------------------------------------------------------------------------------
# 由于 micro-ROS 使用了 custom_memory_manager.c 来替代标准的 FreeRTOS 堆实现，
# 我们必须从编译列表中移除所有标准的 heap_*.c 文件以避免多重定义错误。
file(GLOB STANDARD_HEAP_FILES "Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_*.c")
list(REMOVE_ITEM CLEAN_SOURCES ${STANDARD_HEAP_FILES})
# ------------------------------------------------------------------------------------------

#*******************************************************************************************#
# 3. 创建目标库

# 目标 A: 干净代码的静态库
add_library(clean_lib OBJECT ${CLEAN_SOURCES})
# [KEY CHANGE] 将 micro-ROS 的头文件路径添加到这个库
# 因为 freertos.c 在这个库里，并且需要 rcl/rcl.h
target_include_directories(clean_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/micro-ROS/microros_static_library/include
)


# 目标 B: POSIX 实现的静态库
add_library(freertos_posix OBJECT ${FREERTOS_POSIX_SRCS})
# 将所有 POSIX 相关的头文件路径作为 PUBLIC 属性暴露出来。
target_include_directories(freertos_posix PUBLIC
        lib/include
        lib/include/private
        lib/include/FreeRTOS_POSIX
        lib/include/FreeRTOS_POSIX/sys
        lib/FreeRTOS-Plus-POSIX/include
        lib/FreeRTOS-Plus-POSIX/include/portable
        lib/FreeRTOS-Plus-POSIX/include/portable/empty_portable
)


#*******************************************************************************************#
# 4. 创建最终的可执行文件
# [修改] add_executable 现在不仅包含自己的源文件，
# 还通过 $<TARGET_OBJECTS:...> 语法包含了 object 库的编译产物 (.o 文件)
add_executable(${BOARD_NAME}
        ${POSIX_CLIENT_SOURCES}
        $<TARGET_OBJECTS:clean_lib>
        $<TARGET_OBJECTS:freertos_posix>
)

#*******************************************************************************************#
# 5. 链接所有部分

# [修改] 由于 object 库的 .o 文件已经在 add_executable 中指定，
# 这里只需要链接真正的库文件和设置 include 路径即可
target_link_libraries(${BOARD_NAME} PRIVATE
        # 链接 micro-ROS 的预编译库
        ${CMAKE_CURRENT_SOURCE_DIR}/micro-ROS/microros_static_library/libmicroros.a
)

# 为可执行文件目标添加它编译时需要的头文件路径
# 它会从 object 库继承 PUBLIC 的 include 路径
# clean_lib 提供了 micro-ROS include
# freertos_posix 提供了 POSIX include
target_link_libraries(${BOARD_NAME} PRIVATE clean_lib freertos_posix)


#*******************************************************************************************#
# BSP 宏定义 (保持不变)
string(TOUPPER ${BOARD_NAME} UPPER_BOARD_NAME)
add_definitions(-DUSE_HAL_DRIVER -D${CHIP_NAME} -D__${UPPER_BOARD_NAME} -D__CMAKE_APP)

set(COMPILE_RULE_FLAG "-mcpu=${CMAKE_SYSTEM_PROCESSOR}")
set(LINKER_TARGET_FLAG "--cpu=${CMAKE_SYSTEM_PROCESSOR}")


#*******************************************************************************************#
# 设置编译和链接选项 (这部分保持不变)
set(CMAKE_C_FLAGS "${C_TARGET_FLAG} ${COMPILE_RULE_FLAG} \
    -fno-rtti -funsigned-char -fshort-enums  \
    -gdwarf-4 -Os -ffunction-sections \
    -Wno-packed -Wno-missing-variable-declarations -Wno-missing-prototypes -Wno-missing-noreturn -Wno-sign-conversion -Wno-nonportable-include-path -Wno-reserved-id-macro -Wno-unused-macros -Wno-documentation-unknown-command -Wno-documentation -Wno-license-management -Wno-parentheses-equality -Wno-reserved-identifier \
    ")
set(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS})
set(CMAKE_ASM_FLAGS "${ASM_TARGET_FLAG} ${COMPILE_RULE_FLAG} -masm=auto -c -gdwarf-4")

#*******************************************************************************************#
# 设置链接选项
set(SECTIONS_SCRIPT_PATH ${CMAKE_HOME_DIRECTORY}/Bsp/${BOARD_NAME}/MDK-ARM/${BOARD_NAME}/${BOARD_NAME}.sct)

set(CMAKE_EXE_linker_FLAGS " \
    ${LINKER_TARGET_FLAG} \
    --strict \
    --scatter ${SECTIONS_SCRIPT_PATH} \
    --info sizes --info totals --info unused --info veneers \
    --summary_stderr --info summarysizes --map --load_addr_map_info --xref --callgraph --symbols \
    --list ${CMAKE_BINARY_DIR}/bin/${BOARD_NAME}.map \
    " )


#*******************************************************************************************#
# 后续处理 (保持不变)
set(HEX_FILE ${CMAKE_BINARY_DIR}/bin/${BOARD_NAME}.hex)
set(BIN_FILE ${CMAKE_BINARY_DIR}/bin/${BOARD_NAME}.bin)
set(ELF_FILE ${CMAKE_BINARY_DIR}/bin/${BOARD_NAME}.elf)

# 使用 armclang 自带的 fromelf 工具，实现 elf 转 hex
set(ARMCC_fromelf fromelf.exe)
add_custom_command(TARGET ${BOARD_NAME} POST_BUILD
        # 相当于fromelf.exe" --i32combined --output="xxx/demo.hex" "xxx/demo.elf"
        COMMAND ${ARMCC_fromelf} --i32combined --output="${HEX_FILE}" "${ELF_FILE}"
        COMMENT "Building ${HEX_FILE}"
)