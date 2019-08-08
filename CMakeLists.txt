cmake_minimum_required(VERSION 3.5)



set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



option(GAMEBOY_ADVANCE "GameboyAdvance" ON)



if(GAMEBOY_ADVANCE)

  set(BLINDJUMP_PLATFORM_SOURCE source/gba_platform.cpp)

  set(EXECUTABLE BlindJump.elf)
else()

  set(BLINDJUMP_PLATFORM_SOURCE source/desktop_platform.cpp)
endif()



# include_directories(PROJECT_SOURCE_DIR)
# configure_file("config.hpp.in" "source/config.hpp")



add_executable(${EXECUTABLE}
  source/enemies/dasher.cpp
  source/enemies/probe.cpp
  source/gba_platform.cpp
  source/collision.cpp
  source/itemChest.cpp
  source/easterEgg.cpp
  source/itemChest.cpp
  source/tileMap.cpp
  source/enemies/critter.cpp
  source/numeric.cpp
  source/camera.cpp
  source/random.cpp
  source/player.cpp
  source/enemies/turret.cpp
  source/entity.cpp
  source/sprite.cpp
  source/start.cpp
  source/item.cpp
  source/game.cpp
  source/view.cpp
  ${BLINDJUMP_PLATFORM_SOURCE})



if(GAMEBOY_ADVANCE)
  set(CMAKE_EXE_LINKER_FLAGS "-g -mthumb -mthumb-interwork -Wl,-Map,BlindJump.elf.map -specs=gba.specs"
    CACHE INTERNAL "" FORCE)

  enable_language(ASM)

  add_library(BlindJumpSprites
    source/bgr_spritesheet.s
    source/bgr_tilesheet.s)

  target_compile_options(BlindJumpSprites PRIVATE
    -x assembler-with-cpp -mthumb -mthumb-interwork)

  target_link_libraries(${EXECUTABLE} PRIVATE
    BlindJumpSprites
    ${DEVKITPRO}/libgba/lib/libgba.a)

  target_compile_options(${EXECUTABLE} PRIVATE
    -mthumb
    -mthumb-interwork
    -mcpu=arm7tdmi
    -mtune=arm7tdmi)

  add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMENT "running objcopy"
    COMMAND ${DEVKITARM}/bin/arm-none-eabi-objcopy -O binary ${EXECUTABLE} BlindJump.gba)
endif()



target_compile_options(${EXECUTABLE} PRIVATE
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
