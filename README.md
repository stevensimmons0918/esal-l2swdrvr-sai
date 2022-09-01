# esal-l2swdrvr-sai




## Prerequisites
libsai.so in /usr/lib

After compilation of sai-marvell-api you need manually copy libsai.so to /usr/lib folder:
```
sudo cp sai-marvell-api/dist/libsai.so in /usr/lib
```

Make sure that you have been added a config file to /usr/local/fnc/esal/sai.profile.ini.

Example:
```
mode=1
hwId=ALDRIN2XLFL

switchMacAddress=de:95:7b:67:b5:db
l3_counter_index_base=0 4K
pbrMaxEntries=0
SAI_NUM_ECMP_NUMBERS=0

lpm_fdb_em_profile=MID_L3_MID_L2_NO_EM
portListWithCableLen=000:1 001:1 002:1 003:1 004:1 005:1 006:1 007:1 008:1 009:1 010:1 011:1 012:1 013:1 014:1 015:1
```
> Note: hwId is a mandatory parameter.

## Getting started

```
git submodule init
git submodule update --recursive
make
./esalApp
```
