///////////////////////////////////////////////////////////////////////////////
/// @file CGridLinkDevice.hpp
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
#ifndef CGRIDLINKDEVICE_HPP
#define CGRIDLINKDEVICE_HPP

#include "CLineClient.hpp"
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "CPhysicalDeviceManager.hpp"
#include "PhysicalDeviceTypes.hpp"
#include "IPhysicalDevice.hpp"
#include "CPSCADDevice.hpp"

namespace freedm {
	namespace broker {
        
        //gridlink is the AC power line connecting a PMCU to the main grid's AC line 
		class CGridLinkDevice : public CPSCADDevice { 
		public:
			//constructor
			CGridLinkDevice(CLineClient::TPointer lineClient, CPhysicalDeviceManager& phymanager, IPhysicalDevice::Identifier deviceid);
      
			typedef boost::shared_ptr<CGridLinkDevice> GridLinkDevicePtr;

			//get the power level of the AC power line
			SettingValue get_powerLevel();

			//turn the link on or off
			void turnOn();
			void turnOff();
		};

	}//namespeace broker
}//namespace freedm
#endif
