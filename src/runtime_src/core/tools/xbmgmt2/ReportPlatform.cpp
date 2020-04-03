/**
 * Copyright (C) 2020 Xilinx, Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. A copy of the
 * License is located at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

// ------ I N C L U D E   F I L E S -------------------------------------------
// Local - Include Files
#include "ReportPlatform.h"
#include "flash/flasher.h"

// 3rd Party Library - Include Files
#include <boost/format.hpp>


void 
ReportPlatform::getPropertyTreeInternal( const xrt_core::device * _pDevice,
                                         boost::property_tree::ptree &_pt) const
{
  // Defer to the 20201 format.  If we ever need to update JSON data, 
  // Then update this method to do so.
  getPropertyTree20201(_pDevice, _pt);
}

/*
 * helper function for getPropertyTree20201()
 */
static bool 
same_config(const std::string& vbnv, const std::string& sc,
             const std::string& id, DSAInfo& installed) 
{
  if (!vbnv.empty()) {
    bool same_dsa = ((installed.name == vbnv) &&
      (installed.matchId(id)));
    bool same_bmc = ((sc.empty()) ||
      (installed.bmcVer == sc));
    return same_dsa && same_bmc;
  }
  return false;
}

void 
ReportPlatform::getPropertyTree20201( const xrt_core::device * _pDevice,
                                      boost::property_tree::ptree &_pt) const
{
  boost::property_tree::ptree pt;
  pt.put("Description","Platform Information");

  // There can only be 1 root node
  _pt.add_child("platform", pt);

  boost::property_tree::ptree on_board_rom_info;
  boost::property_tree::ptree on_board_platform_info;
  boost::property_tree::ptree on_board_xmc_info;
  boost::property_tree::ptree on_board_dev_info;
  _pDevice->get_rom_info(on_board_rom_info);
  _pDevice->get_platform_info(on_board_platform_info);
  _pDevice->get_xmc_info(on_board_xmc_info);
  _pDevice->get_info(on_board_dev_info);

  //create information tree for a device
  _pt.put("platform.bdf", on_board_dev_info.get<std::string>("bdf"));
  _pt.put("platform.flash_type", on_board_platform_info.get<std::string>("flash_type", "N/A"));
  //Flashable partition running on FPGA
  _pt.put("platform.shell_on_fpga.vbnv", on_board_rom_info.get<std::string>("vbnv", "N/A"));
  _pt.put("platform.shell_on_fpga.sc_version", on_board_xmc_info.get<std::string>("sc_version", "N/A"));
  _pt.put("platform.shell_on_fpga.id", on_board_rom_info.get<std::string>("id", "N/A"));

  Flasher f(_pDevice->get_device_id());
  std::vector<DSAInfo> installedDSA = f.getInstalledDSA();

  BoardInfo info;
  f.getBoardInfo(info);

  //Flashable partitions installed in system
  _pt.put("platform.installed_shell.vbnv", installedDSA.front().name);
  _pt.put("platform.installed_shell.sc_version", installedDSA.front().bmcVer);
  _pt.put("platform.installed_shell.id", (boost::format("0x%x") % installedDSA.front().timestamp));
  _pt.put("platform.shell_upto_date", true);

  //check if the platforms on the machine and card match
  if(!same_config( on_board_rom_info.get<std::string>("vbnv"), on_board_xmc_info.get<std::string>("sc_version"),
      on_board_rom_info.get<std::string>("id"), installedDSA.front())) {
    _pt.put("platform.shell_upto_date", false);
  }
}

static const std::string
shell_status(bool status)
{
  if(!status)
    return boost::str(boost::format("%-8s : %s\n") % "WARNING" % "Device is not up-to-date.");
  return "";
}

void 
ReportPlatform::writeReport( const xrt_core::device * _pDevice, 
                             const std::vector<std::string> & /*_elementsFilter*/, 
                             std::iostream & _output) const
{
  boost::property_tree::ptree _pt;
  getPropertyTree20201(_pDevice, _pt);

  _output << boost::format("%s : %d\n") % "Device BDF" % _pt.get<std::string>("platform.bdf");
  _output << boost::format("  %-20s : %s\n") % "Flash type" % _pt.get<std::string>("platform.flash_type", "N/A");

  _output << "Flashable partition running on FPGA\n";
  _output << boost::format("  %-20s : %s\n") % "Platform" % _pt.get<std::string>("platform.shell_on_fpga.vbnv", "N/A");
  _output << boost::format("  %-20s : %s\n") % "SC Version" % _pt.get<std::string>("platform.shell_on_fpga.sc_version", "N/A");
  _output << boost::format("  %-20s : 0x%x\n") % "Platform ID" % _pt.get<std::string>("platform.shell_on_fpga.id", "N/A");
  
  _output << "\nFlashable partitions installed in system\n";
  _output << boost::format("  %-20s : %s\n") % "Platform" % _pt.get<std::string>("platform.installed_shell.vbnv", "N/A");
  _output << boost::format("  %-20s : %s\n") % "SC Version" % _pt.get<std::string>("platform.installed_shell.sc_version", "N/A");
  _output << boost::format("  %-20s : 0x%x\n") % "Platform ID" % _pt.get<std::string>("platform.installed_shell.id", "N/A");
  _output << shell_status(_pt.get<bool>("platform.shell_upto_date"));
  _output << "----------------------------------------------------\n";

}

