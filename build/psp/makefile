TARGET = BlindJump
CURRENT_DIR = $(shell pwd)
SRC = $(CURRENT_DIR)/../../source
OBJS =  $(SRC)/platform/psp/psp_platform.o \
	$(SRC)/platform/psp/glib2d.o \
	$(SRC)/blind_jump/entity/effects/wandererSmallLaser.o \
	$(SRC)/blind_jump/entity/effects/wandererBigLaser.o \
	$(SRC)/blind_jump/entity/effects/conglomerateShot.o \
	$(SRC)/blind_jump/entity/details/cutsceneCloud.o \
	$(SRC)/blind_jump/entity/effects/dynamicEffect.o \
	$(SRC)/blind_jump/entity/effects/staticEffect.o \
	$(SRC)/blind_jump/entity/details/cutsceneBird.o \
	$(SRC)/blind_jump/entity/effects/dialogBubble.o \
	$(SRC)/blind_jump/entity/bosses/infestedCore.o \
	$(SRC)/blind_jump/entity/bosses/gatekeeper.o \
	$(SRC)/blind_jump/entity/details/itemChest.o \
	$(SRC)/blind_jump/entity/details/scavenger.o \
	$(SRC)/blind_jump/entity/effects/explosion.o \
	$(SRC)/blind_jump/entity/enemies/scarecrow.o \
	$(SRC)/blind_jump/entity/enemies/compactor.o \
	$(SRC)/blind_jump/entity/details/signpost.o \
	$(SRC)/blind_jump/entity/effects/particle.o \
	$(SRC)/blind_jump/entity/enemies/sinkhole.o \
	$(SRC)/blind_jump/entity/effects/uiNumber.o \
	$(SRC)/blind_jump/entity/bosses/wanderer.o \
	$(SRC)/blind_jump/entity/bosses/theTwins.o \
	$(SRC)/blind_jump/entity/effects/orbshot.o \
	$(SRC)/blind_jump/entity/details/lander.o \
	$(SRC)/blind_jump/entity/enemies/common.o \
	$(SRC)/blind_jump/entity/enemies/turret.o \
	$(SRC)/blind_jump/entity/enemies/dasher.o \
	$(SRC)/blind_jump/entity/details/rubble.o \
	$(SRC)/blind_jump/entity/details/debris.o \
	$(SRC)/blind_jump/entity/enemies/drone.o \
	$(SRC)/blind_jump/entity/enemies/enemy.o \
	$(SRC)/blind_jump/entity/enemies/golem.o \
	$(SRC)/blind_jump/entity/enemies/theif.o \
	$(SRC)/blind_jump/entity/enemies/snake.o \
	$(SRC)/blind_jump/entity/effects/laser.o \
	$(SRC)/blind_jump/entity/details/item.o \
	$(SRC)/blind_jump/entity/bosses/boss.o \
	$(SRC)/blind_jump/entity/peerPlayer.o \
	$(SRC)/blind_jump/state/state_impl.o \
	$(SRC)/blind_jump/game_script_defs.o \
	$(SRC)/graphics/overlay.o \
	$(SRC)/graphics/sprite.o \
	$(SRC)/blind_jump/persistentData.o \
	$(SRC)/number/numeric.o \
	$(SRC)/number/random.o \
	$(SRC)/graphics/view.o \
	$(SRC)/blind_jump/entity/entity.o \
	$(SRC)/blind_jump/network_event.o \
	$(SRC)/blind_jump/entity/player.o \
	$(SRC)/localization.o \
	$(SRC)/blind_jump/inventory.o \
	$(SRC)/collision.o \
	$(SRC)/blind_jump/powerup.o \
	$(SRC)/tileMap.o \
	$(SRC)/globals.o \
	$(SRC)/camera.o \
	$(SRC)/string.o \
	$(SRC)/start.o \
	$(SRC)/path.o \
	$(SRC)/blind_jump/game.o \
	$(SRC)/script/lisp.o \
	$(SRC)/script/vm.o \
	$(SRC)/script/compiler.o \
	$(SRC)/blind_jump/state/multiplayerReviveWaitingState.o \
	$(SRC)/blind_jump/state/healthAndSafetyWarningState.o \
	$(SRC)/blind_jump/state/quickSelectInventoryState.o \
	$(SRC)/blind_jump/state/networkConnectSetupState.o \
	$(SRC)/blind_jump/state/networkConnectWaitState.o \
	$(SRC)/blind_jump/state/multiplayerReviveState.o \
	$(SRC)/blind_jump/state/bossDeathSequenceState.o \
	$(SRC)/blind_jump/state/launchCutsceneState.o \
	$(SRC)/blind_jump/state/endingCutsceneState.o \
	$(SRC)/blind_jump/state/deathContinueState.o \
	$(SRC)/blind_jump/state/endingCreditsState.o \
	$(SRC)/blind_jump/state/logfileViewerState.o \
	$(SRC)/blind_jump/state/preFadePauseState.o \
	$(SRC)/blind_jump/state/introCreditsState.o \
	$(SRC)/blind_jump/state/editSettingsState.o \
	$(SRC)/blind_jump/state/newLevelIdleState.o \
	$(SRC)/blind_jump/state/signalJammerState.o \
	$(SRC)/blind_jump/state/respawnWaitState.o \
	$(SRC)/blind_jump/state/scoreScreenState.o \
	$(SRC)/blind_jump/state/commandCodeState.o \
	$(SRC)/blind_jump/state/pauseScreenState.o \
	$(SRC)/blind_jump/state/titleScreenState.o \
	$(SRC)/blind_jump/state/remoteReplState.o \
	$(SRC)/blind_jump/state/quickChatState.o \
	$(SRC)/blind_jump/state/imageViewState.o \
	$(SRC)/blind_jump/state/inventoryState.o \
	$(SRC)/blind_jump/state/overworldState.o \
	$(SRC)/blind_jump/state/mapSystemState.o \
	$(SRC)/blind_jump/state/deathFadeState.o \
	$(SRC)/blind_jump/state/lispReplState.o \
	$(SRC)/blind_jump/state/newLevelState.o \
	$(SRC)/blind_jump/state/quickMapState.o \
	$(SRC)/blind_jump/state/notebookState.o \
	$(SRC)/blind_jump/state/glowFadeState.o \
	$(SRC)/blind_jump/state/itemShopState.o \
	$(SRC)/blind_jump/state/fadeOutState.o \
	$(SRC)/blind_jump/state/goodbyeState.o \
	$(SRC)/blind_jump/state/dialogState.o \
	$(SRC)/blind_jump/state/activeState.o \
	$(SRC)/blind_jump/state/fadeInState.o \
	$(SRC)/blind_jump/state/warpInState.o \
	$(SRC)/data/file_init.o \
	$(SRC)/data/file_pre_levelgen.o	\
	$(SRC)/data/file_post_levelgen.o \
	$(SRC)/data/file_waypoint_clear.o \
	$(SRC)/data/file_english.o \
	$(SRC)/data/file_spanish.o \
	$(shell cat ./objnames.img.txt)


CFLAGS = -Os -G0 -g3 -mpreferred-stack-boundary=4 -ffast-math -gdwarf-2
CXXFLAGS = $(CFLAGS) -std=c++17 -fno-exceptions -I$(SRC) -D__PSP__ -D__BLINDJUMP_ENABLE_LOGS
ASFLAGS = $(CFLAGS)

LIBS = -lstdc++ -lpspgum -lpspgu -lpng -ljpeg -lz -lm  -lpspvram -lpsprtc -lpspusb -lpsputility -lpspusbcam -lpspjpeg

GAME_IMAGE_DATA = $(SRC)/../images/*.png

# PSP Stuff
BUILD_PRX = 1
PSP_FW_VERSION = 500
PSP_LARGE_MEMORY = 1

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = BlindJump
PSP_EBOOT_ICON = icon.png
#PSP_EBOOT_PIC1 = cover.png

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak



images:
	echo "building images"


$(SRC)/../images/%.png: images
	echo $@
