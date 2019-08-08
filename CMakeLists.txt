cmake_minimum_required(VERSION 3.5)



set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



option(GAMEBOY_ADVANCE "GameboyAdvance" ON)



if(GAMEBOY_ADVANCE)
  set(DEVKITARM $ENV{DEVKITARM})
  set(DEVKITPRO $ENV{DEVKITPRO})

  if(NOT DEVKITARM)
    message(FATAL_ERROR "Please set DEVKITARM in your environment")
  endif(NOT DEVKITARM)

  set(CMAKE_C_COMPILER ${DEVKITARM}/bin/arm-none-eabi-gcc)
  set(CMAKE_CXX_COMPILER ${DEVKITARM}/bin/arm-none-eabi-g++)
  set(CMAKE_FIND_ROOT_PATH ${DEVKITARM})
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

  set(PKG_CONFIG_EXECUTABLE "arm-none-eabi-pkg-config")
  set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Shared libs not available" )
  enable_language(ASM)

  set(BLINDJUMP_PLATFORM_SOURCE source/gba_platform.cpp)
else()

  set(BLINDJUMP_PLATFORM_SOURCE source/desktop_platform.cpp)
endif()



include_directories(PROJECT_SOURCE_DIR)
configure_file("config.hpp.in" "source/config.hpp")




add_executable(BlindJump
  source/enemies/dasher.cpp
  source/enemies/probe.cpp
  source/bgr_spritesheet.s
  source/gba_platform.cpp
  source/bgr_tilesheet.s
  source/collision.cpp
  source/itemChest.cpp
  source/easterEgg.cpp
  source/itemChest.cpp
  source/tileMap.cpp
  source/critter.cpp
  source/numeric.cpp
  source/camera.cpp
  source/random.cpp
  source/player.cpp
  source/turret.cpp
  source/entity.cpp
  source/sprite.cpp
  source/start.cpp
  source/item.cpp
  source/game.cpp
  source/view.cpp
  ${BLINDJUMP_PLATFORM_SOURCE})



if(GAMEBOY_ADVANCE)
  set(CMAKE_EXE_LINKER_FLAGS "-g -mthumb -mthumb-interwork -specs=gba.specs"
    CACHE INTERNAL "" FORCE)

  target_link_libraries(BlindJump PRIVATE
    ${DEVKITPRO}/libgba/lib/libgba.a)

  target_compile_options(BlindJump PRIVATE
    -mthumb
    -mthumb-interwork
    -mcpu=arm7tdmi
    -mtune=arm7tdmi)

  add_custom_command(TARGET BlindJump POST_BUILD
    COMMENT "running objcopy"
    COMMAND ${DEVKITARM}/bin/arm-none-eabi-objcopy -O binary BlindJump BlindJump.gba)
endif()



target_compile_options(BlindJump PRIVATE
  -Os
  -Wall
  -flto
  -Werror
  -pedantic
  -D__GBA__
  -nostdlib
  -ffast-math
  -fno-exceptions
  -fno-math-errno
  -fomit-frame-pointer
  -D__BLINDJUMP_ENABLE_LOGS)
