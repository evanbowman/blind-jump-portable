cmake_minimum_required(VERSION 3.5)



set(SOURCE_DIR ${PROJECT_BINARY_DIR}/../source)



set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_language(ASM)



option(GAMEBOY_ADVANCE "GameboyAdvance" ON)



if(GAMEBOY_ADVANCE AND NOT DEVKITARM)
  message(WARNING "Note: GAMEBOY_ADVANCE option is ON by default.")
  message(FATAL_ERROR "GAMEBOY_ADVANCE option is set, but missing ARM toolchain.")
endif()



include_directories(${SOURCE_DIR})
# For emacs flycheck users
configure_file(dirlocals.in ${SOURCE_DIR}/.dir-locals.el)



set(SOURCES
  ${SOURCE_DIR}/entity/details/itemChest.cpp
  ${SOURCE_DIR}/entity/enemies/critter.cpp
  ${SOURCE_DIR}/entity/effects/orbshot.cpp
  ${SOURCE_DIR}/entity/enemies/turret.cpp
  ${SOURCE_DIR}/entity/enemies/dasher.cpp
  ${SOURCE_DIR}/entity/enemies/probe.cpp
  ${SOURCE_DIR}/entity/enemies/snake.cpp
  ${SOURCE_DIR}/entity/effects/laser.cpp
  ${SOURCE_DIR}/entity/details/item.cpp
  ${SOURCE_DIR}/graphics/overlay.cpp
  ${SOURCE_DIR}/graphics/sprite.cpp
  ${SOURCE_DIR}/number/numeric.cpp
  ${SOURCE_DIR}/number/random.cpp
  ${SOURCE_DIR}/graphics/view.cpp
  ${SOURCE_DIR}/entity/entity.cpp
  ${SOURCE_DIR}/entity/player.cpp
  ${SOURCE_DIR}/collision.cpp
  ${SOURCE_DIR}/easterEgg.cpp
  ${SOURCE_DIR}/tileMap.cpp
  ${SOURCE_DIR}/camera.cpp
  ${SOURCE_DIR}/start.cpp
  ${SOURCE_DIR}/state.cpp
  ${SOURCE_DIR}/game.cpp)



if(GAMEBOY_ADVANCE)
  set(SOURCES
    ${SOURCES}
    ${SOURCE_DIR}/graphics/bgr_overlay.s
    ${SOURCE_DIR}/graphics/bgr_tilesheet.s
    ${SOURCE_DIR}/graphics/bgr_spritesheet.s
    ${SOURCE_DIR}/platform/gba_platform.cpp)
else()
  set(SOURCES
    ${SOURCES}
    ${SOURCE_DIR}/platform/desktop_platform.cpp)
endif()



add_executable(BlindJump
  ${SOURCES})



if(GAMEBOY_ADVANCE)
  set(CMAKE_EXE_LINKER_FLAGS
    "-g -mthumb -mthumb-interwork -Wl,-Map,BlindJump.elf.map -specs=gba.specs"
    CACHE INTERNAL "" FORCE)

  target_link_libraries(BlindJump PRIVATE
    ${DEVKITPRO}/libgba/lib/libgba.a)

  target_compile_options(BlindJump PRIVATE
    -mthumb
    -mthumb-interwork
    -mcpu=arm7tdmi
    -mtune=arm7tdmi)

  add_custom_command(TARGET BlindJump POST_BUILD
    COMMENT "building cartridge"
    COMMAND ${DEVKITARM}/bin/arm-none-eabi-objcopy -O binary BlindJump BlindJump.gba
    COMMAND ${DEVKITPRO}/tools/bin/gbafix -tBlindJump BlindJump.gba)
elseif(APPLE)
  target_link_libraries(BlindJump
    "-framework sfml-window -framework sfml-graphics -framework sfml-system -framework sfml-audio")
  set_target_properties(BlindJump
    PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")

  include_directories("/usr/local/include/")
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



file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE HEADERS "${SOURCE_DIR}/*.hpp")

add_custom_target(clang-format
  COMMAND clang-format --style=file -i ${SOURCES} ${HEADERS}
  COMMENT "Running clang-format"
  VERBATIM)
