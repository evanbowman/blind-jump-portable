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
  ${SOURCE_DIR}/entity/effects/firstExplorerSmallLaser.cpp
  ${SOURCE_DIR}/entity/effects/firstExplorerBigLaser.cpp
  ${SOURCE_DIR}/entity/bosses/theFirstExplorer.cpp
  ${SOURCE_DIR}/entity/bosses/gatekeeper.cpp
  ${SOURCE_DIR}/entity/details/itemChest.cpp
  ${SOURCE_DIR}/entity/effects/explosion.cpp
  ${SOURCE_DIR}/entity/enemies/scarecrow.cpp
  ${SOURCE_DIR}/entity/effects/orbshot.cpp
  ${SOURCE_DIR}/entity/enemies/common.cpp
  ${SOURCE_DIR}/entity/enemies/turret.cpp
  ${SOURCE_DIR}/entity/enemies/dasher.cpp
  ${SOURCE_DIR}/entity/details/rubble.cpp
  ${SOURCE_DIR}/entity/enemies/drone.cpp
  ${SOURCE_DIR}/entity/enemies/snake.cpp
  ${SOURCE_DIR}/entity/effects/laser.cpp
  ${SOURCE_DIR}/entity/details/item.cpp
  ${SOURCE_DIR}/entity/bosses/boss.cpp
  ${SOURCE_DIR}/graphics/overlay.cpp
  ${SOURCE_DIR}/graphics/sprite.cpp
  ${SOURCE_DIR}/number/numeric.cpp
  ${SOURCE_DIR}/number/random.cpp
  ${SOURCE_DIR}/graphics/view.cpp
  ${SOURCE_DIR}/entity/entity.cpp
  ${SOURCE_DIR}/entity/player.cpp
  ${SOURCE_DIR}/localization.cpp
  ${SOURCE_DIR}/inventory.cpp
  ${SOURCE_DIR}/collision.cpp
  ${SOURCE_DIR}/tileMap.cpp
  ${SOURCE_DIR}/camera.cpp
  ${SOURCE_DIR}/string.cpp
  ${SOURCE_DIR}/start.cpp
  ${SOURCE_DIR}/state.cpp
  ${SOURCE_DIR}/game.cpp)



if(GAMEBOY_ADVANCE)
  set(SOURCES
    ${SOURCES}
    ${SOURCE_DIR}/platform/memcpy.s
    ${SOURCE_DIR}/platform/scottbuckley_omega.s
    ${SOURCE_DIR}/platform/scottbuckley_computations.s
    ${SOURCE_DIR}/platform/lenkaland_frostellar.s
    ${SOURCE_DIR}/platform/september.s
    ${SOURCE_DIR}/platform/clair_de_lune.s
    ${SOURCE_DIR}/graphics/overlay.s
    ${SOURCE_DIR}/graphics/ascii.s
    ${SOURCE_DIR}/graphics/tilesheet.s
    ${SOURCE_DIR}/graphics/tilesheet2.s
    ${SOURCE_DIR}/graphics/tilesheet3.s
    ${SOURCE_DIR}/graphics/tilesheet_top.s
    ${SOURCE_DIR}/graphics/tilesheet2_top.s
    ${SOURCE_DIR}/graphics/tilesheet3_top.s
    ${SOURCE_DIR}/graphics/spritesheet.s
    ${SOURCE_DIR}/graphics/spritesheet2.s
    ${SOURCE_DIR}/graphics/spritesheet3.s
    ${SOURCE_DIR}/platform/sound_pop.s
    ${SOURCE_DIR}/platform/sound_coin.s
    ${SOURCE_DIR}/platform/sound_bell.s
    ${SOURCE_DIR}/platform/sound_click.s
    ${SOURCE_DIR}/platform/sound_heart.s
    ${SOURCE_DIR}/platform/sound_creak.s
    ${SOURCE_DIR}/platform/sound_laser1.s
    ${SOURCE_DIR}/platform/sound_openbag.s
    ${SOURCE_DIR}/platform/sound_blaster.s
    ${SOURCE_DIR}/platform/sound_footstep1.s
    ${SOURCE_DIR}/platform/sound_footstep2.s
    ${SOURCE_DIR}/platform/sound_footstep3.s
    ${SOURCE_DIR}/platform/sound_footstep4.s
    ${SOURCE_DIR}/platform/sound_open_book.s
    ${SOURCE_DIR}/platform/sound_explosion1.s
    ${SOURCE_DIR}/platform/sound_explosion2.s
    ${SOURCE_DIR}/graphics/overlay_journal.s
    ${SOURCE_DIR}/graphics/spritesheet_boss0.s
    ${SOURCE_DIR}/graphics/spritesheet_boss1.s
    ${SOURCE_DIR}/graphics/old_poster_flattened.s
    ${SOURCE_DIR}/graphics/seed_packet_flattened.s
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
    "-mthumb -mthumb-interwork -Wl,-Map,BlindJump.elf.map -specs=gba.specs"
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
  -g
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
  -Wdouble-promotion
  -fomit-frame-pointer
  -D__BLINDJUMP_ENABLE_LOGS)



file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE HEADERS "${SOURCE_DIR}/*.hpp")

add_custom_target(clang-format
  COMMAND clang-format --style=file -i ${SOURCES} ${HEADERS}
  COMMENT "Running clang-format"
  VERBATIM)
