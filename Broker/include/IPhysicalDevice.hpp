///////////////////////////////////////////////////////////////////////////////
/// @file      IPhysicalDevice.hpp
///
/// @author    Stephen Jackson <scj7t4@mst.edu>
///
/// @compiler  C++
///
/// @project   FREEDM DGI
///
/// @description The abstract base for physical devices.
///
/// @license
/// These source code files were created at as part of the
/// FREEDM DGI Subthrust, and are
/// intended for use in teaching or research.  They may be
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
/// Technology, Rolla, /// MO  65409 (ff@mst.edu).
///////////////////////////////////////////////////////////////////////////////

#ifndef IPHYSICALDEVICE_HPP
#define IPHYSICALDEVICE_HPP

#include "PhysicalDeviceTypes.hpp"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread.hpp>

namespace freedm {
namespace broker {

class CPhysicalDeviceManager;

/// Abstract class that provides a base for Physical Devices.
class IPhysicalDevice
    : public boost::enable_shared_from_this<IPhysicalDevice>,
      private boost::noncopyable
{
    public:
        /// The type used for the settings key
        typedef std::string SettingKey;
        
        /// The type used for the value of the setting
        typedef double SettingValue;
    
        /// The type used for the device identifier.
        typedef std::string Identifier;

        /// Cleaner Access to the types enumeration
        typedef physicaldevices::DeviceType DeviceType;
    
        /// A short type name to use for a shared ptr to the device.
        typedef boost::shared_ptr<IPhysicalDevice> DevicePtr;
        
        /// Constructor which takes in the manager and device id.
        IPhysicalDevice(CPhysicalDeviceManager& phymanager, Identifier deviceid,
                        DeviceType devtype)
            : m_manager(phymanager),
              m_devid(deviceid),
              m_devtype(devtype) {};

        /// Pulls the setting of some key from the inside.
        virtual SettingValue Get(SettingKey key) = 0;

        /// Sets the value of some key to the input value.
        virtual void Set(SettingKey key, SettingValue value) = 0;

        //turn the device on
        virtual void turnOn() = 0;

        //turn the device off
        virtual void turnOff() = 0;

        //get the power level of the device
        virtual SettingValue get_powerLevel() = 0;
        
        /// Locks the provided mutex.
        virtual void Lock() { m_mutex.lock(); };
    
        /// Tries the mutex and returns true if it can (and has been) taken
        virtual bool TryLock() { return m_mutex.try_lock(); }
    
        /// Releases the lock on the device.
        virtual void Unlock() { m_mutex.unlock(); };
    
        /// Gets the device identifier.
        Identifier GetID() { return m_devid; };

        /// Gets the device type.
        DeviceType GetType() { return m_devtype; };
        
        /// Gets the manager associated with this device.
        CPhysicalDeviceManager& GetManager() { return m_manager; }; 
    protected:
        /// The manager who is tracking this device.
        CPhysicalDeviceManager& m_manager;
        
        /// The mutex to protect this from some actions.
        mutable boost::mutex m_mutex;

        /// The unique identifier for this device.
        Identifier m_devid;
    
        /// The type of device
        physicaldevices::DeviceType m_devtype;
};

    } // Namespace broker
} // namespace freedm

#endif
