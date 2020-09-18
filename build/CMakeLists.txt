cmake_minimum_required(VERSION 3.5)

project(BlindJump)

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
include_directories(${ROOT_DIR}/external/)


if(APPLE)
  set(EMACS_FLYCHECK_FRONTEND clang)
else()
  set(EMACS_FLYCHECK_FRONTEND gcc)
endif()

# For emacs flycheck users
configure_file(dirlocals.in ${SOURCE_DIR}/.dir-locals.el)



set(SOURCES
  ${SOURCE_DIR}/entity/effects/wandererSmallLaser.cpp
  ${SOURCE_DIR}/entity/effects/wandererBigLaser.cpp
  ${SOURCE_DIR}/entity/effects/conglomerateShot.cpp
  ${SOURCE_DIR}/entity/details/cutsceneCloud.cpp
  ${SOURCE_DIR}/entity/details/cutsceneBird.cpp
  ${SOURCE_DIR}/entity/bosses/gatekeeper.cpp
  ${SOURCE_DIR}/entity/details/itemChest.cpp
  ${SOURCE_DIR}/entity/details/scavenger.cpp
  ${SOURCE_DIR}/entity/effects/explosion.cpp
  ${SOURCE_DIR}/entity/enemies/scarecrow.cpp
  ${SOURCE_DIR}/entity/enemies/sinkhole.cpp
  ${SOURCE_DIR}/entity/bosses/wanderer.cpp
  ${SOURCE_DIR}/entity/bosses/theTwins.cpp
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
  ${SOURCE_DIR}/state/state_impl.cpp
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
  ${SOURCE_DIR}/inventory.cpp
  ${SOURCE_DIR}/collision.cpp
  ${SOURCE_DIR}/powerup.cpp
  ${SOURCE_DIR}/tileMap.cpp
  ${SOURCE_DIR}/camera.cpp
  ${SOURCE_DIR}/string.cpp
  ${SOURCE_DIR}/start.cpp
  ${SOURCE_DIR}/path.cpp
  ${SOURCE_DIR}/game.cpp)


set(SOURCES
  ${SOURCES}
  ${SOURCE_DIR}/script/lisp.cpp)


set(SOURCES
  ${SOURCES}
  ${SOURCE_DIR}/state/quickSelectInventoryState.cpp
  ${SOURCE_DIR}/state/networkConnectSetupState.cpp
  ${SOURCE_DIR}/state/networkConnectWaitState.cpp
  ${SOURCE_DIR}/state/bossDeathSequenceState.cpp
  ${SOURCE_DIR}/state/launchCutsceneState.cpp
  ${SOURCE_DIR}/state/deathContinueState.cpp
  ${SOURCE_DIR}/state/endingCreditsState.cpp
  ${SOURCE_DIR}/state/logfileViewerState.cpp
  ${SOURCE_DIR}/state/preFadePauseState.cpp
  ${SOURCE_DIR}/state/introCreditsState.cpp
  ${SOURCE_DIR}/state/editSettingsState.cpp
  ${SOURCE_DIR}/state/newLevelIdleState.cpp
  ${SOURCE_DIR}/state/signalJammerState.cpp
  ${SOURCE_DIR}/state/respawnWaitState.cpp
  ${SOURCE_DIR}/state/commandCodeState.cpp
  ${SOURCE_DIR}/state/pauseScreenState.cpp
  ${SOURCE_DIR}/state/remoteReplState.cpp
  ${SOURCE_DIR}/state/imageViewState.cpp
  ${SOURCE_DIR}/state/inventoryState.cpp
  ${SOURCE_DIR}/state/overworldState.cpp
  ${SOURCE_DIR}/state/mapSystemState.cpp
  ${SOURCE_DIR}/state/deathFadeState.cpp
  ${SOURCE_DIR}/state/lispReplState.cpp
  ${SOURCE_DIR}/state/newLevelState.cpp
  ${SOURCE_DIR}/state/quickMapState.cpp
  ${SOURCE_DIR}/state/notebookState.cpp
  ${SOURCE_DIR}/state/glowFadeState.cpp
  ${SOURCE_DIR}/state/itemShopState.cpp
  ${SOURCE_DIR}/state/fadeOutState.cpp
  ${SOURCE_DIR}/state/goodbyeState.cpp
  ${SOURCE_DIR}/state/activeState.cpp
  ${SOURCE_DIR}/state/fadeInState.cpp
  ${SOURCE_DIR}/state/warpInState.cpp)


set(SCRIPTS_DIR ${SOURCE_DIR}/../scripts)
set(SCRIPTS "")
set(SCRIPT_DECLS "")


macro(add_script script)

  set(SCRIPTS ${SCRIPTS} "\n    {\"${script}.lisp\", script_${script}},\n//")
  set(SCRIPT_DECLS ${SCRIPT_DECLS}
    "\nextern const unsigned char script_${script}[];\n//")

  if(GBA_AUTOBUILD_CONF)

    set(SCRIPT_IN ${SCRIPTS_DIR}/${script}.lisp)
    set(SCRIPT_OUT ${SOURCE_DIR}/data/script_${script}.cpp)

    add_custom_command(OUTPUT ${SCRIPT_OUT}
      COMMAND echo "extern const unsigned char script_${script}[] = {" > ${SCRIPT_OUT}
      COMMAND cat ${SCRIPT_IN} | xxd -i | tee -a ${SCRIPT_OUT} > /dev/null
      COMMAND echo ", 0x00}\;" | tee -a ${SCRIPT_OUT}
      DEPENDS ${SCRIPT_IN})

    add_custom_target(convert_${script}_script DEPENDS ${SCRIPT_OUT})

    add_dependencies(BlindJump convert_${script}_script)

  endif()
endmacro()


if(GAMEBOY_ADVANCE)

  set(DATA_DIR ${SOURCE_DIR}/data/)

  set(SOURCES
    ${SOURCES}
    ${SOURCE_DIR}/platform/gba/gba_platform.cpp
    ${SOURCE_DIR}/platform/gba/memcpy.s
    ${SOURCE_DIR}/platform/gba/interrupt.c
    ${SOURCE_DIR}/platform/gba/interrupt_dispatch.s
    ${SOURCE_DIR}/platform/gba/bios_math.s
    ${SOURCE_DIR}/platform/gba/reset.s

    # Data files below. Because the gameboy advance does not have a filesystem,
    # all of the sounds and images need to be compiled into the ROM.
    ${DATA_DIR}/script_init.cpp
    ${DATA_DIR}/script_pre_levelgen.cpp
    ${DATA_DIR}/script_post_levelgen.cpp
    ${DATA_DIR}/scottbuckley_omega.s
    ${DATA_DIR}/music_murmuration.s
    ${DATA_DIR}/scottbuckley_computations.s
    ${DATA_DIR}/scottbuckley_hiraeth.s
    ${DATA_DIR}/clair_de_lune.s
    ${DATA_DIR}/overlay.s
    ${DATA_DIR}/repl.s
    ${DATA_DIR}/overlay_cutscene.s
    ${DATA_DIR}/charset_en_spn_fr.s
    ${DATA_DIR}/tilesheet_intro_cutscene_flattened.s
    ${DATA_DIR}/tilesheet.s
    ${DATA_DIR}/tilesheet2.s
    ${DATA_DIR}/tilesheet3.s
    ${DATA_DIR}/tilesheet4.s
    ${DATA_DIR}/tilesheet_top.s
    ${DATA_DIR}/tilesheet2_top.s
    ${DATA_DIR}/tilesheet3_top.s
    ${DATA_DIR}/tilesheet4_top.s
    ${DATA_DIR}/spritesheet_intro_cutscene.s
    ${DATA_DIR}/spritesheet_intro_clouds.s
    ${DATA_DIR}/spritesheet.s
    ${DATA_DIR}/spritesheet2.s
    ${DATA_DIR}/spritesheet3.s
    ${DATA_DIR}/spritesheet4.s
    ${DATA_DIR}/spritesheet_launch_anim.s
    ${DATA_DIR}/launch_flattened.s
    ${DATA_DIR}/sound_pop.s
    ${DATA_DIR}/sound_coin.s
    ${DATA_DIR}/sound_bell.s
    ${DATA_DIR}/sound_click.s
    ${DATA_DIR}/sound_heart.s
    ${DATA_DIR}/sound_creak.s
    ${DATA_DIR}/sound_dropitem.s
    ${DATA_DIR}/sound_scroll.s
    ${DATA_DIR}/sound_laser1.s
    ${DATA_DIR}/sound_select.s
    ${DATA_DIR}/sound_openbag.s
    ${DATA_DIR}/sound_blaster.s
    ${DATA_DIR}/sound_tw_bell.s
    ${DATA_DIR}/sound_footstep1.s
    ${DATA_DIR}/sound_footstep2.s
    ${DATA_DIR}/sound_footstep3.s
    ${DATA_DIR}/sound_footstep4.s
    ${DATA_DIR}/sound_open_book.s
    ${DATA_DIR}/sound_explosion1.s
    ${DATA_DIR}/sound_typewriter.s
    ${DATA_DIR}/sound_explosion2.s
    ${DATA_DIR}/overlay_journal.s
    ${DATA_DIR}/spritesheet_boss0.s
    ${DATA_DIR}/spritesheet_boss1.s
    ${DATA_DIR}/spritesheet_boss2.s
    ${DATA_DIR}/spritesheet_boss2_done.s
    ${DATA_DIR}/spritesheet_boss2_mutate.s
    ${DATA_DIR}/spritesheet_boss2_final.s
    ${DATA_DIR}/old_poster_flattened.s
    ${DATA_DIR}/postal_advert_flattened.s
    ${DATA_DIR}/blaster_info_flattened.s
    ${DATA_DIR}/seed_packet_flattened.s
    ${DATA_DIR}/overlay_network_flattened.s)
else()
  set(SOURCES
    ${SOURCES}
    ${SOURCE_DIR}/platform/desktop/desktop_platform.cpp
    ${SOURCE_DIR}/platform/desktop/resource_path.cpp)
endif()


if(APPLE)
  add_executable(BlindJump MACOSX_BUNDLE ${SOURCES})


else()
  add_executable(BlindJump
    ${SOURCES})
endif()


set(IMAGE_INCLUDES "")
set(IMAGE_SPR_STUBS "")
set(IMAGE_TILE_STUBS "")
set(IMAGE_OVERLAY_STUBS "")

macro(add_spritesheet filename mw mh flatten)

  set(IMAGE_INCLUDES ${IMAGE_INCLUDES}
    "\n#include \"data/${filename}.h\"\n//")

  set(IMAGE_SPR_STUBS ${IMAGE_SPR_STUBS}
    "\n    TEXTURE_INFO(${filename}),\n//")

  compile_image(${filename} ${mw} ${mh} ${flatten})
endmacro()


macro(add_tilesheet filename mw mh flatten)
  if(${flatten} STREQUAL "YES")
    set(FLATTENED_SUFFIX "_flattened")
  else()
    set(FLATTENED_SUFFIX "")
  endif()

  set(IMAGE_INCLUDES ${IMAGE_INCLUDES}
    "\n#include \"data/${filename}${FLATTENED_SUFFIX}.h\"\n//")

  set(IMAGE_TILE_STUBS ${IMAGE_TILE_STUBS}
    "\n    TEXTURE_INFO(${filename}${FLATTENED_SUFFIX}),\n//")

  compile_image(${filename} ${mw} ${mh} ${flatten})
endmacro()


macro(add_overlay filename mw mh flatten)
  if(${flatten} STREQUAL "YES")
    set(FLATTENED_SUFFIX "_flattened")
  else()
    set(FLATTENED_SUFFIX "")
  endif()

  set(IMAGE_INCLUDES ${IMAGE_INCLUDES}
    "\n#include \"data/${filename}${FLATTENED_SUFFIX}.h\"\n//")

  set(IMAGE_OVERLAY_STUBS ${IMAGE_OVERLAY_STUBS}
    "\n    TEXTURE_INFO(${filename}${FLATTENED_SUFFIX}),\n//")

  compile_image(${filename} ${mw} ${mh} ${flatten})
endmacro()


function(compile_image filename mw mh flatten)
  if(${flatten} STREQUAL "YES")
    add_custom_command(OUTPUT ${SOURCE_DIR}/data/${filename}_flattened.s
      COMMAND python3 prep_image.py ${ROOT_DIR} ${filename}.png yes
      COMMAND ${DEVKITPRO}/tools/bin/grit tmp/${filename}_flattened.png -gB4 -gTFF00FF
      COMMAND mv ${filename}_flattened.s ${SOURCE_DIR}/data/${filename}_flattened.s
      COMMAND mv ${filename}_flattened.h ${SOURCE_DIR}/data/${filename}_flattened.h
      DEPENDS ${IMAGE_DIR}/${filename}.png)

    add_custom_target(compile_image_${filename} DEPENDS ${SOURCE_DIR}/data/${filename}_flattened.s)

  elseif(${mw} STREQUAL "0" AND ${mh} STREQUAL "0")
    add_custom_command(OUTPUT ${SOURCE_DIR}/data/${filename}.s
      COMMAND python3 prep_image.py ${ROOT_DIR} ${filename}.png no
      COMMAND ${DEVKITPRO}/tools/bin/grit  tmp/${filename}.png -gB4 -gTFF00FF
      COMMAND mv ${filename}.s ${SOURCE_DIR}/data/${filename}.s
      COMMAND mv ${filename}.h ${SOURCE_DIR}/data/${filename}.h
      DEPENDS ${IMAGE_DIR}/${filename}.png)

    add_custom_target(compile_image_${filename} DEPENDS ${SOURCE_DIR}/data/${filename}.s)

  else()
    add_custom_command(OUTPUT ${SOURCE_DIR}/data/${filename}.s
      COMMAND python3 prep_image.py ${ROOT_DIR} ${filename}.png no
      COMMAND ${DEVKITPRO}/tools/bin/grit tmp/${filename}.png -gB4 -Mw ${mw} -Mh ${mh} -gTFF00FF
      COMMAND mv ${filename}.s ${SOURCE_DIR}/data/${filename}.s
      COMMAND mv ${filename}.h ${SOURCE_DIR}/data/${filename}.h
      DEPENDS ${IMAGE_DIR}/${filename}.png)

    add_custom_target(compile_image_${filename} DEPENDS ${SOURCE_DIR}/data/${filename}.s)

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
    -fno-math-errno
    -Wdouble-promotion
    -fomit-frame-pointer
    -D__BLINDJUMP_ENABLE_LOGS)

  if(GAMEBOY_ADVANCE)
    set(SHARED_COMPILE_OPTIONS
      ${SHARED_COMPILE_OPTIONS}
      -fno-exceptions)
  endif()

elseif(WIN32)

  set(SHARED_COMPILE_OPTIONS
    /W4)

  include_directories(${ROOT_DIR}/external/SFML-2.5.1/include/)

  set(SFML_LIB_DIR ${ROOT_DIR}/external/SFML-2.5.1/lib/Release)

  target_link_libraries(BlindJump
    ${SFML_LIB_DIR}/sfml-graphics.lib
    ${SFML_LIB_DIR}/sfml-window.lib
    ${SFML_LIB_DIR}/sfml-system.lib
    ${SFML_LIB_DIR}/sfml-audio.lib
    ${SFML_LIB_DIR}/sfml-network.lib)

  add_custom_command(TARGET BlindJump POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SFML_LIB_DIR}/sfml-graphics-2.dll"
        $<TARGET_FILE_DIR:BlindJump>)

  add_custom_command(TARGET BlindJump POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SFML_LIB_DIR}/sfml-audio-2.dll"
        $<TARGET_FILE_DIR:BlindJump>)

  add_custom_command(TARGET BlindJump POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SFML_LIB_DIR}/sfml-window-2.dll"
        $<TARGET_FILE_DIR:BlindJump>)

  add_custom_command(TARGET BlindJump POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SFML_LIB_DIR}/sfml-network-2.dll"
        $<TARGET_FILE_DIR:BlindJump>)

  add_custom_command(TARGET BlindJump POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SFML_LIB_DIR}/sfml-system-2.dll"
        $<TARGET_FILE_DIR:BlindJump>)

   add_custom_command(TARGET BlindJump POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ROOT_DIR}/external/SFML-2.5.1/extlibs/bin/x64/openal32.dll"
        $<TARGET_FILE_DIR:BlindJump>)
endif()


if(GAMEBOY_ADVANCE)

  # I am setting CMAKE_AR in the toolchain file, but for some reason, the
  # variable no longer exists within the cmakelists... which is strange, because
  # other variables set in the toolchain file seem to be persistent...
  set(CMAKE_AR $ENV{DEVKITARM}/bin/arm-none-eabi-ar)

  add_library(ARM_ROUTINES ${SOURCE_DIR}/platform/gba/gba_arm_routines.cpp)

  target_compile_options(ARM_ROUTINES PRIVATE
    ${SHARED_COMPILE_OPTIONS})

  target_compile_options(ARM_ROUTINES PRIVATE
    -marm
    -mlong-calls
    -mthumb-interwork
    -mcpu=arm7tdmi
    -mtune=arm7tdmi)

  add_script(init)
  add_script(pre_levelgen)
  add_script(post_levelgen)

  if(GBA_AUTOBUILD_CONF)
    configure_file(scripts.cpp.in ${SOURCE_DIR}/platform/gba/scripts.cpp)
  endif()

  if(GBA_AUTOBUILD_IMG)
    add_spritesheet(spritesheet_intro_clouds 2 4 NO)
    add_spritesheet(spritesheet_intro_cutscene 2 4 NO)
    add_spritesheet(spritesheet 2 4 NO)
    add_spritesheet(spritesheet2 2 4 NO)
    add_spritesheet(spritesheet3 2 4 NO)
    add_spritesheet(spritesheet4 2 4 NO)
    add_spritesheet(spritesheet_boss0 2 4 NO)
    add_spritesheet(spritesheet_boss1 2 4 NO)
    add_spritesheet(spritesheet_boss2 2 4 NO)
    add_spritesheet(spritesheet_boss2_done 2 4 NO)
    add_spritesheet(spritesheet_boss2_mutate 2 4 NO)
    add_spritesheet(spritesheet_boss2_final 2 4 NO)
    add_spritesheet(spritesheet_launch_anim 2 4 NO)
    add_tilesheet(launch 0 0 YES)
    add_tilesheet(tilesheet_intro_cutscene 0 0 YES)
    add_tilesheet(tilesheet 4 3 NO)
    add_tilesheet(tilesheet2 4 3 NO)
    add_tilesheet(tilesheet3 4 3 NO)
    add_tilesheet(tilesheet4 4 3 NO)
    add_tilesheet(tilesheet_top 4 3 NO)
    add_tilesheet(tilesheet2_top 4 3 NO)
    add_tilesheet(tilesheet3_top 4 3 NO)
    add_tilesheet(tilesheet4_top 4 3 NO)
    add_overlay(overlay 0 0 NO)
    add_overlay(repl 0 0 NO)
    add_overlay(overlay_cutscene 0 0 NO)
    add_overlay(charset_en_spn_fr 0 0 NO)
    add_overlay(overlay_journal 0 0 NO)
    add_overlay(old_poster 0 0 YES)
    add_overlay(postal_advert 0 0 YES)
    add_overlay(blaster_info 0 0 YES)
    add_overlay(seed_packet 0 0 YES)
    add_overlay(overlay_network 0 0 YES)

    configure_file(images.cpp.in ${SOURCE_DIR}/platform/gba/images.cpp)
  endif()

  set(CMAKE_EXE_LINKER_FLAGS
    "-mthumb -mthumb-interwork -Wl,-Map,BlindJump.elf.map -specs=gba.specs"
    CACHE INTERNAL "" FORCE)

  target_link_libraries(BlindJump PRIVATE
    # ${DEVKITPRO}/libgba/lib/libgba.a
    ARM_ROUTINES)

  target_compile_options(BlindJump PRIVATE
    -mthumb
    -mthumb-interwork
    -mcpu=arm7tdmi
    -mtune=arm7tdmi)

  add_custom_command(TARGET BlindJump POST_BUILD
    COMMENT "building cartridge"
    COMMAND ${DEVKITARM}/bin/arm-none-eabi-objcopy -O binary BlindJump BlindJump.gba
    COMMAND ${DEVKITPRO}/tools/bin/gbafix -tBlindJump -cCBJE -r0 -mEB BlindJump.gba)

  target_compile_options(BlindJump PRIVATE
    -D__GBA__)

elseif(APPLE)
  target_link_libraries(BlindJump
    "-framework sfml-window -framework sfml-graphics -framework sfml-system -framework sfml-audio -framework sfml-network -framework Cocoa")
  set_target_properties(BlindJump
    PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")

  include_directories("/usr/local/include/")

  add_custom_target(pkg ALL mkdir -p BlindJump.app/Contents/Resources
    COMMAND mkdir -p BlindJump.app/Contents/Frameworks
    COMMAND cp -r ${ROOT_DIR}/images/ BlindJump.app/Contents/images/
    COMMAND cp -r ${ROOT_DIR}/shaders/ BlindJump.app/Contents/shaders/
    COMMAND cp -r ${ROOT_DIR}/sounds/ BlindJump.app/Contents/sounds/
    COMMAND cp -r ${ROOT_DIR}/scripts/ BlindJump.app/Contents/scripts/
    # COMMAND cp macOS/icon.icns BlindJump.app/Contents/Resources
    # COMMAND cp -r ${SFML_DIR}/lib/* BlindJump.app/Contents/Frameworks
    # COMMAND cp -r ${SFML_DIR}/extlibs/libs-osx/Frameworks/* BlindJump.app/Contents/Frameworks
    )

elseif(UNIX)

  target_link_libraries(BlindJump
    -lpthread
    -lsfml-window
    -lsfml-graphics
    -lsfml-system
    -lsfml-audio
    -lsfml-network)

endif()



target_compile_options(BlindJump PRIVATE
  ${SHARED_COMPILE_OPTIONS})



file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE HEADERS "${SOURCE_DIR}/*.hpp")

add_custom_target(clang-format
  COMMAND clang-format --style=file -i ${SOURCES} ${HEADERS}
  COMMENT "Running clang-format"
  VERBATIM)
