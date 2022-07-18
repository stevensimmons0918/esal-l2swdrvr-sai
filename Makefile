INSTALL_LIBDIR = $(INSTALL_DIR)/usr/lib
CROSS_COMPILE ?=
ECHO = echo
CC = g++
LD ?= $(CROSS_COMPILE)gcc
AR ?= $(CROSS_COMPILE)ar
BASE_DIR = .
CFLAGS = 
CFLAGS += -fPIC -Wall -Werror -Wno-unused -std=c++11 -g -Og -DDEBUG
LDFLAGS =
OBJ_PATH = $(PWD)

SAI_H_DIR := $(BASE_DIR)/sai_vendor_api/
ESAL_H_DIR := $(BASE_DIR)/esalsai/headers/ 
ESAL_SRC_DIR := $(BASE_DIR)/esalsai/

CFLAGS += \
    -I. \
    -I$(SAI_H_DIR) \
    -I$(ESAL_H_DIR) \
    -I../sai_cpss/plugins/sai/SAI/xpSai/


SHARED_OBJECTS = $(BASE_DIR)/libsai.so \
                      $(BASE_DIR)/libXdkCpss.so

#List of files to include in list
FILES := \
  esalSaiAcl.cc \
  esalSaiBridge.cc \
  esalSaiFdb.cc \
  esalSaiHost.cc \
  esalSaiMc.cc \
  esalSaiPort.cc \
  esalSaiStatus.cc \
  esalSaiStp.cc \
  esalSaiSwitch.cc \
  esalSaiTag.cc \
  esalSaiVlan.cc

MKDIR_P = mkdir -p
define compile
@# Create ouput folder
  $(MKDIR_P) $(dir $@)
@#create dependency file
  $(CC) -MM -c $1 $< -o $2/$*.d
@#Duplicate it
  cp $2/$*.d $2/$*.tmp
@# Create empty rule per file in dependency rule to avoid "No rule to make target ..." Errors 
  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' -e '/^$$/d' -e 's/$$/ :/' < $2/$*.d >> $2/$*.tmp
@# Change object filename to include full path
  sed -e 's/$(subst .,\.,$(notdir $@))/$(subst /,\/,$@)/'  < $2/$*.tmp > $2/$*.d
@# Remove temporary file
  rm $2/$*.tmp
@# compile
  $(CC) -c $1 $< -o $@
endef

ESAL_OBJECTS := $(patsubst %.cc,%.o,$(FILES))
ESAL_DEP = $(patsubst %.o,%.d,$(ESAL_OBJECTS))

%.o:%.cc
	$(call compile,$(CFLAGS),$(OBJ_PATH))
#-L. -lsai -lXdkCpss 
esal_lib: $(ESAL_OBJECTS)
	echo ESAL objects: $(ESAL_OBJECTS)
	$(CC) -shared -o libesal.so $(LDFLAGS) $(ESAL_OBJECTS) -ldl -lpthread -lrt -ldl -lstdc++ -lm -L. -lsai

esal_app: esal_lib
	$(CC) -o esal_app $(CFLAGS) $(LDFLAGS) esalMain.cc -L. -lesal

-include $(ESAL_DEP)
