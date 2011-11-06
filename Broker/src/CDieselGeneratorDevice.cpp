///////////////////////////////////////////////////////////////////////////////
/// @file CDieselGeneratorDevice.cpp
///
/// @author Yaxi Liu <ylztf@mst.edu>
///
/// @compiler C++
///
/// @project FREEDM DGI
///
/// @description Manages the solar panels
///
/// @license
/// These source code files were created at as part of the
/// FREEDM DGI Subthrust, and are
/// intended for use in teaching or research. They may be
/// freely copied, modified and redistributed as long
/// as modified versions are clearly marked as such and
/// this notice is not removed.
///
/// Neither the authors nor the FREEDM Project nor the
/// National Science Foundation
/// make any warranty, express or implied, nor assumes
/// any legal responsibility for the accuracy,
/// completeness or usefulness of these codes or any
/// information distributed with these codes.
///
/// Suggested modifications or questions about these codes
/// can be directed to Dr. Bruce McMillin, Department of
/// Computer Science, Missour University of Science and
/// Technology, Rolla, MO 65409 (ff@mst.edu).
///////////////////////////////////////////////////////////////////////////////

#include "CDieselGeneratorDevice.hpp"

namespace freedm {
	namespace broker {

		///////////////////////////////////////////////////////////////////////////////
		/// @fn CDIESELGENERATORDEVICE
		/// @brief  construcctor for the diesel generator.
        ///         Its type is always DG.
		/// @param phymanager The related physical device manager.
		/// @param deviceid The identifier for this generic device.
		/// @param lineClient  the client that connects to the PSCAD interface
		///////////////////////////////////////////////////////////////////////////////
		CDieselGeneratorDevice::CDieselGeneratorDevice(CLineClient::TPointer lineClient, CPhysicalDeviceManager& phymanager, IPhysicalDevice::Identifier deviceid)  
			: CPSCADDevice(lineClient, phymanager, deviceid, physicaldevices::DG)
		{};

		/////////////////////////////////////////////////////////////////////////////
		/// @fn GET_POWERLEVEL
		/// @brief get the generated power level of the diesel generator
		/// @return  the generated power level read from PSCAD simulation
		///////////////////////////////////////////////////////////////////////////////
		CDieselGeneratorDevice:: SettingValue CDieselGeneratorDevice::get_powerLevel()
		{
			return CPSCADDevice::Get("powerLevel");
		}

		/////////////////////////////////////////////////////////////////////////////
		/// @fn TURNON
		/// @brief turn diesel generator on
		///////////////////////////////////////////////////////////////////////////////
		void CDieselGeneratorDevice::turnOn()
		{
			CPSCADDevice::Set("onOffSwitch", 1);
		}

		/////////////////////////////////////////////////////////////////////////////
		/// @fn TURNOFF
		/// @brief turn diesel generator off
		///////////////////////////////////////////////////////////////////////////////
		void CDieselGeneratorDevice::turnOff()
		{
			CPSCADDevice::Set("onOffSwitch", 0);
		}

	} // namespace broker
} // namespace freedm



