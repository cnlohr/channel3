include user.cfg
-include esp82xx/common.mf
-include esp82xx/main.mf

% :
	$(warning This is the empty rule. Something went wrong.)
	@true

ifndef TARGET
$(info Modules were not checked out... use git clone --recursive in the future. Pulling now.)
$(shell git submodule update --init --recursive)
endif

SRCS+= \
	user/video_broadcast.c \
	user/3d.c \
	tablemaker/broadcast_tables.c \
	tablemaker/CbTable.c

# Example for a custom rule.
# Most of the build is handled in main.mf
.PHONY : showvars
showvars:
	$(foreach v, $(.VARIABLES), $(info $(v) = $($(v))))
	true

