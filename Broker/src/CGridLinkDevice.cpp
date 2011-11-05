///////////////////////////////////////////////////////////////////////////////
/// @file CGridLinkDevice.cpp
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

#include "CGridLinkDevice.hpp"

namespace freedm {
	namespace broker {

		///////////////////////////////////////////////////////////////////////////////
		/// @fn CGridLinkDevice
		/// @brief  construcctor for the AC power line connecting a PMCU to the main grid.
        ///         Its type is always GRID.
		/// @param phymanager The related physical device manager.
		/// @param deviceid The identifier for this generic device.
		/// @param lineClient  the client that connects to the PSCAD interface
		///////////////////////////////////////////////////////////////////////////////
		CGridLinkDevice::CGridLinkDevice(CLineClient::TPointer lineClient, CPhysicalDeviceManager& phymanager, IPhysicalDevice::Identifier deviceid)  
			: CPSCADDevice(lineClient, phymanager, deviceid, physicaldevices::GRID)
		{};

		/////////////////////////////////////////////////////////////////////////////
		/// @fn GET_POWERLEVEL
		/// @brief get the generated power level of the power line 
		/// @return  the generated power level read from PSCAD simulation
		///////////////////////////////////////////////////////////////////////////////
		CGridLinkDevice:: SettingValue CGridLinkDevice::get_powerLevel()
		{
			CPSCADDevice::Get("powerLevel");
		}

		/////////////////////////////////////////////////////////////////////////////
		/// @fn turnOn
		/// @brief turn the breaker on the powerline off, so power flows
		///////////////////////////////////////////////////////////////////////////////
		void CGridLinkDevice::turnOn()
		{
			CPSCADDevice::Set("onOffSwitch", 0);
		}

		/////////////////////////////////////////////////////////////////////////////
		/// @fn turnOff
		/// @brief turn the breaker on the power line on, so power stops flowing
		///////////////////////////////////////////////////////////////////////////////
		void CGridLinkDevice::turnOff()
		{
			CPSCADDevice::Set("onOffSwitch", 1);
		}

	} // namespace broker
} // namespace freedm



