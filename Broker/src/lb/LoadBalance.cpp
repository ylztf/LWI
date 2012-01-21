//////////////////////////////////////////////////////////
/// @file         LoadBalance.cpp
///
/// @author       Ravi Akella <rcaq5c@mst.edu>
///               Yaxi Liu <ylztf@mst.edu>
///
/// @compiler     C++
///
/// @project      FREEDM DGI
///
/// @description  Main file that includes load balancing (drafting) algorithm
///
/// @functions 	LoadManage()
///	        SendDraftRequest()
///		LoadTable()
///		handle_read()
///		get_peer()
///		add_peer()
///		lb()
///	
/// @license	
/// These source code files were created at as part of the
/// FREEDM DGI Subthrust, and are
/// intended for use in teaching or research.  They may be
/// freely copied, modified and redistributed as long
/// as modified versions are clearly marked as such and
/// this notice is not removed.

/// Neither the authors nor the FREEDM Project nor the
/// National Science Foundation
/// make any warranty, express or implied, nor assumes
/// any legal responsibility for the accuracy,
/// completeness or usefulness of these codes or any
/// information distributed with these codes.

/// Suggested modifications or questions about these codes
/// can be directed to Dr. Bruce McMillin, Department of
/// Computer Science, Missouri University of Science and
/// Technology, Rolla, 
/// MO  65409 (ff@mst.edu).
///
/////////////////////////////////////////////////////////

#include "LoadBalance.hpp"
#include "Utility.hpp"
#include "CMessage.hpp"
#include <algorithm>
#include <cassert>
#include <exception>
#include <sys/types.h>
#include <unistd.h>
#include <iomanip>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>
#define foreach     BOOST_FOREACH

#include <boost/property_tree/ptree.hpp>
using boost::property_tree::ptree;

#include "logger.hpp"
CREATE_EXTERN_STD_LOGS()

namespace freedm {

///////////////////////////////////////////////////////////////////////////////
/// lbAgent
/// @description: Constructor for the load balancing module.
/// @limitations: None
/// @pre: None
/// @post: Object initialized and ready to enter run state.
/// @param GetUUID(): This object's uuid.
/// @param ios: the io service this node will use to share memory
/// @param p_dispatch: The dispatcher used by this module
/// @param m_conManager: The connection manager to use in this class.
/// @param m_phyManager: The physical device manager to use in this class.
///////////////////////////////////////////////////////////////////////////////
  
lbAgent::lbAgent(std::string uuid_, boost::asio::io_service &ios, 
                 freedm::broker::CDispatcher &p_dispatch, 
                 freedm::broker::CConnectionManager &m_conManager, 
                 freedm::broker::CPhysicalDeviceManager &m_phyManager):
  LPeerNode(uuid_, m_conManager, ios, p_dispatch),
  m_phyDevManager(m_phyManager),
  m_GlobalTimer(ios)
{
  Logger::Debug << __PRETTY_FUNCTION__ << std::endl;
  PeerNodePtr self_(this);
  InsertInPeerSet(l_AllPeers, self_);
  step = 0;
  preLoad = LPeerNode::NORM;
}

///////////////////////////////////////////////////////////////////////////////
/// ~lbAgent
/// @description: Class desctructor
/// @pre: None
/// @post: The object is ready to be destroyed.
///////////////////////////////////////////////////////////////////////////////
lbAgent::~lbAgent()
{

}

////////////////////////////////////////////////////////////
/// LoadManage
/// @description: Manages the execution of the load balancing algorithm by 
///               broadcasting load changes (DEMAND-> Normal and Normal-> DEMAND)
///               computed by LoadTable() and initiating SendDraftRequest()
/// @pre: Node is not in Fail state
/// @post: Restarts on timeout
/// @return: Local load state is monitored on and specific load changes are 
///          advertised to peers on timeout
/// @limitations None
/////////////////////////////////////////////////////////
void lbAgent::LoadManage()
{
  Logger::Debug << __PRETTY_FUNCTION__ << std::endl;
  MessagePtr m_;
  preLoad = l_Status; // Remember previous load before computing current load

  // Physical device information managed by Broker can be obtained as below
  Logger::Info << "LB module identified "<< m_phyDevManager.DeviceCount()
               << " physical devices on this node" << std::endl;
  freedm::broker::IPhysicalDevice::DevicePtr DevPtr;
  freedm::broker::CPhysicalDeviceManager::PhysicalDeviceSet::iterator it_;
  for( it_ = m_phyDevManager.begin(); it_ != m_phyDevManager.end(); ++it_ )
  {
    DevPtr = it_->second;
    DevPtr = m_phyDevManager.GetDevice(it_->first); 
    Logger::Debug<< "Device ID: " << DevPtr->GetID() << ", Device Type: " 
    		 << DevPtr->GetType()<< ", power level: " << DevPtr->get_powerLevel() << std::endl;                         
  }  
  
  // Call LoadTable to update load state of the system as observed by this node
  LoadTable();
     
  // If state is DEMAND, broadcast
  if ( LPeerNode::DEMAND == l_Status )
  {
      // Create Demand message and send it to all nodes
    freedm::broker::CMessage m_;
    std::stringstream ss_;
    ss_.clear();
    ss_ << GetUUID();
    ss_ >> m_.m_srcUUID;
    m_.m_submessages.put("lb.source", ss_.str());
    ss_.clear();
    ss_.str("demand");
    m_.m_submessages.put("lb", ss_.str());
    
        
    //Send Demand message to all nodes
    Logger::Notice <<"Broadcasting Load change: NORM -> DEMAND " <<std::endl;
     foreach( PeerNodePtr peer_, l_AllPeers | boost::adaptors::map_values)
    {
      if( peer_->GetUUID() == GetUUID())      
	  continue;  
      else
      {    	 
	try
  	{
   	  peer_->Send(m_);
  	}
  	catch (boost::system::system_error& e)
  	{
    	  Logger::Info << "Couldn't Send Message To Peer" << std::endl;
   	}
      }
    }//end foreach
  }//endif

   //On load change from Normal, broadcast the change
  else if (LPeerNode::NORM != preLoad && LPeerNode::NORM == l_Status)
  {   
    freedm::broker::CMessage m_;  
    std::stringstream ss_;
    ss_ << GetUUID();
    ss_ >> m_.m_srcUUID;
    m_.m_submessages.put("lb.source", ss_.str());
    ss_.clear();
    ss_.str("normal");
    m_.m_submessages.put("lb", ss_.str());

    //Send Normal message to all nodes 
    Logger::Notice <<"Broadcasting Load change: DEMAND -> NORM " <<std::endl;   
    foreach( PeerNodePtr peer_, l_AllPeers | boost::adaptors::map_values)
    {
      if( peer_->GetUUID() == GetUUID())
         continue;
      else
      {    	 
	try
        {
          peer_->Send(m_);
        }
       
        catch (boost::system::system_error& e)
        {
          Logger::Info << "Couldn't Send Message To Peer" << std::endl;
        }
      }
     }
    }//end elseif

  // If your are in Supply state 
  else if (LPeerNode::SUPPLY == l_Status)
  {
    SendDraftRequest(); //initiate draft request
  }

  // If you are in Normal state
  else if (LPeerNode::NORM == l_Status)
  {
   // Do nothing (atleast for now )
   }

  //Start the timer; on timeout, this function is called again 
  m_GlobalTimer.expires_from_now( boost::posix_time::seconds(LOAD_TIMEOUT) );
  m_GlobalTimer.async_wait( boost::bind(&lbAgent::LoadManage, this,
  					boost::asio::placeholders::error));  
}//end LoadManage

////////////////////////////////////////////////////////////
/// LoadManage
/// @description: Overloaded function of LoadManage
/// @pre: Timer expired sending an error code
/// @post: Restarts the timer
/// @param err: Error associated with calling timer.
/// @return: Local load state is monitored on and specific load changes are 
///          advertised to peers on timeout
/////////////////////////////////////////////////////////
void lbAgent::LoadManage( const boost::system::error_code& err )
{
  Logger::Debug << __PRETTY_FUNCTION__ << std::endl;
  
  if(!err)
    {
      LoadManage();
    }
  else if(boost::asio::error::operation_aborted == err )
    {
      Logger::Info << "LoadManage(operation_aborted error) " <<
	__LINE__ << std::endl;
    }
  else
    {
      /* An error occurred or timer was canceled */
      Logger::Error << err << std::endl;
      throw boost::system::system_error(err);
    }
}

////////////////////////////////////////////////////////////
/// SendDraftRequest
/// @description Advertise willingness to share load whenever you can supply
/// @Citations: A Distributed Drafting ALgorithm for Load Balancing,
///             Lionel Ni, Chong Xu, Thomas Gendreau, IEEE Transactions on
///				Software Engineering, 1985
/// @pre: Current load state of this node is 'Supply'
/// @post: Change load to Normal after migrating load, on timer
/// @return Send "request" message to all peers
/// @limitations Currently broadcasts request to all the entries in the list of nodes
/////////////////////////////////////////////////////////

void lbAgent::SendDraftRequest(){

  Logger::Debug << __PRETTY_FUNCTION__ << std::endl;  
  if(LPeerNode::SUPPLY == l_Status)
    {
        //Create new request and send it to all nodes   
      	freedm::broker::CMessage m_;
      	std::stringstream ss_;
      	ss_.clear();
      	ss_ << GetUUID();
      	ss_ >> m_.m_srcUUID;
      	m_.m_submessages.put("lb.source", ss_.str());
      	ss_.clear();
      	ss_.str("request");
      	m_.m_submessages.put("lb", ss_.str());
        Logger::Notice << "\nSending DraftRequest from: " << 
        	m_.m_submessages.get<std::string>("lb.source") <<std::endl;
        foreach( PeerNodePtr peer_, l_AllPeers | boost::adaptors::map_values)
    	{
	  if( peer_->GetUUID() == GetUUID())
	     continue;
          else
          {    	 
            try
            {
               peer_->Send(m_);
            }
         catch (boost::system::system_error& e)
           {
            Logger::Info << "Couldn't Send Message To Peer" << std::endl;
           }
         }
        }//end foreach
     	     
    }//end if
}//end SendDraftRequest

////////////////////////////////////////////////////////////
/// InitiatePowerMigration 
/// @description Initiates 'power migration' on Draft Accept
///              message from a demand node    
/// @pre: Current load state of this node is 'Supply'
/// @post: Set command(s) to reduce DESD charge 
/// @limitations Changes significantly depending on SST's control capability; 
///		 for now, the supply node reduces the amount of power currently 
///		 in use to charge DESDs
/////////////////////////////////////////////////////////
void lbAgent::InitiatePowerMigration(float DemandValue){
    std::cout<<"************************************"<<std::endl;
    std::cout<<"*                                  *"<<std::endl;
    std::cout<<"*      Power Migrate now.          *"<<std::endl;
    std::cout<<"************************************"<<std::endl;
    m_phyDevManager.GetDevice("grid3")->turnOn();//set power to flow to main grid
}

////////////////////////////////////////////////////////////
/// LoadTable
/// @description:  prints load table
///  if netMigration == 0 (most likely grid connection is off)
///     if generation == load
///        set state to NORMAL
///     if generation < load
///        set state to DEMAND
///     if generation > load
///        set state to SUPPLY
///  if netMigration < 0 (is receiving power from peer)
///     if generation - gridPower >= load
///        set state to NORMAL
///     if generation - gridPower < load
///        set state to DEMAND
///  if netMigration >  0 (is donating power to peer)
///     if generation - gridPower > load
///        set state to SUPPLY
///     if generation - gridPower <= load
///        set state to NORMAL
/// @pre: All the peers should be connected
/// @post: Printed load values should be current and consistent
/// @return Load table and computes Load state
/// @limitations Only retrieves the loads of peers and prints
///              ,does not probe them
/////////////////////////////////////////////////////////

void lbAgent::LoadTable(){    
  std::cout <<"\n ----------- LOAD TABLE (Power Management) ------------" << std::endl;
  std::cout <<"| " << " @ " << microsec_clock::local_time()  <<std::endl;

  //temp variable to compute net generation from DRERs, storage from DESDs and LOADs
  double net_gen = 0;
  double net_storage = 0;
  double net_load = 0;
  
  //# devices of each type attached and alive 
  int DRER_count = 0;
  int DESD_count = 0;
  int LOAD_count = 0;

freedm::broker::IPhysicalDevice::DevicePtr DevPtr;
freedm::broker::CPhysicalDeviceManager::PhysicalDeviceSet::iterator it_;
for( it_ = m_phyDevManager.begin(); it_ != m_phyDevManager.end(); ++it_ )
    {
      //Compute Net Generation
      if ((m_phyDevManager.DeviceExists(it_->first)) && 
         (it_->second->GetType() == freedm::broker::physicaldevices::DRER))
	  {       	
	  net_gen += it_->second->get_powerLevel();   
	 DRER_count++;          
      }  
      //Compute Net Storage
      if ((m_phyDevManager.DeviceExists(it_->first)) && 
	 (it_->second->GetType() == freedm::broker::physicaldevices::DESD))
      {        
          net_storage += it_->second->get_powerLevel();     
          DESD_count++;      
      } 
      //Compute Net Load
      if ((m_phyDevManager.DeviceExists(it_->first)) && 
	 (it_->second->GetType() == freedm::broker::physicaldevices::LOAD))
      {       	
          net_load += it_->second->get_powerLevel();   
          LOAD_count++;        
      }  
    }


//unit set to kw
 P_Gen = (float)(net_gen * 1000);
 B_Soc = (float)(net_storage * 1000);
 P_Load = (float)(net_load * 1000);
 P_Gateway = (float)(P_Load - P_Gen);

 //print Load Table frame and column headers
  std::cout <<"| " << "Net DRER (" << DRER_count << "): " << P_Gen << std::setw(14) 
	    << "Net DESD (" << DESD_count << "): " << B_Soc << std::endl;
  std::cout <<"| " << "Net Load (" << LOAD_count << "): "<< P_Load << std::setw(14)
	    << "Gateway: " << P_Gateway << std::endl;
  std::cout <<"| ---------------------------------------------------- |" << std::endl;
  std::cout <<"| " << std::setw(20) << "UUID" << std::setw(27)<< "State" << std::setw(7) <<"|"<< std::endl;
  std::cout <<"| "<< std::setw(20) << "----" << std::setw(27)<< "-----" << std::setw(7) <<"|"<< std::endl;

  float netMigration = m_phyDevManager.GetDevice("grid3")->get_powerLevel();
  //round near zero numbers to 0.
  if (netMigration > 0)
    netMigration = floor(netMigration *1000)/1000;
  else
    netMigration = ceil(netMigration *1000)/1000;
  std::cout<<"NET migration IS "<<netMigration<<std::endl;
  
  if(P_Gen > P_Load)   {
    l_Status = LPeerNode::SUPPLY;
  }
  else if(P_Load > P_Gen)  {
    l_Status = LPeerNode::DEMAND; 
    DemandValue = P_Load - P_Gen;
  }
  else
    l_Status = LPeerNode::NORM;
  
  if (netMigration > 0){
    //you are donating power to others now
    if(P_Gen - netMigration > P_Load)   {
      l_Status = LPeerNode::SUPPLY;
    }
    else if(P_Load >= P_Gen - netMigration)  {
      l_Status = LPeerNode::NORM; 
    }
  }
  if ( netMigration < 0){
    //you are receiving power from others now
    if(P_Gen - netMigration >= P_Load)   {
      l_Status = LPeerNode::NORM;
    }
    else if(P_Load > P_Gen - netMigration)  {
      l_Status = LPeerNode::DEMAND; 
    }
  }
  
  //Update information about this node in the load table based on above computation
  //foreach( PeerNodePtr self_, l_AllPeers )
  //need to figure out how the new states needs to be reflected here.
  foreach( PeerNodePtr self_, l_AllPeers | boost::adaptors::map_values)
  {
    if( self_->GetUUID() == GetUUID())
    {
       EraseInPeerSet(m_LoNodes,self_);
       EraseInPeerSet(m_HiNodes,self_);
       EraseInPeerSet(m_NoNodes,self_);
       if (LPeerNode::SUPPLY == l_Status)
       {
	  InsertInPeerSet(m_LoNodes,self_);
       }
       else if (LPeerNode::NORM == l_Status)
       {
	  InsertInPeerSet(m_NoNodes,self_);
       }
       else if (LPeerNode::DEMAND == l_Status)
       {
	  InsertInPeerSet(m_HiNodes,self_);
       }
    }
  }
  
 //Print the load information you have about the rest of the system  
 foreach( PeerNodePtr p_, l_AllPeers | boost::adaptors::map_values)
    {
        if (CountInPeerSet(m_HiNodes,p_) > 0 ){
            std::cout<<"| " << p_->GetUUID() << std::setw(12)<< "Demand" <<std::setw(6) <<"|"<<std::endl;
            std::cout <<"| ---------------------------------------------------- |" <<std::endl;
        }
        else if (CountInPeerSet(m_NoNodes,p_) > 0 ) {
            std::cout<<"| " << p_->GetUUID() << std::setw(12)<< "Normal" <<std::setw(6) <<"|"<<std::endl;
            std::cout <<"| ---------------------------------------------------- |" <<std::endl;
        }
        else if (CountInPeerSet(m_LoNodes,p_) > 0 ) {
            std::cout<<"| " << p_->GetUUID() << std::setw(12)<< "Supply" <<std::setw(6) <<"|"<<std::endl;
            std::cout <<"| ---------------------------------------------------- |" <<std::endl;
        }
        else
            std::cout<<"| " << p_->GetUUID() << std::setw(12)<< "------" <<std::setw(6) <<"|"<<std::endl;
    }
  std::cout <<" ------------------------------------------------------" <<std::endl;
  return;
}//end LoadTable

////////////////////////////////////////////////////////////
/// HandleRead
/// @description: handles the incoming messages meant for lb module and performs 
///               action according to the Loadbalancing algorithm
/// @pre: The message obtained as ptree should be intended for this module
/// @post: The sender of the message always gets a response from this node
/// @param pt: message (ptree) reference obtained via broker
/// @return: multiple objectives depending on the message and power migration on 
///          successful negotiation
/// @limitations: Basis for setting P* is currently limited as follows:
///               Supplier sets his DRERs and DESDs in that order in a way to 
///               satisfy the net demand (P_Migrate); Assumption is that DRERs and DESDs 
///               have a special setting called "vin" and "vout" to direct their
///               output to the local system or to the grid respectively. 
///               In the case of Demanding node, the target gateway (P*) is set on the 
///               SST assuming that the SST within the simulation can allow the 
///               inflow to serve the demand load; This was done so since loads cannot
///               be controlled-so what would we set at the demand node to route 
///               into this node, the power being "migrated" by the supplying node.
///               Obviously, this would change in the near future
///
/////////////////////////////////////////////////////////
void lbAgent::HandleRead(const ptree& pt )
{
  Logger::Debug << __PRETTY_FUNCTION__ << std::endl;
  PeerSet tempSet_;
  MessagePtr m_;
  std::string line_;
  std::stringstream ss_;
  PeerNodePtr peer_;   
  line_ = pt.get<std::string>("lb.source");
  Logger::Debug << "Message '" <<pt.get<std::string>("lb")<<"' received from "<< line_<<std::endl;

   // Evaluate the identity of the message source
   if(line_ != GetUUID())
   {
      Logger::Debug << "Flag " <<std::endl;
      // Update the peer entry, if needed
      peer_ = get_peer(line_); 

       if( peer_ != NULL)
       {
         Logger::Debug << "Peer already exists. Do Nothing " <<std::endl;
       }
       else
       {
         // Add the peer, if an entry wasn`t found
	 Logger::Debug << "Peer doesn`t exist. Add it up to LBPeerSet" <<std::endl;
	 add_peer(line_);
         peer_ = get_peer(line_);
       }
   }//endif   
     
   // If you receive a peerList from your new leader, process it and 
   // identify your new group members
   if(pt.get<std::string>("lb") == "peerList")
   {
      std::string peers_, token;
      peers_ = pt.get<std::string>("lb.peers");
      Logger::Notice << "\nPeer List < " << peers_ <<
	           " > received from Group Leader: " << line_ <<std::endl;

      //Update the PeerNode lists accordingly         

      foreach( PeerNodePtr p_, l_AllPeers | boost::adaptors::map_values)
      {
        if( p_->GetUUID() == GetUUID()) continue;	   
	EraseInPeerSet(l_AllPeers,p_);
      }

      foreach( PeerNodePtr p_, m_LoNodes | boost::adaptors::map_values)
      {
        if( p_->GetUUID() == GetUUID()) continue;	   
	EraseInPeerSet(m_LoNodes,p_);
      }

      foreach( PeerNodePtr p_, m_HiNodes | boost::adaptors::map_values)
      {
        if( p_->GetUUID() == GetUUID()) continue;	   
	EraseInPeerSet(m_HiNodes,p_);
      }

      foreach( PeerNodePtr p_, m_NoNodes | boost::adaptors::map_values)
      {
        if( p_->GetUUID() == GetUUID()) continue;	   
	EraseInPeerSet(m_NoNodes,p_);
      }
      
      // Tokenize the peer list string 
      std::istringstream iss(peers_);
      while ( getline(iss, token, ',') )
      {
      	peer_ = get_peer(token); 
        if( false != peer_ )
	{
	  Logger::Debug << "LB knows this peer " <<std::endl;
	}
        else
	{
          Logger::Debug << "LB sees a new member "<< token  
                        << " in the group " <<std::endl;
          add_peer(token);
	}
      }//endwhile
  }//end if("peerlist")

  // You received a draft request    
  else if(pt.get<std::string>("lb") == "request"  && peer_->GetUUID() != GetUUID())
  {
    Logger::Notice << "\nRequest message received from: " << peer_->GetUUID() << std::endl;               
    // Just not to duplicate the peer, erase the existing entries of it               

    EraseInPeerSet(m_LoNodes,peer_);
    EraseInPeerSet(m_HiNodes,peer_);
    EraseInPeerSet(m_NoNodes,peer_);
    // Insert into set of Supply nodes

    InsertInPeerSet(m_LoNodes,peer_);

    // Create your response to the Draft request sent by the source
    freedm::broker::CMessage m_;
    std::stringstream ss_;
    ss_ << GetUUID();
    ss_ >> m_.m_srcUUID;
    m_.m_submessages.put("lb.source", ss_.str());   

    // If you are in Demand, accept the request with a 'yes' 
    if( LPeerNode::DEMAND == l_Status )
    {
      ss_.clear();
      ss_.str("yes");
      m_.m_submessages.put("lb", ss_.str());
      //std::cout<<"accepting draft"<<std::endl;
    }

    // Otherwise, inform the source that you are not interested
    // NOTE: This may change in future when we incorporate advanced economics 
    else
    {
      ss_.clear();
      ss_.str("no");
      m_.m_submessages.put("lb", ss_.str());
    }
 
    // Send your response         
    if( peer_->GetUUID() != GetUUID())
    {
      try
      {
   	peer_->Send(m_);
      }
      catch (boost::system::system_error& e)
      {
   	Logger::Info << "Couldn't Send Message To Peer" << std::endl;
      }
    }
  }//end if("request")

  // You received a Demand message from the source. Update list and do nothing. 
  else if(pt.get<std::string>("lb") == "demand"  && peer_->GetUUID() != GetUUID())
  {
    Logger::Notice << "\nDemand message received from: " 
                   << pt.get<std::string>("lb.source") <<std::endl;
    EraseInPeerSet(m_HiNodes,peer_);
    EraseInPeerSet(m_NoNodes,peer_);
    EraseInPeerSet(m_LoNodes,peer_);
    InsertInPeerSet(m_HiNodes,peer_);
   
  }//end if("demand")

  // You received a Load change of source to Normal state
  else if(pt.get<std::string>("lb") == "normal"  && peer_->GetUUID() != GetUUID())
  {
    Logger::Notice << "\nNormal message received from: " 
                   << pt.get<std::string>("lb.source") <<std::endl;
    EraseInPeerSet(m_NoNodes,peer_);
    EraseInPeerSet(m_HiNodes,peer_);
    EraseInPeerSet(m_LoNodes,peer_);
    InsertInPeerSet(m_NoNodes,peer_);
  }//end if("normal")

  // You received a response from source, to your draft request
  else if(((pt.get<std::string>("lb") == "yes") || 
         (pt.get<std::string>("lb") == "no")) && peer_->GetUUID() != GetUUID())
  {   
    // The response is a 'yes' 
    if(pt.get<std::string>("lb") == "yes")
    {
      Logger::Notice << "(Yes) from " << peer_->GetUUID() << std::endl;
      //Initiate drafting with a message accordingly
      freedm::broker::CMessage m_;
      std::stringstream ss_;   
      ss_ << GetUUID();
      ss_ >> m_.m_srcUUID;
      m_.m_submessages.put("lb.source", ss_.str());
      ss_.clear();
      ss_.str("drafting");
      m_.m_submessages.put("lb", ss_.str());
      //Its better to check the status again before initiating drafting
      if( (peer_->GetUUID() != GetUUID()) && (LPeerNode::SUPPLY == l_Status))
      {
        try
        {
          peer_->Send(m_);
        }
        catch (boost::system::system_error& e)
        {
          Logger::Info << "Couldn't send Message To Peer" << std::endl;
        }
       }
    }//endif
     
    // The response is a 'No'; do nothing  
    else
    {
      Logger::Notice << "(No) from " << peer_->GetUUID() << std::endl;
    }
  }//end if("yes/no from the demand node")

 //You received a Drafting message in reponse to your Demand
 //Ackowledge by sending an 'Accept' message
 else if(pt.get<std::string>("lb") == "drafting" && peer_->GetUUID() != GetUUID())
 {
   Logger::Notice << "\nDrafting message received from: " << peer_->GetUUID() << std::endl;   
   if( LPeerNode::DEMAND == l_Status )
   {
     freedm::broker::CMessage m_;
     std::stringstream ss_;
     ss_ << GetUUID();
     ss_ >> m_.m_srcUUID;
     m_.m_submessages.put("lb.source", ss_.str());
     ss_.clear();
     ss_.str("accept");
     m_.m_submessages.put("lb", ss_.str()); 
     ss_.clear();
     ss_ << DemandValue;
     m_.m_submessages.put("lb.value", ss_.str());
     
     if( peer_->GetUUID() != GetUUID() )
     {
       try
       {
   	 peer_->Send(m_);
       }
       catch (boost::system::system_error& e)
       {
  	 Logger::Info << "Couldn't Send Message To Peer" << std::endl;
       }
       //Then connect to the main grid to get power
       InitiatePowerMigration(1);
     }
     else
     {
     //Nothing; Local Load change from Demand state (Migration will not proceed)
     }
   }
 }//end if("drafting")

 // The Demand node you agreed to supply power to, is awaiting migration
 else if(pt.get<std::string>("lb") == "accept" && peer_->GetUUID() != GetUUID())
 {
   //Ravi would change status from supply to norm once the node is giving power to others
   //to prevent double dipping.  LWI changes this.  LWI allows giving power to more nodes.
   float DemValue;
   std::stringstream ss_;
   ss_ << pt.get<std::string>("lb.value");
   ss_ >> DemValue;
   Logger::Notice << " Draft Accept message received from: "
		  << peer_->GetUUID()<< "with demand of "<<DemValue << std::endl;	     
   if( LPeerNode::SUPPLY == l_Status )
   {
   // Make necessary power setting accordingly to allow power migration
      Logger::Notice<<"\nMigrating power on request from: "<< peer_->GetUUID() << std::endl;
      InitiatePowerMigration(DemValue);            
   }
     
   else
   {
     Logger::Warn << "Unexpected Accept message" << std::endl;
   }
 }//end if("accept")

 // "load" message is sent by the State Collection module of the source 
 // (local or remote). Respond to it by sending in your current load status
 else if(pt.get<std::string>("lb") == "load")
 {
   peer_ = get_peer(line_);
   Logger::Notice << "\nCurrent Load State requested by " << peer_->GetUUID() << std::endl;    
     
   freedm::broker::CMessage m_;
   std::stringstream ss_;   
   ss_ << GetUUID();
   ss_ >> m_.m_srcUUID;
   m_.m_submessages.put("sc", "load");
   m_.m_submessages.put("sc.source", ss_.str());
   ss_.clear();
   if (LPeerNode::SUPPLY == l_Status)
   {
     ss_.str("SUPPLY");
   }
   else if (LPeerNode::DEMAND == l_Status)
   {
     ss_.str("DEMAND");
   }
   else if (LPeerNode::NORM == l_Status)
   {
     ss_.str("NORMAL");
   }
   else
   {
     ss_.str("Unknown");
   }       
   m_.m_submessages.put("sc.status", ss_.str());
   try
   { 
     peer_->Send(m_);
   }
   catch (boost::system::system_error& e)
   {
     Logger::Info << "Couldn't send Message To Peer" << std::endl;
   }
 }//end if("load")

 // Other message type is invalid within lb module
 else
 {
   Logger::Warn << "Invalid Message Type" << std::endl;
 }
 	
}//end function


lbAgent::PeerNodePtr lbAgent::get_peer(std::string uuid)
{
  PeerSet::iterator it = l_AllPeers.find(uuid);
  if(it != l_AllPeers.end())
  {
    return it->second;   
  }
  else
  {
    return PeerNodePtr();
  }
}

lbAgent::PeerNodePtr lbAgent::add_peer(std::string uuid)
{
  Logger::Debug << __PRETTY_FUNCTION__ << std::endl;
  PeerNodePtr tmp_;
  tmp_.reset(new LPeerNode(uuid,GetConnectionManager(),GetIOService(),GetDispatcher()));
  InsertInPeerSet(l_AllPeers,tmp_);
  InsertInPeerSet(m_NoNodes,tmp_);
  return tmp_;
}
////////////////////////////////////////////////////////////
/// LB
/// @description Main function which initiates the algorithm
/// @limitations None
/// @pre: connections to peers should be instantiated (Broker does that)
/// @post: initiation of drafting algorithm
/////////////////////////////////////////////////////////
int lbAgent::LB()
{
  Logger::Debug << __PRETTY_FUNCTION__ << std::endl;
   
  // This initializes the algorithm
  LoadManage();

  return 0;
}

}
