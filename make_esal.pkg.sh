#!/bin/bash
mkdir -p esal_package
cp /usr/lib/libsai.so* esal_package
cp obj/libesal.so esal_package
cp esalApp esal_package
cp -r iniFiles/ esal_package
cp -r py/ esal_package
tar -czvf esal_package_`date +"%Y%m%d"`.tar esal_package/

