cmake_minimum_required(VERSION 3.5)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(SOURCE_DIR ${ROOT_DIR}/source)
set(IMAGE_DIR ${ROOT_DIR}/images)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_language(ASM)



option(GAMEBOY_ADVANCE "GameboyAdvance" ON)
option(GBA_AUTOBUILD_IMG "AutobuildImg" OFF)
option(GBA_AUTOBUILD_CONF "AutobuildConf" OFF)


if(GAMEBOY_ADVANCE AND NOT DEVKITARM)
  message(WARNING "Note: GAMEBOY_ADVANCE option is ON by default.")
  message(FATAL_ERROR "GAMEBOY_ADVANCE option is set, but missing ARM toolchain.")
endif()


if(GAMEBOY_ADVANCE AND WIN32)
  message(FATAL_ERROR "Gameboy Advance builds not supported in windows")
endif()


include_directories(${SOURCE_DIR})
# For emacs flycheck users
configure_file(dirlocals.in ${SOURCE_DIR}/.dir-locals.el)



set(SOURCES
  ${SOURCE_DIR}/entity/effects/firstExplorerSmallLaser.cpp
  ${SOURCE_DIR}/entity/effects/firstExplorerBigLaser.cpp
  ${SOURCE_DIR}/entity/bosses/theFirstExplorer.cpp
  ${SOURCE_DIR}/entity/details/cutsceneCloud.cpp
  ${SOURCE_DIR}/entity/details/cutsceneBird.cpp
  ${SOURCE_DIR}/entity/bosses/gatekeeper.cpp
  ${SOURCE_DIR}/entity/details/itemChest.cpp
  ${SOURCE_DIR}/entity/details/scavenger.cpp
  ${SOURCE_DIR}/entity/effects/explosion.cpp
  ${SOURCE_DIR}/entity/enemies/scarecrow.cpp
  ${SOURCE_DIR}/entity/enemies/sinkhole.cpp
  ${SOURCE_DIR}/entity/effects/orbshot.cpp
  ${SOURCE_DIR}/entity/details/lander.cpp
  ${SOURCE_DIR}/entity/enemies/common.cpp
  ${SOURCE_DIR}/entity/enemies/turret.cpp
  ${SOURCE_DIR}/entity/enemies/dasher.cpp
  ${SOURCE_DIR}/entity/details/rubble.cpp
  ${SOURCE_DIR}/entity/enemies/drone.cpp
  ${SOURCE_DIR}/entity/enemies/enemy.cpp
  ${SOURCE_DIR}/entity/enemies/theif.cpp
  ${SOURCE_DIR}/entity/enemies/snake.cpp
  ${SOURCE_DIR}/entity/effects/laser.cpp
  ${SOURCE_DIR}/entity/details/item.cpp
  ${SOURCE_DIR}/entity/bosses/boss.cpp
  ${SOURCE_DIR}/entity/peerPlayer.cpp
  ${SOURCE_DIR}/graphics/overlay.cpp
  ${SOURCE_DIR}/graphics/sprite.cpp
  ${SOURCE_DIR}/persistentData.cpp
  ${SOURCE_DIR}/number/numeric.cpp
  ${SOURCE_DIR}/number/random.cpp
  ${SOURCE_DIR}/graphics/view.cpp
  ${SOURCE_DIR}/entity/entity.cpp
  ${SOURCE_DIR}/network_event.cpp
  ${SOURCE_DIR}/entity/player.cpp
  ${SOURCE_DIR}/localization.cpp
  ${SOURCE_DIR}/state_impl.cpp
  ${SOURCE_DIR}/inventory.cpp
  ${SOURCE_DIR}/collision.cpp
  ${SOURCE_DIR}/powerup.cpp
  ${SOURCE_DIR}/tileMap.cpp
  ${SOURCE_DIR}/camera.cpp
  ${SOURCE_DIR}/string.cpp
  ${SOURCE_DIR}/start.cpp
  ${SOURCE_DIR}/game.cpp
  ${SOURCE_DIR}/conf.cpp)



if(GAMEBOY_ADVANCE)
  set(CONF_INI "${SOURCE_DIR}/../config.ini")
  set(CONF_SRC "${SOURCE_DIR}/config.cpp")

  add_custom_command(OUTPUT ${CONF_SRC}
    COMMAND echo "extern const unsigned char config_ini[] = {" > ${CONF_SRC}
    COMMAND cat ${CONF_INI} | xxd -i | tee -a ${CONF_SRC} > /dev/null
    COMMAND echo ", 0x00}\;" | tee -a ${CONF_SRC}
    DEPENDS ${CONF_INI})

  add_custom_target(generate_config DEPENDS ${CONF_SRC})

  set(SOURCES
    ${SOURCES}
    ${CONF_SRC}
    ${SOURCE_DIR}/platform/memcpy.s
    ${SOURCE_DIR}/platform/scottbuckley_omega.s
    ${SOURCE_DIR}/platform/scottbuckley_computations.s
    ${SOURCE_DIR}/platform/scottbuckley_hiraeth.s
    ${SOURCE_DIR}/platform/september.s
    ${SOURCE_DIR}/platform/clair_de_lune.s
    ${SOURCE_DIR}/graphics/overlay.s
    ${SOURCE_DIR}/graphics/overlay_cutscene.s
    ${SOURCE_DIR}/graphics/charset_en_spn_fr.s
    ${SOURCE_DIR}/graphics/tilesheet_intro_cutscene_flattened.s
    ${SOURCE_DIR}/graphics/tilesheet.s
    ${SOURCE_DIR}/graphics/tilesheet2.s
    ${SOURCE_DIR}/graphics/tilesheet3.s
    ${SOURCE_DIR}/graphics/tilesheet_top.s
    ${SOURCE_DIR}/graphics/tilesheet2_top.s
    ${SOURCE_DIR}/graphics/tilesheet3_top.s
    ${SOURCE_DIR}/graphics/spritesheet_intro_cutscene.s
    ${SOURCE_DIR}/graphics/spritesheet_intro_clouds.s
    ${SOURCE_DIR}/graphics/spritesheet.s
    ${SOURCE_DIR}/graphics/spritesheet2.s
    ${SOURCE_DIR}/graphics/spritesheet3.s
    ${SOURCE_DIR}/graphics/spritesheet_launch_anim.s
    ${SOURCE_DIR}/graphics/launch_flattened.s
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
    ${SOURCE_DIR}/graphics/blaster_info_flattened.s
    ${SOURCE_DIR}/graphics/seed_packet_flattened.s
    ${SOURCE_DIR}/graphics/overlay_network_flattened.s
    ${SOURCE_DIR}/platform/gba_platform.cpp)
else()
  set(SOURCES
    ${SOURCES}
    ${SOURCE_DIR}/platform/desktop_platform.cpp)
endif()



add_executable(BlindJump
  ${SOURCES})



function(compile_image filename mw mh flatten)
  if(${flatten} STREQUAL "YES")
    add_custom_command(OUTPUT ${SOURCE_DIR}/graphics/${filename}_flattened.s
      COMMAND python3 prep_image.py ${ROOT_DIR} ${filename}.png yes
      COMMAND grit tmp/${filename}_flattened.png -gB4 -gTFF00FF
      COMMAND mv ${filename}_flattened.s ${SOURCE_DIR}/graphics/${filename}_flattened.s
      COMMAND mv ${filename}_flattened.h ${SOURCE_DIR}/graphics/${filename}_flattened.h
      DEPENDS ${IMAGE_DIR}/${filename}.png)

    add_custom_target(compile_image_${filename} DEPENDS ${SOURCE_DIR}/graphics/${filename}_flattened.s)

  elseif(${mw} STREQUAL "0" AND ${mh} STREQUAL "0")
    add_custom_command(OUTPUT ${SOURCE_DIR}/graphics/${filename}.s
      COMMAND python3 prep_image.py ${ROOT_DIR} ${filename}.png no
      COMMAND grit tmp/${filename}.png -gB4 -gTFF00FF
      COMMAND mv ${filename}.s ${SOURCE_DIR}/graphics/${filename}.s
      COMMAND mv ${filename}.h ${SOURCE_DIR}/graphics/${filename}.h
      DEPENDS ${IMAGE_DIR}/${filename}.png)

    add_custom_target(compile_image_${filename} DEPENDS ${SOURCE_DIR}/graphics/${filename}.s)

  else()
    add_custom_command(OUTPUT ${SOURCE_DIR}/graphics/${filename}.s
      COMMAND python3 prep_image.py ${ROOT_DIR} ${filename}.png no
      COMMAND grit tmp/${filename}.png -gB4 -Mw ${mw} -Mh ${mh} -gTFF00FF
      COMMAND mv ${filename}.s ${SOURCE_DIR}/graphics/${filename}.s
      COMMAND mv ${filename}.h ${SOURCE_DIR}/graphics/${filename}.h
      DEPENDS ${IMAGE_DIR}/${filename}.png)

    add_custom_target(compile_image_${filename} DEPENDS ${SOURCE_DIR}/graphics/${filename}.s)

  endif()

  add_dependencies(BlindJump compile_image_${filename})
endfunction()


if(APPLE OR UNIX OR GAMEBOY_ADVANCE)

  set(SHARED_COMPILE_OPTIONS
    -g
    -Os
    -Wall
    -Werror
    -pedantic
    -nostdlib
    -ffast-math
    -fno-exceptions
    -fno-math-errno
    -Wdouble-promotion
    -fomit-frame-pointer
    -D__BLINDJUMP_ENABLE_LOGS)

elseif(WIN32)

  message("TODO: set up proper compiler flags for windows")
  set(SHARED_COMPILE_OPTIONS
    /W4)

endif()


if(GAMEBOY_ADVANCE)

  # I am setting CMAKE_AR in the toolchain file, but for some reason, the
  # variable no longer exists within the cmakelists... which is strange, because
  # other variables set in the toolchain file seem to be persistent...
  set(CMAKE_AR $ENV{DEVKITARM}/bin/arm-none-eabi-ar)

  add_library(ARM_ROUTINES ${SOURCE_DIR}/platform/gba_arm_routines.cpp)

  target_compile_options(ARM_ROUTINES PRIVATE
    ${SHARED_COMPILE_OPTIONS})

  target_compile_options(ARM_ROUTINES PRIVATE
    -marm
    -mlong-calls
    -mthumb-interwork
    -mcpu=arm7tdmi
    -mtune=arm7tdmi)


  add_executable(MultiTest ${SOURCE_DIR}/platform/multi_test.cpp)

  # target_compile_options(MultiTest PRIVATE
  #   ${SHARED_COMPILE_OPTIONS})

  target_compile_options(MultiTest PRIVATE
    -marm
    -mlong-calls
    -mthumb-interwork
    -mcpu=arm7tdmi
    -mtune=arm7tdmi)

  target_link_libraries(MultiTest PRIVATE
    ${DEVKITPRO}/libgba/lib/libgba.a)




  if(GBA_AUTOBUILD_IMG)
    compile_image(spritesheet_intro_clouds 2 4 NO)
    compile_image(spritesheet_intro_cutscene 2 4 NO)
    compile_image(spritesheet 2 4 NO)
    compile_image(spritesheet2 2 4 NO)
    compile_image(spritesheet3 2 4 NO)
    compile_image(spritesheet_boss0 2 4 NO)
    compile_image(spritesheet_boss1 2 4 NO)
    compile_image(spritesheet_launch_anim 2 4 NO)
    compile_image(launch 0 0 YES)
    compile_image(tilesheet_intro_cutscene 0 0 YES)
    compile_image(tilesheet 4 3 NO)
    compile_image(tilesheet2 4 3 NO)
    compile_image(tilesheet3 4 3 NO)
    compile_image(tilesheet_top 4 3 NO)
    compile_image(tilesheet2_top 4 3 NO)
    compile_image(tilesheet3_top 4 3 NO)
    compile_image(overlay 0 0 NO)
    compile_image(overlay_cutscene 0 0 NO)
    compile_image(charset_en_spn_fr 0 0 NO)
    compile_image(overlay_journal 0 0 NO)
    compile_image(old_poster 0 0 YES)
    compile_image(blaster_info 0 0 YES)
    compile_image(seed_packet 0 0 YES)
    compile_image(overlay_network 0 0 YES)
  endif()

  set(CMAKE_EXE_LINKER_FLAGS
    "-mthumb -mthumb-interwork -Wl,-Map,BlindJump.elf.map -specs=gba.specs"
    CACHE INTERNAL "" FORCE)

  if(GBA_AUTOBUILD_CONF)
    add_dependencies(BlindJump generate_config)
  endif()

  target_link_libraries(BlindJump PRIVATE
    ${DEVKITPRO}/libgba/lib/libgba.a
    ARM_ROUTINES)

  target_compile_options(BlindJump PRIVATE
    -mthumb
    -mthumb-interwork
    -mcpu=arm7tdmi
    -mtune=arm7tdmi)

  add_custom_command(TARGET BlindJump POST_BUILD
    COMMENT "building cartridge"
    COMMAND ${DEVKITARM}/bin/arm-none-eabi-objcopy -O binary BlindJump BlindJump.gba
    COMMAND ${DEVKITPRO}/tools/bin/gbafix -tBlindJump BlindJump.gba)

  add_custom_command(TARGET MultiTest POST_BUILD
    COMMENT "building cartridge"
    COMMAND ${DEVKITARM}/bin/arm-none-eabi-objcopy -O binary MultiTest MultiTest.gba
    COMMAND ${DEVKITPRO}/tools/bin/gbafix -tMultiTest MultiTest.gba)



  target_compile_options(BlindJump PRIVATE
    -D__GBA__)

elseif(APPLE)
  target_link_libraries(BlindJump
    "-framework sfml-window -framework sfml-graphics -framework sfml-system -framework sfml-audio -framework sfml-network")
  set_target_properties(BlindJump
    PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")

  include_directories("/usr/local/include/")
endif()



target_compile_options(BlindJump PRIVATE
  ${SHARED_COMPILE_OPTIONS})



file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE HEADERS "${SOURCE_DIR}/*.hpp")

add_custom_target(clang-format
  COMMAND clang-format --style=file -i ${SOURCES} ${HEADERS}
  COMMENT "Running clang-format"
  VERBATIM)
