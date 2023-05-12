/*
 *  *  esalX86Evaldip.cc
 *   *  copyright (C) 2021 Fujitsu Network Communications, Inc.
 *    *  Created on: Auug 21, 2021
 *     */
#include "headers/esalSaiDip.h"
#include "Cgos.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <unistd.h>
#include <string.h>

#ifndef LARCH_ENVIRON
/****************************************************************************************
 *  *  DIP command to perform Cgos I2C Read
 *   *  Usage: cgos_i2c_read <reg_addr> <size>
 *    *  *************************************************************************************/
extern bool esalHealthMonEnable;
void
EsalSaiDipEsalHealthMon::dip_handle_cmd (const std::string & path,
				       const std::vector < std::string >
				       &args) {
    if (args.size() < 2) {
        cmd_->dip_reply("Invalid arguments esalHealthMon enable|disable");
    } else {
        if (args[1] == "enable") {
            esalHealthMonEnable = true;
            cmd_->dip_reply("Enabled Health Mon");
        } else if (args[1] == "disable") {
            esalHealthMonEnable = false;
            cmd_->dip_reply("Disabled Health Mon");
        } else {
            cmd_->dip_reply("Invalid arguments esalHealthMon enable|disable");
        }
    }
    cmd_->dip_reply (DIP_CMD_HANDLED);
}

#endif
