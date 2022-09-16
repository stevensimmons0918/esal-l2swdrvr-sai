INSTALL_LIBDIR = $(INSTALL_DIR)/usr/lib
CROSS_COMPILE ?=
ECHO = echo
CC = g++
LD ?= $(CROSS_COMPILE)gcc
AR ?= $(CROSS_COMPILE)ar
BASE_DIR = .
CFLAGS =
CFLAGS += -fPIC -Wall -Werror -Wno-unused -std=c++11 -g -Og -DDEBUG -DLARCH_ENVIRON -DSFP_RDY -DHAVE_MRVL
LDFLAGS =
OBJ_PATH = $(PWD)/obj
OUT_DIR = $(OBJ_PATH)
BIN_DIR = $(PWD)/bin

SAI_H_DIR := $(BASE_DIR)/sai/
ESAL_H_DIR := $(BASE_DIR)/headers/
ESAL_SRC_DIR := $(BASE_DIR)/

CFLAGS += \
    -I. \
    -I$(SAI_H_DIR) \
    -I$(ESAL_H_DIR) \
    -I../sai_cpss/plugins/sai/SAI/xpSai/ \
    -I../sai-marvell-api/xps/include \
    -Ipy/


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
  $(MKDIR_P) $(OBJ_PATH)
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

ESAL_OBJECTS := $(patsubst %.cc,$(OUT_DIR)/%.o,$(FILES))
ESAL_DEP = $(patsubst %.o,%.d,$(ESAL_OBJECTS))

$(OUT_DIR)/%.o:%.cc
	$(call compile,$(CFLAGS),$(OUT_DIR))
#-L. -lsai -lXdkCpssgit 

esal_app: esal_lib
	$(MKDIR_P) $(BIN_DIR)
	$(CC) -o esalApp $(CFLAGS) $(LDFLAGS) esalMain.cc -lesal -I/usr/include/python2.7 -lpython2.7

esal_lib: $(ESAL_OBJECTS)
	echo ESAL objects: $(ESAL_OBJECTS)
	$(CC) -shared -o $(OUT_DIR)/libesal.so $(LDFLAGS) $(ESAL_OBJECTS) -ldl -lpthread -lrt -lstdc++ -lm -lsai -ldl
	sudo cp $(OUT_DIR)/libesal.so /usr/lib

clean:
	rm $(OBJ_PATH)/*


-include $(ESAL_DEP)
