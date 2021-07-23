/*
  Copyright (c) 1997, 1998 Carnegie Mellon University.  All Rights
  Reserved. 

  Permission to use, copy, modify, and distribute this
  software and its documentation is hereby granted (including for
  commercial or for-profit use), provided that both the copyright notice
  and this permission notice appear in all copies of the software,
  derivative works, or modified versions, and any portions thereof, and
  that both notices appear in supporting documentation, and that credit
  is given to Carnegie Mellon University in all publications reporting
  on direct or indirect use of this code or its derivatives.

  ALL CODE, SOFTWARE, PROTOCOLS, AND ARCHITECTURES DEVELOPED BY THE CMU
  MONARCH PROJECT ARE EXPERIMENTAL AND ARE KNOWN TO HAVE BUGS, SOME OF
  WHICH MAY HAVE SERIOUS CONSEQUENCES. CARNEGIE MELLON PROVIDES THIS
  SOFTWARE OR OTHER INTELLECTUAL PROPERTY IN ITS ``AS IS'' CONDITION,
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE OR
  INTELLECTUAL PROPERTY, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
  DAMAGE.

  Carnegie Mellon encourages (but does not require) users of this
  software or intellectual property to return any improvements or
  extensions that they make, and to grant Carnegie Mellon the rights to
  redistribute these changes without encumbrance.

  The AODV code developed by the CMU/MONARCH group was optimized
  and tuned by Samir Das and Mahesh Marina, University of Cincinnati. The
  work was partially done in Sun Microsystems.
*/

/* 
 * ############################################################################
 * This code was extended by Ali Hamidian, Department of Electrical and 
 * Information Technology, Faculty of Engineering (LTH), Lund University.
 * AODV+ allows ns-users to use AODV as an ad hoc routing protocol for 
 * simulations of MANETs with Internet access (wired-cum-wireless scenarios).
 * e-mail: ali.hamidian@eit.lth.se
 * ############################################################################
 */

//#include <ip.h>
#include <aodv/aodv.h>
#include <aodv/aodv_packet.h>
#include <random.h>
#include <cmu-trace.h>
//#include <energy-model.h>
//My modification************************************************************//
#include <mobilenode.h> //For base_stn() and set_base_stn(int addr)
//***************************************************************************//

#define max(a,b)        ( (a) > (b) ? (a) : (b) )
#define CURRENT_TIME    Scheduler::instance().clock()

//ly修改****************************
#define THRESHOLD1 10
#define THRESHOLD2 30
#define max_hop_count 10
#define max_length 50

//***********************************


//#define DEBUG
#ifdef DEBUG
static int route_request = 0;
#endif


/* ===================================================================
   TCL Hooks
   ================================================================= */
int hdr_aodv::offset_;
static class AODVHeaderClass : public PacketHeaderClass {
public:
  AODVHeaderClass() : PacketHeaderClass("PacketHeader/AODV",
					sizeof(hdr_all_aodv)) {
    bind_offset(&hdr_aodv::offset_);
  } 
} class_rtProtoAODV_hdr;

static class AODVclass : public TclClass {
public:
  AODVclass() : TclClass("Agent/AODV") {}
  TclObject* create(int argc, const char*const* argv) {
    assert(argc == 5);
    //return (new AODV((nsaddr_t) atoi(argv[4])));
    return (new AODV((nsaddr_t) Address::instance().str2addr(argv[4])));
  }
} class_rtProtoAODV;


int
AODV::command(int argc, const char*const* argv) {
  if(argc == 2) {
    Tcl& tcl = Tcl::instance();
    
    if(strncasecmp(argv[1], "id", 2) == 0) {
      tcl.resultf("%d", index);
      return TCL_OK;
    }
    
    if(strncasecmp(argv[1], "start", 2) == 0) {
      btimer.handle((Event*) 0);
#ifndef AODV_LINK_LAYER_DETECTION
      htimer.handle((Event*) 0);
      ntimer.handle((Event*) 0);
#endif // LINK LAYER DETECTION
      //My modification******************************************************//
      /*
	Implementing proactive and hybrid gateway discovery method...
      */
      //Call AdvertisementTimer::handle(Event*)
      adtimer.handle((Event*) 0);
      //*********************************************************************//
      rtimer.handle((Event*) 0);
      return TCL_OK;
    }
  }
  else if(argc == 3) {
    if(strcmp(argv[1], "index") == 0) {
      index = atoi(argv[2]);
      return TCL_OK;
    }
    //ly修改*******************
     else if(strcmp(argv[1],"set-mac") == 0){
        mymac = (Mac802_11 *) TclObject::lookup(argv[2]);
        if(mymac == 0){
             fprintf(stderr,"MESPAgent:%s lookup %s failed.\n",argv[1],argv[2]);
             return TCL_ERROR;
        }
        else{
             //printf("Get Node mac bss_id:%d \n",mymac->bss_id());
             return TCL_OK;
        }
     }
    //*************************
   


    else if(strcmp(argv[1], "log-target") == 0 || 
	    strcmp(argv[1], "tracetarget") == 0) {
      logtarget = (Trace*) TclObject::lookup(argv[2]);
      if(logtarget == 0)
	return TCL_ERROR;
      return TCL_OK;
    }
    else if(strcmp(argv[1], "drop-target") == 0) {
      int stat = rqueue.command(argc,argv);
      if(stat != TCL_OK) return stat;
      return Agent::command(argc, argv);
    }
    else if(strcmp(argv[1], "if-queue") == 0) {
      ifqueue = (PriQueue*) TclObject::lookup(argv[2]);
      
      if(ifqueue == 0)
	return TCL_ERROR;
      return TCL_OK;
    }
    else if (strcmp(argv[1], "port-dmux") == 0) {
      dmux_ = (PortClassifier *)TclObject::lookup(argv[2]);
      if (dmux_ == 0) {
	fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__,
		 argv[1], argv[2]);
	return TCL_ERROR;
      }
      return TCL_OK;
    }
  }
  return Agent::command(argc, argv);
}



/* ===================================================================
   Constructor
   ================================================================= */
AODV::AODV(nsaddr_t id) : Agent(PT_AODV),
			  btimer(this), htimer(this), ntimer(this), 
			  rtimer(this), lrtimer(this), adtimer(this), rqueue(){
 
  index = id;
  seqno = 2;
  bid = 1;

  //My modification**********************************************************//
  thisnode = (MobileNode *) (Node::get_node_by_address(id));
  /*
    Implementing proactive gateway discovery method...
  */
  ad_bid = 1;
  /*
    Implementing proactive and hybrid gateway discovery method...
    if gw_discovery==0: Proactive gateway discovery on
    if gw_discovery==1: Hybrid gateway discovery on
  */
  bind("gw_discovery", &gw_discovery);
  //*************************************************************************//
  
  LIST_INIT(&nbhead);
  LIST_INIT(&bihead);

  logtarget = 0;
  ifqueue = 0;
}



/* ===================================================================
   Timers
   ================================================================= */
void
BroadcastTimer::handle(Event*) {
  agent->id_purge();
  Scheduler::instance().schedule(this, &intr, BCAST_ID_SAVE);
}

//My comment*****************************************************************//
/*
  This function is invoked ONLY if AODV_LINK_LAYER_DETECTION is NOT defined. 
  The function uses Hello messages instead of link layer (802.11) feedback to 
  determine when links are up/down.
*/
//***************************************************************************//
void
HelloTimer::handle(Event*) {
  agent->sendHello();
  double interval = MinHelloInterval + 
    ((MaxHelloInterval - MinHelloInterval) * Random::uniform());
  assert(interval >= 0);
  Scheduler::instance().schedule(this, &intr, interval);
}

//My comment*****************************************************************//
/*
  This function is invoked ONLY if AODV_LINK_LAYER_DETECTION is NOT defined.
  See aodv.h.
*/
//***************************************************************************//
void
NeighborTimer::handle(Event*) {
  agent->nb_purge();
  Scheduler::instance().schedule(this, &intr, HELLO_INTERVAL);
}

void
RouteCacheTimer::handle(Event*) {
  agent->rt_purge();
#define FREQUENCY 0.5 // sec
  Scheduler::instance().schedule(this, &intr, FREQUENCY);
}

void
LocalRepairTimer::handle(Event* p)  {  // SRD: 5/4/99
  aodv_rt_entry *rt;
  struct hdr_ip *ih = HDR_IP( (Packet *)p);
  
  /* you get here after the timeout in a local repair attempt */
  /*	fprintf(stderr, "%s\n", __FUNCTION__); */
  
  
  rt = agent->rtable.rt_lookup(ih->daddr());
  
  if(rt && rt->rt_flags != RTF_UP) {
    // route is yet to be repaired
    // I will be conservative and bring down the route
    // and send route errors upstream.
    /* The following assert fails, not sure why */
    /* assert (rt->rt_flags == RTF_IN_REPAIR); */
    
    //rt->rt_seqno++;
    agent->rt_down(rt);
    // send RERR
#ifdef DEBUG
    fprintf(stderr,"\n\n*** Node %d: Dst - %d, failed local repair at %f!\n\n",
	    index, rt->rt_dst, CURRENT_TIME);
#endif      
  }
  Packet::free((Packet *)p);
}

//My modification************************************************************//
/*
  Implementing proactive and hybrid gateway discovery method...
*/
void
AdvertisementTimer::handle(Event*) {
  static bool printed = false;
  
  if(agent->gw_discovery == 0) {
    /*
      Proactive gateway discovery - call sendAdvertisement
      The packet sent is defined as a new AODV packet: AODVTYPE_ADVERTISEMENT
    */
    if(!printed) {
      fprintf(stderr, "\n*************************************************\n");
      fprintf(stderr, "\t Proactive gateway discovery");
      fprintf(stderr, "\n*************************************************\n");
      printed = true;
    }
    agent->sendAdvertisement(NETWORK_DIAMETER);
    
    //Randomize the sending of broadcast packets to reduce collisions
    double interval =  ADVERTISEMENT_INTERVAL * Random::uniform(0.85, 1.15);
    Scheduler::instance().schedule(this, &intr, interval);
  }
  else if(agent->gw_discovery == 1) {
    /*
      Hybrid gateway discovery - call sendAdvertisement
      The packet sent is defined as an new AODV packet: AODVTYPE_ADVERTISEMENT
    */
    if(!printed) {
      fprintf(stderr, "\n*************************************************\n");
      fprintf(stderr, "\t Hybrid gateway discovery");
      fprintf(stderr, "\n*************************************************\n");
      printed = true;
    }
    agent->sendAdvertisement(ADVERTISEMENT_ZONE);
  
    //Randomize the sending of broadcast packets to reduce collisions
    double interval =  ADVERTISEMENT_INTERVAL * Random::uniform(0.85, 1.15);
    Scheduler::instance().schedule(this, &intr, interval);
  }
  else if(agent->gw_discovery == 2) {
    /*
      Reactive gateway discovery - do nothing
    */
    if(!printed) {
      fprintf(stderr, "\n*************************************************\n");
      fprintf(stderr, "\t Reactive gateway discovery");
      fprintf(stderr, "\n*************************************************\n");
      printed = true;
    }
  }
  else {
    fprintf(stderr, "\n\nNo gateway discovery method chosen! Add the following"
	    " line in your Tcl file:"
	    "\n\tAgent/AODV set gw_discovery <0 or 1 or 2>\n\n");
    exit(1);
  }
}
//***************************************************************************//



/* =====================================================================
   Broadcast ID Management Functions
   ===================================================================== */
void
AODV::id_insert(nsaddr_t id, u_int32_t bid) {
  BroadcastID *b = new BroadcastID(id, bid);
  
  assert(b);
  b->expire = CURRENT_TIME + BCAST_ID_SAVE;
  LIST_INSERT_HEAD(&bihead, b, link);
}

/* SRD */
bool
AODV::id_lookup(nsaddr_t id, u_int32_t bid) {
  BroadcastID *b = bihead.lh_first;
  
  // Search the list for a match of source and bid
  for( ; b; b = b->link.le_next) {
    if((b->src == id) && (b->id == bid))
      return true;     
  }
  return false;
}

void
AODV::id_purge() {
  BroadcastID *b = bihead.lh_first;
  BroadcastID *bn;
  double now = CURRENT_TIME;
  
  for(; b; b = bn) {
    bn = b->link.le_next;
    if(b->expire <= now) {
      LIST_REMOVE(b,link);
      delete b;
    }
  }
}



/* =====================================================================
   Helper Functions
   ===================================================================== */
double
AODV::PerHopTime(aodv_rt_entry *rt) {
  int num_non_zero = 0, i;
  double total_latency = 0.0;
  
  if(!rt)
    return ((double) NODE_TRAVERSAL_TIME );
  
  //==> MAX_HISTORY is defined to 3 in rttable.h.
  for(i=0; i < MAX_HISTORY; i++) {
    if(rt->rt_disc_latency[i] > 0.0) {
      num_non_zero++;
      total_latency += rt->rt_disc_latency[i];
    }
  }
  if(num_non_zero > 0)
    return(total_latency / (double) num_non_zero);
  else
    return((double) NODE_TRAVERSAL_TIME);
  
}



/* =====================================================================
   Link Failure Management Functions
   ===================================================================== */
static void
aodv_rt_failed_callback(Packet *p, void *arg) {
  ((AODV*) arg)->rt_ll_failed(p);
}

/*
 * This routine is invoked when the link-layer reports a route failed.
 */
void
AODV::rt_ll_failed(Packet *p) {
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  aodv_rt_entry *rt;
  nsaddr_t broken_nbr = ch->next_hop_;
  
  //print_routing_table();
  
#ifndef AODV_LINK_LAYER_DETECTION
  drop(p, DROP_RTR_MAC_CALLBACK);
#else 
  
  /*
   * Non-data packets and Broadcast Packets can be dropped.
   */
  if(! DATA_PACKET(ch->ptype()) || (u_int32_t) ih->daddr() == IP_BROADCAST) {
    drop(p, DROP_RTR_MAC_CALLBACK);
    return;
  }
  log_link_broke(p);
  if((rt = rtable.rt_lookup(ih->daddr())) == 0) {
    drop(p, DROP_RTR_MAC_CALLBACK);
    return;
  }
  log_link_del(ch->next_hop_);
  
  
#ifdef AODV_LOCAL_REPAIR
  /* if the broken link is closer to the dest than source, 
     attempt a local repair. Otherwise, bring down the route. */
  if(ch->num_forwards() > rt->rt_hops) {
    local_rt_repair(rt, p); // local repair
    // retrieve all the packets in the ifq using this link,
    // queue the packets for which local repair is done, 
    return;
  }
  else
#endif // LOCAL REPAIR	
    {
      drop(p, DROP_RTR_MAC_CALLBACK);
      // Do the same thing for other packets in the interface queue using the
      // broken link -Mahesh
      while((p = ifqueue->filter(broken_nbr))) {
	drop(p, DROP_RTR_MAC_CALLBACK);
      }
      nb_delete(broken_nbr);
    }
  
#endif // LINK LAYER DETECTION
}

void
AODV::handle_link_failure(nsaddr_t id) {
  aodv_rt_entry *rt, *rtn;
  Packet *rerr = Packet::alloc();
  struct hdr_aodv_error *re = HDR_AODV_ERROR(rerr);
  
  //My modification********************************************//
  /*
    I added some conditions to the if statement below.
  */
  nsaddr_t send_rt_nexthop = 0;
  aodv_rt_entry *fn_rt = find_fn_entry();
  aodv_rt_entry *send_rt;
  if(fn_rt) {
    send_rt = find_send_entry(fn_rt);
  }    
  if(send_rt) {
    send_rt_nexthop = send_rt->rt_nexthop;
  }
  //***********************************************************//
  
  re->DestCount = 0;
  for(rt = rtable.head(); rt; rt = rtn) {  // for each rt entry
    rtn = rt->rt_link.le_next;
    //My modification
    if(((rt->rt_hops != INFINITY2) && (rt->rt_nexthop == id))
       || (rt->rt_dst == DEFAULT && rt->rt_hops != INFINITY2 && 
	   send_rt_nexthop == id) ) {
      assert (rt->rt_flags == RTF_UP);
      assert((rt->rt_seqno%2) == 0);
      rt->rt_seqno++;
      re->unreachable_dst[re->DestCount] = rt->rt_dst;
      re->unreachable_dst_seqno[re->DestCount] = rt->rt_seqno;
#ifdef DEBUG
      fprintf(stderr, "%d - Can't reach next hop=%d, dest=%d\n", index, 
	      rt->rt_nexthop, re->unreachable_dst[re->DestCount]);
#endif // DEBUG
      re->DestCount += 1;
      rt_down(rt);
    } /*if*/
    // remove the lost neighbor from all the precursor lists
    rt->pc_delete(id);
  } /*for*/
  
  if(re->DestCount > 0) {
#ifdef DEBUG
    fprintf(stderr, "%d - sending RERR at %f\n", index, CURRENT_TIME);
#endif // DEBUG
    sendError(rerr, false);
  }
  else {
    Packet::free(rerr);
  }
}

void
AODV::local_rt_repair(aodv_rt_entry *rt, Packet *p) {
#ifdef DEBUG
  fprintf(stderr,"%s: Dst - %d\n", __FUNCTION__, rt->rt_dst); 
#endif
  
  //My modification**********************************************************//
  /*
    RFC 3561, Section 6.12:
    "To repair the link break, the node increments the sequence number for the
    destination and then broadcasts a RREQ for that destination."
  */
  rt->rt_seqno += 1;
  //*************************************************************************//
  
  // Buffer the packet 
  rqueue.enque(p);
  // mark the route as under repair 
  rt->rt_flags = RTF_IN_REPAIR;
  sendRequest(rt->rt_dst, 0x00);
  // set up a timer interrupt
  Scheduler::instance().schedule(&lrtimer, p->copy(), rt->rt_req_timeout);
}

void
AODV::rt_update(aodv_rt_entry *rt, u_int32_t seqnum, u_int16_t metric,
	       	nsaddr_t nexthop, double expire_time,int load) {
  rt->rt_seqno = seqnum;
  rt->rt_hops = metric;
  rt->rt_flags = RTF_UP;
  rt->rt_nexthop = nexthop;
  rt->rt_expire = expire_time;
//ly修改**************************************
  rt->rt_load = load;//这个load是乘以100之后的
  //************************************************************
   
  //print_routing_table();
}

void
AODV::rt_down(aodv_rt_entry *rt) {
  /*
   *  Make sure that you don't "down" a route more than once.
   */

  if(rt->rt_flags == RTF_DOWN) {
    return;
  }

  // assert (rt->rt_seqno%2); // is the seqno odd?
  rt->rt_last_hop_count = rt->rt_hops;
  rt->rt_hops = INFINITY2;
  rt->rt_flags = RTF_DOWN;
  rt->rt_nexthop = 0;
  rt->rt_expire = 0;
} /* rt_down function */



/* =====================================================================
   Route Handling Functions
   ===================================================================== */
//My modification************************************************************//
/*
  This function prints the routing table.
*/
void
AODV::print_routing_table() {
  aodv_rt_entry *my_rt;
  int i=1; 
  printf("Routing Table for %d\n",index);
  for(my_rt = rtable.head(); my_rt; my_rt = my_rt->rt_link.le_next) {  
    printf("%d) rt_dst=%d rt_nexthop=%d rt_hops=%d rt_seqno=%d rt_expire=%f "
	   "rt_flags=%u\n", i++,my_rt->rt_dst,my_rt->rt_nexthop,
	   my_rt->rt_hops,my_rt->rt_seqno,my_rt->rt_expire,my_rt->rt_flags);
  }
}
//***************************************************************************//


//My modification************************************************************//
/*
  This function finds the route entry that the packet(s) can be sent to.
  
  Example of a routing table for a mobile node (MN3) with address 1.0.3:
  Assume that 1.0.0 is the address of the gateway (GW), 1.0.2 is the 
  address of a mobile node (MN2) and 0.0.1 is the address of a fixed node (FN).
  Assume further that MN3 wants to communicate with FN. The next hop of FN is 
  "Default". The functions looks for "Default" and finds that the next hop for 
  "Default" is 1.0.0 and finally it finds that the next hop to 1.0.0 is 1.0.2.
  Hence, the packet(s) are sent to address 1.0.2 (MN2).
  
  Destination          Next hop
  0.0.1                Default
  Default              1.0.0
  1.0.0                1.0.2
*/

aodv_rt_entry*
AODV::find_send_entry(aodv_rt_entry *rt) {
  /*
    Return if the destination is not a fixed node. All the things below this 
    if statement should be done only when the destination is a fixed node.
  */
  if(rt->rt_nexthop != DEFAULT) {
    return rt;
  }
  
  aodv_rt_entry *default_rt = rtable.rt_lookup(DEFAULT);
  aodv_rt_entry *temp_rt = rtable.rt_lookup(default_rt->rt_nexthop);
  
  if(default_rt && temp_rt && default_rt->rt_flags == RTF_UP && 
     temp_rt->rt_flags == RTF_UP && temp_rt->rt_dst == temp_rt->rt_nexthop) {
    return temp_rt;
  }
  else if(default_rt && temp_rt && default_rt->rt_flags == RTF_UP && 
	  temp_rt->rt_flags == RTF_UP) {
    return temp_rt;
  }
  else if(default_rt->rt_flags != RTF_UP) {
    return default_rt;
  }
  else if(temp_rt->rt_flags != RTF_UP) {
    return temp_rt;
  }
  else {
    fprintf(stderr, "\n\n\n!!! aodv.cc ERROR !!!\n\n\n");
    return rt;
  }
}


aodv_rt_entry*
AODV::find_fn_entry() {
  aodv_rt_entry *rt = rtable.head();
  for(; rt; rt = rt->rt_link.le_next) {  // for each rt entry
    if(rt->rt_nexthop == DEFAULT) {
      break;
    }
  }
  return rt;
}
//***************************************************************************//


void
AODV::rt_resolve(Packet *p) {
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  aodv_rt_entry *rt;
  
  //==> An entry to the default route...
  aodv_rt_entry *default_rt = rtable.rt_lookup(DEFAULT);
  
  /*
   * Set the transmit failure callback. That won't change.
   */
  ch->xmit_failure_ = aodv_rt_failed_callback;
  ch->xmit_failure_data_ = (void*) this;
  
  rt = rtable.rt_lookup(ih->daddr());
  
  //My modification**********************************************************//
  /*
    Internet connection broken...
  */
  if(rt && (rt->rt_nexthop == DEFAULT) && 
     find_send_entry(rt)->rt_flags != RTF_UP && index == ih->saddr()) {
    
    aodv_rt_entry *ALL_GW_rt = rtable.rt_lookup(ALL_MANET_GW_MULTICAST);
    if(ALL_GW_rt == 0) {
      rtable.rt_add(ALL_MANET_GW_MULTICAST);
    }
    rqueue.enque(p);
    sendRequest(ALL_MANET_GW_MULTICAST, RREQ_IFLAG);
    return;
  }
  //*************************************************************************//
  
  if(rt == 0) {
    rt = rtable.rt_add(ih->daddr());
  }
  
  /*
   * 1. If the route is up, forward the packet.
   *
   * My modification
   * I added the second condition: MN_S sends packets to MN_I which forwards 
   * them to GW. Then link between MN_I and GW breaks. MN_I broadcasts RERR 
   * which should be received by MN_S but the RERR is dropped due to collision.
   * MN_S keeps sending packets to MN_I, which must send another RERR because
   * it can't forward the packets. Since routes to fixed nodes don't go down 
   * (this should be changed later) MN_I must check that the route to the 
   * nexthop is up before trying to forward the packet!
   */
  if(rt->rt_flags == RTF_UP && find_send_entry(rt)->rt_flags == RTF_UP) {
    assert(rt->rt_hops != INFINITY2);
    //==> I've changed "rt" to "find_send_entry(rt)".
    forward(find_send_entry(rt), p, NO_DELAY);
  }
  /*
   * 2. If I am the source of the packet, then do a route request.
   */
  else if(ih->saddr() == index) {
    rqueue.enque(p);
    sendRequest(rt->rt_dst, 0x00);
  }
  //My modification**********************************************************//
  /*
   * 3. If valid default route exists, update route (to fixed node) and forward
   * the packet.
   *
   * Maybe we should have the condition "rt->rt_nexthop==DEFAULT"?
   */
  else if(default_rt && (default_rt->rt_flags == RTF_UP) && 
	  rt->rt_flags != RTF_IN_REPAIR && rt->rt_seqno == 0) {
    /*
      Valid Default Route exists. Network-wide search has been done once but no
      RREP received. Assume the destination node is a fixed node (FN) on 
      Internet. 
      1. Update your route entry in the routing table to FN. 
      2. Forward these "future" packets to FN.
      Only intermediate nodes, that don't have any valid route to the 
      destination, will enter this else-if because the sender will
      enter else-if(ih->saddr()==index). 
      
      Condition 3 guarantees that this function works correct also when local 
      repair is used.
      Condition 4 guarantees that this route is created recently so a MN does
      not enter this 'else if' statement although it shouldn't (e.g. when a 
      valid default route exists and this MN is trying to forward a packet to 
      another MN and the route to that MN went down recently). 
      
      NOTE! Don't mix *rt (entry to FN) and *default_rt (entry to Default 
      Route)!
    */
    rt_update(rt, default_rt->rt_seqno, default_rt->rt_hops, DEFAULT, 
	      default_rt->rt_expire,default_rt->rt_load);//ly修改
    
    //Forward the packet to FN...
    //==> I've changed "rt" to "find_send_entry(rt)".
    forward(find_send_entry(rt), p, NO_DELAY);
  }
  
  /*
   * 4. I am trying to forward a packet for someone else to whom I don't have
   *    a route, but I'm GW ==> send RREQ. 
   */
  else if(index == thisnode->base_stn()) {
    /*
      I am trying to forward a packet for someone else to whom I don't have a
      route. But since I'm GW and I'm forwarding it means that this packet is 
      from a fixed node. So broadcast a RREQ if there is no route to the 
      destination.
    */
    rqueue.enque(p);
    sendRequest(rt->rt_dst, 0x00); 
  }
  //*************************************************************************//
  /*
   * 5. A local repair is in progress. Buffer the packet. 
   */
  else if(rt->rt_flags == RTF_IN_REPAIR) {
    rqueue.enque(p);
  }
  /*
   * 6. I am trying to forward a packet for someone else to whom I don't have
   *    a route.
   */
  else {
    Packet *rerr = Packet::alloc();
    struct hdr_aodv_error *re = HDR_AODV_ERROR(rerr);
    /* 
     * For now, drop the packet and send error upstream.
     * Now the route errors are broadcast to upstream
     * neighbors - Mahesh 09/11/99
     */	
    
    //My modification*****************************************//
    /*
      Unreachable destination is not FN, but DEFAULT...
      May be changed later...
    */
    if(rt->rt_nexthop == DEFAULT && find_send_entry(rt)->rt_flags != RTF_UP) {
      assert(find_send_entry(rt)->rt_flags == RTF_DOWN);
      re->DestCount = 0;
      re->unreachable_dst[re->DestCount] = thisnode->base_stn();
      re->unreachable_dst_seqno[re->DestCount] = find_send_entry(rt)->rt_seqno;
      re->DestCount += 1;
    }
    else {
      //*******************************************************//
      assert (rt->rt_flags == RTF_DOWN);
      re->DestCount = 0;
      re->unreachable_dst[re->DestCount] = rt->rt_dst;
      re->unreachable_dst_seqno[re->DestCount] = rt->rt_seqno;
      re->DestCount += 1;
    }
    
#ifdef DEBUG
    fprintf(stderr, "!!! %d - NRTE dst=%d, sending RERR.................\n", 
	    index, rt->rt_dst);
#endif
    sendError(rerr, false);
    drop(p, DROP_RTR_NO_ROUTE);
  }
}

void
AODV::rt_purge() {
  aodv_rt_entry *rt, *rtn;
  double now = CURRENT_TIME;
  double delay = 0.0;
  Packet *p;
  //My modification**************************************//
  aodv_rt_entry *default_rt = rtable.rt_lookup(DEFAULT);
  //*****************************************************//
  
  for(rt = rtable.head(); rt; rt = rtn) {  // for each rt entry
    rtn = rt->rt_link.le_next;
    /*
      1. If a valid route has expired, purge all packets from sendbuffer and 
      invalidate the route.
      
      My modification
      I added the third condition: Route to fixed node should always have 
      DEFAULT as nexthop! Don't call route down if the expired route is a route
      to a fixed node.
    */
    if((rt->rt_flags == RTF_UP) && (rt->rt_expire < now) 
       && (rt->rt_nexthop != DEFAULT) ) {
      assert(rt->rt_hops != INFINITY2);
      while((p = rqueue.deque(rt->rt_dst))) {
#ifdef DEBUG
	fprintf(stderr, "%s: calling drop()\n", __FUNCTION__);
#endif // DEBUG
	drop(p, DROP_RTR_NO_ROUTE);
      }
      rt->rt_seqno++;
      assert (rt->rt_seqno%2);
      rt_down(rt);
    }
    //My modification********************************************************//
    /*
      2. If the route has not expired and it is a route to a fixed node and a
      valid default route exists and there are packets in the sendbuffer 
      waiting, forward them.
    */
    else if(rt->rt_flags == RTF_UP && rt->rt_nexthop == DEFAULT && 
	    find_send_entry(rt)->rt_flags == RTF_UP) {
      assert(default_rt->rt_hops != INFINITY2);
      int j = 0;
      while((p = rqueue.deque(rt->rt_dst))) {
	++j;
	//==> I've changed "rt" to "find_send_entry(rt)".
	forward (find_send_entry(rt), p, delay);
	delay += ARP_DELAY;
      }
#ifdef DEBUG
      if(j > 0 && rt->rt_nexthop == DEFAULT) {
	fprintf(stderr,"%d - %d packet(s) destined to %d emptied from "
		"sendbuffer at %f\n\tGW is %d (rt_purge)\n", index, j, 
		rt->rt_dst, CURRENT_TIME, default_rt->rt_nexthop);
	printf("%d - %d packet(s) destined to %d emptied from "
	       "sendbuffer at %f, GW is %d (rt_purge)\n", index, j, 
	       rt->rt_dst, CURRENT_TIME, default_rt->rt_nexthop);
      }
      else if(j > 0) {
	fprintf(stderr,"%d - %d packet(s) destined to %d emptied from "
		"sendbuffer at %f\n (rt_purge)\n", index, j, rt->rt_dst, 
		CURRENT_TIME);
	printf("%d - %d packet(s) destined to %d emptied from sendbuffer at %f"
	       ", (rt_purge)\n", index, j, rt->rt_dst, CURRENT_TIME);
      }
#endif
    }
    //***********************************************************************//
    /*
      3. If the route has not expired, and there are packets in the sendbuffer
      waiting, forward them. This should not be needed, but this extra check 
      does no harm.
      
      My modification
      I added the second condition below in order to prevent packets getting 
      forwarded to the default route (-10) although it is down.
    */
    else if(rt->rt_flags == RTF_UP && rt->rt_nexthop != DEFAULT) {
      assert(rt->rt_hops != INFINITY2);
      while((p = rqueue.deque(rt->rt_dst))) {
	//==> I've changed "rt" to "find_send_entry(rt)".
	forward (find_send_entry(rt), p, delay);
	delay += ARP_DELAY;
      }
    }
    /*
      4. If the route is down and if there is a packet for this destination 
      waiting in the sendbuffer, then send out route request. sendRequest will
      check whether it is time to really send out request or not.
      This may not be crucial to do it here, as each generated packet will do a
      sendRequest anyway.
    */
    else if(rqueue.find(rt->rt_dst)) {
      sendRequest(rt->rt_dst, 0x00); 
    }
  }
}



/* =====================================================================
   Packet Reception Routines
   ===================================================================== */
void
AODV::recv(Packet *p, Handler*) {
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  assert(initialized());
  /*
    //assert(p->incoming == 0);
    NOTE: use of incoming flag has been depracated; In order to track direction
    of pkt flow, direction_ in hdr_cmn is used instead. See packet.h for 
    details.
  */
  
  //print_routing_table();
  
  if(ch->ptype() == PT_AODV) {
    ih->ttl_ -= 1;
    recvAODV(p);
    return;
  }
  
  /*
   * Must be a packet I'm originating...
   */
  if((ih->saddr() == index) && (ch->num_forwards() == 0)) {
    //My modification********************************************************//
    /*
     * tcp.cc/tcp-full.cc add both TCP and IP headers to the packets, i.e.,
     * tcpip_base_hdr_size_ = 40 B (see ns-default.tcl). Since we shouldn't add
     * the IP header size twice (both in AODV and in TCP), we should add it 
     * here only if the packet is not PT_TCP or PT_ACK.
     */
    //Add the IP Header to non-TCP packets
    if(ch->ptype() != PT_TCP && ch->ptype() != PT_ACK) {
      ch->size() += IP_HDR_LEN;
    }
    //***********************************************************************//
    ih->ttl_ = NETWORK_DIAMETER;
  }
  /*
   * I received a packet that I sent. Probably a routing loop.
   */
  else if(ih->saddr() == index) {
    drop(p, DROP_RTR_ROUTE_LOOP);
    return;
  }
  /*
   * Packet I'm forwarding...
   */
  else {
    // Check the TTL. If it is zero, then discard.
    if(--ih->ttl_ == 0) {
      if(DATA_PACKET(ch->ptype())) {
	fprintf(stderr, "\n%d - TTL=0 ==> DATA PACKET!!!\n\n", index);
      }
      drop(p, DROP_RTR_TTL);
      return;
    }
  }
  rt_resolve(p);
}


void
AODV::recvAODV(Packet *p) {
  struct hdr_aodv *ah = HDR_AODV(p);
  struct hdr_ip *ih = HDR_IP(p);
  
  assert(ih->sport() == RT_PORT);
  assert(ih->dport() == RT_PORT);
 
  /*
   * Incoming Packets.
   */
  switch(ah->ah_type) {
   
  case AODVTYPE_RREQ:
    recvRequest(p);
    break;
   
  case AODVTYPE_RREP:
    recvReply(p);
    break;
   
  case AODVTYPE_RERR:
    recvError(p);
    break;
   
  case AODVTYPE_HELLO:
    recvHello(p);
    break;
   
    //My modification********************************************************//
    /*
      Implementing proactive gateway discovery...
    */
  case AODVTYPE_ADVERTISEMENT:
    recvAdvertisement(p);
    break;
    //***********************************************************************//
   
  default:
    fprintf(stderr, "Invalid AODV type (%x)\n", ah->ah_type);
    exit(1);
  }
}


void
AODV::recvRequest(Packet *p) {
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_aodv_request *rq = HDR_AODV_REQUEST(p);
  aodv_rt_entry *rt;
  
  /*
   * Drop if: 1. I'm the source   2. I recently heard this request
   */
  if(rq->rq_src == index) {
#ifdef DEBUG
    //fprintf(stderr, "%d - got my own RREQ, drop it\n", index);
#endif // DEBUG
    Packet::free(p);
    return;
  } 
  
  if(id_lookup(rq->rq_src, rq->rq_bcast_id)) {
#ifdef DEBUG
    //fprintf(stderr, "%d - discarding RREQ\n", index);
#endif // DEBUG
    Packet::free(p);
    return;
  }
  
  /*
   * Cache the broadcast ID
   */
  id_insert(rq->rq_src, rq->rq_bcast_id);
  
  
  /* 
   * We are either going to forward the REQUEST or generate a REPLY. Before we
   * do anything, we make sure that the REVERSE route is in the route table.
   */
  aodv_rt_entry *rt0; // rt0 is the reverse route 
  
  rt0 = rtable.rt_lookup(rq->rq_src);
  if(rt0 == 0) { /* if not in the route table */
    // Create an entry for the reverse route.
    rt0 = rtable.rt_add(rq->rq_src);
  }
  
  rt0->rt_expire = max(rt0->rt_expire, (CURRENT_TIME + REV_ROUTE_LIFE));
  
  if((rq->rq_src_seqno > rt0->rt_seqno ) ||
     ((rq->rq_src_seqno == rt0->rt_seqno) && 
      (rq->rq_hop_count < rt0->rt_hops)) ) {
    // If we have a fresher sequence number or less #hops for the same sequence
    // number, update the rt entry. Else don't bother.
    rt_update(rt0, rq->rq_src_seqno, rq->rq_hop_count, ih->saddr(),
	      max(rt0->rt_expire, (CURRENT_TIME + REV_ROUTE_LIFE)),0);
    
    if(rt0->rt_req_timeout > 0.0) {
      // Reset the soft state and 
      // set expiry time to CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT.
      // This is because route is used in the forward direction,
      // but only sources get benefited by this change.
      rt0->rt_req_cnt = 0;
      rt0->rt_req_timeout = 0.0; 
      rt0->rt_req_last_ttl = rq->rq_hop_count;
      rt0->rt_expire = CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT;
    }
    
    /* Find out whether any buffered packet can benefit from the 
     * reverse route.
     * May need some change in the following code - Mahesh 09/11/99
     */
    assert (rt0->rt_flags == RTF_UP);
    Packet *buffered_pkt;
    while((buffered_pkt = rqueue.deque(rt0->rt_dst))) {
      if(rt0 && (rt0->rt_flags == RTF_UP)) {
	assert(rt0->rt_hops != INFINITY2);
	forward(rt0, buffered_pkt, NO_DELAY);
      }
    }
  } 
  // End for putting reverse route in rt table
  
  
  /*
   * We have taken care of the reverse route stuff.
   * Now see whether we can send a route reply. 
   */
  rt = rtable.rt_lookup(rq->rq_dst);
  
  /*
   * 1. First check if I am the destination ..
   */
  if(rq->rq_dst == index) {
#ifdef DEBUG
    fprintf(stderr, "%d - destination sending RREP at %f\n", index, 
	    CURRENT_TIME);
#endif // DEBUG
    
    // Just to be safe, I use the max. Somebody may have
    // incremented the dst seqno.
    seqno = max(seqno, rq->rq_dst_seqno) + 1;
    if(seqno%2) seqno++;
    
    sendReply(rq->rq_src,           // Destination IP address from RREQ message
	      1,                    // Hop Count
	      index,                // Dest IP Address
	      seqno,                // Dest Sequence Num
	      MY_ROUTE_TIMEOUT,     // Lifetime
	      rq->rq_timestamp,     // timestamp
	      0x00,                 //Flag
              0);                   //ly修改              
    Packet::free(p);
  }
  
  /*
   * 2. I am not the destination, but I may have a fresh enough route.
   *
   * My modification
   * I added the third condition so intermediate MNs don't send RREP to RREQs
   * for FN! If they do, the source thinks FN is a MN!
   */
  else if(rt && (rt->rt_hops != INFINITY2) && 
	  (rt->rt_seqno >= rq->rq_dst_seqno) && rt->rt_nexthop != DEFAULT) {
#ifdef DEBUG
    fprintf(stderr, "%d - Intermediate node sending RREP\n", index);
#endif
    assert(rq->rq_dst == rt->rt_dst);
    //assert ((rt->rt_seqno%2) == 0);	// is the seqno even?
    sendReply(rq->rq_src,           // Destination IP address from RREQ message
	      rt->rt_hops + 1,
	      rq->rq_dst,
	      rt->rt_seqno,
	      (u_int32_t) (rt->rt_expire - CURRENT_TIME),
	      rq->rq_timestamp,
	      0x00,
              0);                      //ly修改
    // Insert nexthops to RREQ source and RREQ destination in the
    // precursor lists of destination and source respectively
    rt->pc_insert(rt0->rt_nexthop); // nexthop to RREQ source
    rt0->pc_insert(rt->rt_nexthop); // nexthop to RREQ destination
    // TODO: send grat RREP to dst if G flag set in RREQ

    //My comment*********************************//
    /*
      TODO:
      If I'm GW and I have a route to the destination (which is a MN), send
      a RREP_I besides the RREP packet!
    */
    //*******************************************//
    
    Packet::free(p);
  }
  
  //My modification**********************************************************//
  /*
   * 3. I am not the destination, I don't have a route, but maybe I'm a gateway
   * and this is a RREQ_I packet destined to ALL_MANET_GW_MULTICAST.
   */
  /*
    If receive RREQ_I && GW: send RREP_I
    If receive RREQ_I && !GW: forward RREQ_I (see else below)
  */
  else if(rq->rq_flags == RREQ_IFLAG && index == thisnode->base_stn() 
	  && rq->rq_dst == ALL_MANET_GW_MULTICAST) {
#ifdef DEBUG
    fprintf(stderr, "%d - GW received RREQ_I ==> GW sending RREP_I\n", index);
#endif

    //ly修改，如果队列长度过大，就不发送*******************
       int  ifqlen =  mymac->getqlen();
    if(ifqlen > THRESHOLD2){
       Packet::free(p);
       return;// 
    } 
    //***************************************************
    
    seqno = max(seqno, rq->rq_dst_seqno) + 1;
    if(seqno%2) seqno++;
    
    sendReply(rq->rq_src,          //Address of the node who sent the RREQ
	      1,                   // Hop Count
	      index,               // Dest IP Address
	      seqno,               // Dest Sequence Num
	      GWINFO_LIFETIME,     //Lifetime, defined to 10 sec
	      rq->rq_timestamp,    //Timestamp, when the RREQ was sent
	      RREP_IFLAG,          //I Flag
              ifqlen);         
    Packet::free(p);
  }
  /*
   * 4. I am not the destination, I don't have a route, this is not a RREQ_I 
   * packet but maybe I'm a gateway.
   */
  else if(rt == 0 && index == thisnode->base_stn()) {
#ifdef DEBUG
    fprintf(stderr, "%d - GW sending RREP_I\n", index);
#endif
   //ly修改**************************************************
      int  ifqlen =  mymac->getqlen();
    if(ifqlen > THRESHOLD2){
       Packet::free(p);
       return;//
    }
   
   //********************************************************
   



    seqno = max(seqno, rq->rq_dst_seqno) + 1;
    if(seqno%2) seqno++;
    
    sendReply(rq->rq_src,          //Address of the node who sent the RREQ
	      1,                   // Hop Count
	      index,               // Dest IP Address
	      seqno,               // Dest Sequence Num
	      GWINFO_LIFETIME,     //Lifetime, defined to 10 sec
	      rq->rq_timestamp,    //Timestamp, when the RREQ was sent
	      RREP_IFLAG,          //I Flag
              ifqlen);             //ly修改，增加队列长度
    Packet::free(p); 
  }
  //*************************************************************************//
  
  /*
   * 5. I'm not destination; I don't have a route, I'm not gateway. 
   *    Can't reply. So forward the Route Request
   */
  else {
    ih->saddr() = index;
    ih->daddr() = IP_BROADCAST;
    rq->rq_hop_count += 1;
    // Maximum sequence number seen en route
    if(rt) rq->rq_dst_seqno = max(rt->rt_seqno, rq->rq_dst_seqno);
    forward((aodv_rt_entry*) 0, p, DELAY);
  }
}


void
AODV::recvReply(Packet *p) {
  //struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_aodv_reply *rp = HDR_AODV_REPLY(p);
  aodv_rt_entry *rt;
  char suppress_reply = 0;
  double delay = 0.0;

#ifdef DEBUG
  if(ih->daddr() == index && rp->rp_flags == RREP_IFLAG) {
    fprintf(stderr, "%d - source received RREP_I from %d\n",index, rp->rp_dst);
  }
  else if(ih->daddr() == index) {
    fprintf(stderr, "%d - source received RREP from %d\n", index, rp->rp_dst);
  }
  else if(rp->rp_flags == RREP_IFLAG && ih->daddr() != (nsaddr_t)IP_BROADCAST){
    fprintf(stderr, "%d - received RREP_I, forward RREP_I\n", index); 
  }
  else if(rp->rp_flags != RREP_IFLAG) {
    fprintf(stderr, "%d - received RREP, forward RREP\n", index);
  }
#endif // DEBUG
  
  //My modification**********************************************************//
  /*
    Implementing hybrid gateway discovery method...
  */
  if(rp->rp_flags == RREP_IFLAG && index == thisnode->base_stn()) {
    Packet::free(p);
    return;
  }
  //*************************************************************************//
  
  /*
   *  Got a reply. So reset the "soft state" maintained for 
   *  route requests in the request table. We don't really have
   *  have a separate request table. It is just a part of the
   *  routing table itself. 
   *
   * Note that rp_dst is the dest of the DATA packets, not the dest of the 
   * RREP, which is the src of the data packets.
   * 
   */
  rt = rtable.rt_lookup(rp->rp_dst);
  
  /*
   *  If I don't have a rt entry to this host... adding
   */
  if(rt == 0) {
    rt = rtable.rt_add(rp->rp_dst);
  }
  
  /*
   * Add a forward route table entry... here I am following 
   * Perkins-Royer AODV paper almost literally - SRD 5/99
   */
  //My modification
  //I added the condition "inactive route".
  if((rt->rt_flags != RTF_UP) ||              // inactive route 
     (rt->rt_seqno < rp->rp_dst_seqno) ||     // newer route 
     ((rt->rt_seqno == rp->rp_dst_seqno) &&  
      (rt->rt_hops > rp->rp_hop_count)) ) { // shorter or better route
    // Update the rt entry
    //My modification**************************************//
    //Bug fix from Riadh
    /*rt_update(rt, rp->rp_dst_seqno, rp->rp_hop_count,
      rp->rp_src, CURRENT_TIME + rp->rp_lifetime);*/
    rt_update(rt, rp->rp_dst_seqno, rp->rp_hop_count,
	      ih->saddr(), CURRENT_TIME + rp->rp_lifetime,0);
    //*****************************************************//
    
    // reset the soft state
    rt->rt_req_cnt = 0;
    rt->rt_req_timeout = 0.0; 
    rt->rt_req_last_ttl = rp->rp_hop_count;
    
    if(ih->daddr() == index) { // If I am the original source
      // Update the route discovery latency statistics
      // rp->rp_timestamp is the time of request origination
      
      rt->rt_disc_latency[rt->hist_indx] = (CURRENT_TIME - rp->rp_timestamp)
	/ (double) rp->rp_hop_count;
      // increment indx for next time
      rt->hist_indx = (rt->hist_indx + 1) % MAX_HISTORY;
    }
    
    /*
     * Send all packets queued in the sendbuffer destined for
     * this destination. 
     * XXX - observe the "second" use of p.
     */
    Packet *buf_pkt;
    int j = 0;
    while((buf_pkt = rqueue.deque(rt->rt_dst))) {
      if(rt->rt_hops != INFINITY2) {
	assert (rt->rt_flags == RTF_UP);
	// Delay them a little to help ARP. Otherwise ARP 
	// may drop packets. -SRD 5/23/99
	++j;
	forward(rt, buf_pkt, delay);
	delay += ARP_DELAY;
      } /*if*/
    } /*while*/
#ifdef DEBUG
    if(j > 0) {
      fprintf(stderr,"%d - %d packet(s) destined to %d emptied from sendbuffer"
	      " at %f\n (recvReply)\n", index, j, rt->rt_dst, CURRENT_TIME);
    }
#endif
  } /*if newer/shorter rt*/
  else {
    suppress_reply = 1;
  }
  //My modification**********************************************************//
  /*
    Add/update Default Route if you've received a RREP_I...
  */
  if(rp->rp_flags == RREP_IFLAG) {
    suppress_reply = 0;
    
    aodv_rt_entry *ALL_GW_rt;
    if((ALL_GW_rt = rtable.rt_lookup(ALL_MANET_GW_MULTICAST))) {
      // reset the soft state
      ALL_GW_rt->rt_req_timeout = 0.0; 
      if(rp->rp_hop_count != INFINITY2) {
	ALL_GW_rt->rt_req_last_ttl = rp->rp_hop_count;
      }
    }
    
    /*
      2. GATEWAY SELECTION
      
      1. If Default Route does not exist: Create and update route
      2. If Default Route already exists: Update route if number of hops to 
      the new gateway is less than the number of hops to the current gateway
    
      NOTE: In the current form, a MN that receives a RREP_I from another GW 
      than its default GW, will select the new GW ONLY if the new GW is closer
      than the default GW! It means that it will forward a RREP_I from the new
      GW although it doesn't use that GW as its default GW! So when the 
      destination receives the RREP_I, it will think that it is using the new 
      GW, which is not correct. This might cause problems!
      One way to solve it might be to force a MN that receives a RREP_I from
      a new GW, to select the new GW if it is going to forward the RREP_I to
      another MN! See the commented code below...
      This problem also applies to recvAdvertisement()!
      
      See comment in recvError!
    */
    aodv_rt_entry *default_rt = rtable.rt_lookup(DEFAULT);
    if(default_rt == 0) {
      default_rt = rtable.rt_add(DEFAULT);
    }
   //ly修改,增加选择因子****************************************************
     double gw1load = 0.7* rp->rp_hop_count / max_hop_count  + 0.3 * rp->ifqlen / max_length;
   int gw1_load = (int)gw1load*100;

    
    if((default_rt->rt_flags != RTF_UP) ||            // no route before
	
       (rp->rp_dst == default_rt->rt_nexthop &&
	rp->rp_dst_seqno > default_rt->rt_seqno) ||  // newer route 
	
       (rp->rp_dst == default_rt->rt_nexthop &&
	rp->rp_dst_seqno == default_rt->rt_seqno &&  
	rp->rp_hop_count < default_rt->rt_hops) ||   // shorter route

        (rp->rp_dst != default_rt->rt_nexthop && rp->rp_hop_count==1)|| //只有1跳

       (rp->rp_dst != default_rt->rt_nexthop &&
	rp->rp_hop_count < THRESHOLD1 && gw1_load < default_rt->rt_load)
    







       /* ||   // new GW & newer route
	  (rp->rp_dst != default_rt->rt_nexthop &&
	  ih->daddr() != index && ih->daddr() != IP_BROADCAST 
	  && ih->ttl_ != 0)*/ //RREP_I will be forwarded 
       ) {
      
      thisnode->set_base_stn(rp->rp_dst);
      rt_update(default_rt, rp->rp_dst_seqno, rp->rp_hop_count,
		rp->rp_dst, CURRENT_TIME + rp->rp_lifetime,gw1_load);//ly修改
      
      /*
	2a. Update the route to the fixed node.
      */
      aodv_rt_entry *rtn, *this_rt;
      for(this_rt = rtable.head(); this_rt; this_rt = rtn) {
	rtn = this_rt->rt_link.le_next;
	if(this_rt->rt_nexthop == DEFAULT) {
	  rt_update(this_rt, rp->rp_dst_seqno, rp->rp_hop_count,
		    DEFAULT, CURRENT_TIME + rp->rp_lifetime,gw1_load);
	}
      }
      
#ifdef DEQUE_AFTER_RREP_I      
      /*
	2b. If the route is a route to a fixed node and a valid default route 
	exists and there are packets in the sendbuffer waiting, forward them.
      */
      
      aodv_rt_entry *fn_rt = find_fn_entry();
      if(ih->daddr() == index && fn_rt && fn_rt->rt_flags == RTF_UP && 
	 find_send_entry(fn_rt)->rt_flags == RTF_UP) {
	assert(default_rt->rt_hops != INFINITY2);
	int j = 0;
	Packet *buf_pkt;
	while((buf_pkt = rqueue.deque(fn_rt->rt_dst))) {
	  ++j;
	  forward(find_send_entry(fn_rt), buf_pkt, delay);
	  delay += ARP_DELAY;
	}
#ifdef DEBUG
	if(j > 0) {
	  fprintf(stderr,"%d - %d packet(s) destined to %d emptied from "
		  "sendbuffer at %f\n\tGW is %d (recvReply)\n", index, j, 
		  fn_rt->rt_dst, CURRENT_TIME, default_rt->rt_nexthop);
	  printf("%d - %d packet(s) destined to %d emptied from "
		 "sendbuffer at %f, GW is %d (recvReply)\n", index, j, 
		 fn_rt->rt_dst, CURRENT_TIME, default_rt->rt_nexthop);
	}
#endif
      }
#endif //DEQUE_AFTER_RREP_I
      
    }
    else {
      suppress_reply = 1;
    }
  } /*if RREP_I*/
  //*************************************************************************//
#ifdef DEBUG
  if(suppress_reply && rp->rp_flags != RREP_IFLAG) {
    fprintf(stderr, "%d - received RREP, suppress RREP\n", index);
  }
#endif // DEBUG
  
  /*
   * 1. If reply is for me, discard it.
   */
  //My modification***********************************//
  /*
    RREP_Is should not be suppressed!
  */
  //**************************************************//
  if(ih->daddr() == index || (suppress_reply && rp->rp_flags != RREP_IFLAG)) {
    Packet::free(p);
  }
  /*
   * 2. Otherwise, forward the Route Reply.
   */
  else {
    // Find the rt entry
    aodv_rt_entry *rt0 = rtable.rt_lookup(ih->daddr());
    /*
      2a. If the rt is up, forward
    */
    if(rt0 && (rt0->rt_hops != INFINITY2)) {
      assert (rt0->rt_flags == RTF_UP);
      rp->rp_hop_count += 1;
      //My modification**************//
      //Bug fix from Riadh
      //rp->rp_src = index;
      ih->saddr() = index;
      //****************************//
      forward(rt0, p, NO_DELAY);
      // Insert the nexthop towards the RREQ source to 
      // the precursor list of the RREQ destination
      rt->pc_insert(rt0->rt_nexthop); // nexthop to RREQ source
      
    }
    //My modifications*******************************************************//
    /*
      Implementing hybrid gateway discovery method...
      
      2b. If RREP_I that should be rebroadcasted, rebroadcast it.
    */
    else if(rp->rp_flags == RREP_IFLAG && 
	    ih->daddr() == (nsaddr_t) IP_BROADCAST) {
      //Broadcast the RREP_I to other mobile nodes (your neighbors)
      aodv_rt_entry *default_rt = rtable.rt_lookup(DEFAULT);
      ih->saddr() = index;
      if(rp->rp_hop_count != INFINITY2 && default_rt &&
	 rp->rp_hop_count > default_rt->rt_hops) {
	rp->rp_hop_count = default_rt->rt_hops + 1;
      }
      else {
	rp->rp_hop_count += 1;
      }
      
      struct hdr_cmn *ch = HDR_CMN(p);
      ch->prev_hop_ = index;
      //Randomize the sending of broadcast packets to reduce collisions 
      //(100 ms jitter)
      //forward((aodv_rt_entry*) 0, p, DELAY);
      forward((aodv_rt_entry*) 0, p, 0.01 * Random::uniform());
      //Precursor list??
    }
    //***********************************************************************//
    /*
      2c. I don't know how to forward .. drop the reply.
    */
    else {
#ifdef DEBUG
      fprintf(stderr, "%d - dropping RREP\n", index);
#endif // DEBUG
      drop(p, DROP_RTR_NO_ROUTE);
    }
  }
}


void
AODV::recvError(Packet *p) {
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_aodv_error *re = HDR_AODV_ERROR(p);
  aodv_rt_entry *rt;
  u_int8_t i;
  Packet *rerr = Packet::alloc();
  struct hdr_aodv_error *nre = HDR_AODV_ERROR(rerr);
  nre->DestCount = 0;
  
  //My modification******************************************//
  /*
    I added some conditions to the if statement below.
  */
  //Find route to fixed node
  aodv_rt_entry *default_rt = rtable.rt_lookup(DEFAULT);
  aodv_rt_entry *gw_rt;
  if(default_rt && default_rt->rt_flags == RTF_UP)
    gw_rt = rtable.rt_lookup(default_rt->rt_nexthop);
  //*********************************************************//
  for(i=0; i<re->DestCount; i++) {
    // For each unreachable destination
    rt = rtable.rt_lookup(re->unreachable_dst[i]);
    
    if((rt && (rt->rt_hops != INFINITY2) && (rt->rt_nexthop == ih->saddr()) &&
	(rt->rt_seqno <= re->unreachable_dst_seqno[i])) ||
       
       (rt && (rt->rt_hops != INFINITY2) && (rt->rt_nexthop == ih->saddr()) &&
	(rt->rt_seqno <= re->unreachable_dst_seqno[i]) && default_rt &&
	rt->rt_dst == default_rt->rt_nexthop) ) {
      assert(rt->rt_flags == RTF_UP);
      assert((rt->rt_seqno%2) == 0); // is the seqno even?
      
#ifdef DEBUG
      fprintf(stderr, "%d - receiving RERR; src=%d, dst=%d, unr=%d, nh=%d\n",
	      index, ih->saddr(), rt->rt_dst, re->unreachable_dst[i], 
	      rt->rt_nexthop);
#endif // DEBUG
      rt->rt_seqno = re->unreachable_dst_seqno[i];
      rt_down(rt);
      
      // Not sure whether this is the right thing to do
      Packet *pkt;
      while((pkt = ifqueue->filter(ih->saddr()))) {
	drop(pkt, DROP_RTR_MAC_CALLBACK);
      }
      
      // if precursor list non-empty add to RERR and delete the precursor list
      if(!rt->pc_empty()) {
	nre->unreachable_dst[nre->DestCount] = rt->rt_dst;
	nre->unreachable_dst_seqno[nre->DestCount] = rt->rt_seqno;
	nre->DestCount += 1;
	rt->pc_delete();
	//My modification*************************************//
	if(default_rt && rt->rt_dst == default_rt->rt_nexthop) {
	  nre->unreachable_dst[nre->DestCount] = default_rt->rt_dst;
	  nre->unreachable_dst_seqno[nre->DestCount] = default_rt->rt_seqno;
	  nre->DestCount += 1; 
	}
	//****************************************************//
      }
      
      //My modification******************************************//
      //If this is a route to a GW, bring down default route.
      if(default_rt && rt->rt_dst == default_rt->rt_nexthop) {
	//Seqno not correct, but it seems it's ok...
	//default_rt->rt_seqno = re->unreachable_dst_seqno[i];
	rt_down(default_rt);
      }
      //*********************************************************//
    
    } /*if*/
  } /*for*/
  
  //My modification**********************************************************//
  /*
    If this is a route to a GW (any GW!), bring down default route.
    
    MN_S sends RREQ_I which is received by gwA and gwB. Both gateways send 
    RREP_Is. RREP_I from gwA is dropped (e.g. due to collision or by ARP). So 
    MN_I receives RREP_I from gwB. MN_I does not update its default route 
    because it has already a route to gwA and the route to gwB is not shorter. 
    MN_I forwards RREP_I to MN_S. MN_S think its default GW is gwB when sending
    packets to MN_I, but in fact MN_I sends the packets to gwA! So when the 
    link between MN_I and gwA breaks and MN_I sends a RERR, MN_S doesn't care 
    about the RERR (although it should) because it is not the nexthop of one of
    the advertised unreachable destinations. Thus, MN_S doesn't bring down 
    default route and continues sending packets to MN_I which dropps the 
    packets (NRTE). The problem occurs when MN_I forwards a RREP_I for gwB 
    although its own default route is gwA!
    
    The root of this problem is that ARP dropps the RREP_I! It can be solved by
    introducing a queue in ARP. But maybe RREP_Is can be dropped in another 
    way besides by ARP? In that case we must solve the problem either like 
    below or by changing the gateway selection procedure.
    
    See recvReply!
  */
  if(default_rt && default_rt->rt_flags == RTF_UP && gw_rt && 
     ih->saddr() == gw_rt->rt_nexthop) {
    rt_down(default_rt);
  }
  //*************************************************************************//
  
  if(nre->DestCount > 0) {
#ifdef DEBUG
    fprintf(stderr, "%d - sending RERR\ttime=%f\n", index, CURRENT_TIME);
#endif // DEBUG
    sendError(rerr);
  }
  else {
    Packet::free(rerr);
  }
  
  Packet::free(p);
}



/* ======================================================================
   Packet Transmission Routines
   ===================================================================== */
void
AODV::forward(aodv_rt_entry *rt, Packet *p, double delay) {
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  
  if(ih->ttl_ == 0) {
    //My modification********************************************************//
#ifdef DEBUG
    if(gw_discovery != 0 && gw_discovery != 1) {
      fprintf(stderr, "%d - TTL=0 ==> drop packet\n", index);
    }
#endif // DEBUG
    //***********************************************************************//
    drop(p, DROP_RTR_TTL);
    return;
  }
 
  if(ch->ptype() != PT_AODV && ch->direction() == hdr_cmn::UP &&
     ((u_int32_t)ih->daddr() == IP_BROADCAST)
     || ((u_int32_t)ih->daddr() == here_.addr_)) {
    dmux_->recv(p,0);
    return;
  }
  
  if(rt) {
    assert(rt->rt_flags == RTF_UP);
   
    //My modification********************************************************//
    /*
      Update rt_expire for fixed node (FN) route and default route...
      That is, we have to update rt_expire for FN==>DEFAULT and DEFAULT==>GW!
      However, we have to update rt_expire for these entries only if they are
      going to be used. That's why we need the if statement, but the condition 
      is maybe not the best one...
    */
    rt->rt_expire = max(rt->rt_expire, CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT);
    
    aodv_rt_entry *default_rt = rtable.rt_lookup(DEFAULT);
    aodv_rt_entry *fn_rt = rtable.rt_lookup(ih->daddr());
    if(ih->daddr() != rt->rt_dst) {
      default_rt->rt_expire = max(default_rt->rt_expire, CURRENT_TIME + 
				  GWINFO_LIFETIME);
      fn_rt->rt_expire = max(fn_rt->rt_expire, CURRENT_TIME + GWINFO_LIFETIME);
    }
    //***********************************************************************//
    ch->next_hop_ = rt->rt_nexthop;
    ch->addr_type() = NS_AF_INET;
    ch->direction() = hdr_cmn::DOWN; //important: change the packet's direction
  }
  else { // if it is a broadcast packet
    assert(ch->ptype() == PT_AODV);
    assert(ih->daddr() == (nsaddr_t) IP_BROADCAST);
    ch->addr_type() = NS_AF_NONE;
    ch->direction() = hdr_cmn::DOWN; //important: change the packet's direction
  }
 
  if(ih->daddr() == (nsaddr_t) IP_BROADCAST) {
    // If it is a broadcast packet
    assert(rt == 0);
   
    //Jitter the sending of broadcast packets by 10 ms
    Scheduler::instance().schedule(target_, p, 0.01 * Random::uniform());
  }
  else { // Not a broadcast packet 
    if(delay > 0.0) {
      Scheduler::instance().schedule(target_, p, delay);
    }
    else {
      // Not a broadcast packet, no delay, send immediately
      Scheduler::instance().schedule(target_, p, 0.);
    }
  }
}


void
AODV::sendRequest(nsaddr_t dst, u_int8_t flag) {
  // Allocate a RREQ packet 
  Packet *p = Packet::alloc();
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_aodv_request *rq = HDR_AODV_REQUEST(p);
  aodv_rt_entry *rt = rtable.rt_lookup(dst);
  assert(rt);
  
  /*
   * Rate limit sending of route requests. We are very conservative about
   * sending out route requests. 
   */
  if(rt->rt_flags == RTF_UP) {
    assert(rt->rt_hops != INFINITY2);
    Packet::free((Packet *)p);
    return;
  }
  
  if(rt->rt_req_timeout > CURRENT_TIME) {
    Packet::free((Packet *)p);
    return;
  }
  
  
  /* rt_req_cnt is the no. of times we did network-wide broadcast RREQ_RETRIES
     is the maximum number we will allow before going to a long timeout.
  */
  //==> I added the second condition to treat RREQ_Is differently!
  if((rt->rt_req_cnt > RREQ_RETRIES) && (flag != RREQ_IFLAG)) {
    rt->rt_req_timeout = CURRENT_TIME + MAX_RREQ_TIMEOUT;
    rt->rt_req_cnt = 0;
    Packet *buf_pkt;
    while((buf_pkt = rqueue.deque(rt->rt_dst))) {
      drop(buf_pkt, DROP_RTR_NO_ROUTE);
    }
    Packet::free((Packet *)p);
    return;
  }
  
  //My modification**********************************************************//
  else if((rt->rt_req_cnt > RREQ_I_RETRIES) && (flag == RREQ_IFLAG)) {
    rt->rt_req_timeout = CURRENT_TIME + MAX_RREQ_TIMEOUT;
    rt->rt_req_cnt = 0;
    Packet *buf_pkt;
    while((buf_pkt = rqueue.deque(rt->rt_dst))) {
      drop(buf_pkt, DROP_RTR_NO_ROUTE);
    }
    Packet::free((Packet *)p);
    return;
  }
  //*************************************************************************//
  
  //My modification**********************************************************//
  aodv_rt_entry *default_rt = rtable.rt_lookup(DEFAULT);
  if(default_rt && (default_rt->rt_flags == RTF_UP) && (rt->rt_req_cnt >= 1)) {
    /*
      If the destination is a MN that I have been communicated with before, but
      which now is unreachable - i.e. it is not in the transmission range of
      ANY mobile node in this ad hoc network.
      
      HACK!?
    */
    if(rt && rt->rt_seqno != 0) {
      rt->rt_req_timeout = CURRENT_TIME + MAX_RREQ_TIMEOUT;
      rt->rt_req_cnt = 0;
      Packet *buf_pkt;
      int j = 0;
      while((buf_pkt = rqueue.deque(rt->rt_dst))) {
	++j;
	drop(buf_pkt, DROP_RTR_NO_ROUTE);
      }
      
      //For debugging
      if(j > 0) {
	fprintf(stderr, "\n===============================================\n");
	fprintf(stderr, "%d - UNREACHABLE MOBILE NODE %d! %d packets dropped "
		"at %f\n" , index, rt->rt_dst, j, CURRENT_TIME);
	fprintf(stderr, "===============================================\n\n");
      }

      Packet::free((Packet *)p);
      return;
    }
    
    /*
      Default Route exists. Network-wide search has been done once but no RREP
      received. Assume the destination node is a fixed node (FN) on Internet. 
      1. Update your route entry in the routing table to FN. 
      2. Send the packets in sendbuffer to FN, via GW.
      Future packets (see rt_resolve) should be sent to FN.
      Only intermediate nodes, that don't have any valid route to the 
      destination, will enter this else-if because the sender will
      enter else-if(ih->saddr()==index). 
      We should come here only once, after network-wide search (and if rt goes
      down?).
      
      NOTE! Don't mix *rt (entry to FN) and *default_rt (entry to Default 
      Route)!
    */
    
    //Update your route entry to FN...
    rt_update(rt, default_rt->rt_seqno, default_rt->rt_hops, DEFAULT, 
	      default_rt->rt_expire,default_rt->rt_load);//ly修改
    
    //Send all packets queued in the sendbuffer
    double delay = 0.0;
    int j = 0;
    while((p = rqueue.deque(rt->rt_dst))) {
      // Delay them a little to help ARP. Otherwise ARP may drop packets.
      
      ++j;
      //==> I've changed "rt" to "find_send_entry(rt)".
      forward(find_send_entry(rt), p, delay);
      delay += ARP_DELAY;
    }
#ifdef DEBUG
    if(j > 0 && rt->rt_nexthop == DEFAULT) {
      fprintf(stderr,"%d - %d packet(s) destined to %d emptied from sendbuffer"
	      " at %f\n\tGW is %d (sendRequest)\n", index, j, rt->rt_dst, 
	      CURRENT_TIME, default_rt->rt_nexthop);
    }
    else if(j > 0) {
      fprintf(stderr,"%d - %d packet(s) destined to %d emptied from sendbuffer"
	      " at %f\n (sendRequest)\n", index, j, rt->rt_dst, CURRENT_TIME);
    }
#endif
    return;
  }
  //*************************************************************************//
  
  // Determine the TTL to be used this time. Dynamic TTL evaluation - SRD
  rt->rt_req_last_ttl = max(rt->rt_req_last_ttl, rt->rt_last_hop_count);
  
  if(0 == rt->rt_req_last_ttl) {
    // first time query broadcast
    ih->ttl_ = TTL_START;
  }
  else {
    // expanding ring search
    if(rt->rt_req_last_ttl < TTL_THRESHOLD)
      ih->ttl_ = rt->rt_req_last_ttl + TTL_INCREMENT;
    else {
      // network-wide broadcast
      ih->ttl_ = NETWORK_DIAMETER;
      rt->rt_req_cnt += 1;
    }
  }
  
  // remember the TTL used  for the next time
  rt->rt_req_last_ttl = ih->ttl_;
 
  // PerHopTime is the roundtrip time per hop for route requests.
  // The factor 2.0 is just to be safe .. SRD 5/22/99
  // Also note that we are making timeouts to be larger if we have 
  // done network wide broadcast before. 
  rt->rt_req_timeout = 2.0 * (double) ih->ttl_ * PerHopTime(rt); 
  if(rt->rt_req_cnt > 0)
    rt->rt_req_timeout *= rt->rt_req_cnt;
  rt->rt_req_timeout += CURRENT_TIME;
  
  // Don't let the timeout to be too large, however .. SRD 6/8/99
  if(rt->rt_req_timeout > CURRENT_TIME + MAX_RREQ_TIMEOUT)
    rt->rt_req_timeout = CURRENT_TIME + MAX_RREQ_TIMEOUT;
  rt->rt_expire = 0;
  
#ifdef DEBUG
  if(flag==RREQ_IFLAG) {
    fprintf(stderr, 
	    "(%2d )\n%d - sending RREQ_I for %d at %f, timeout=%f\t TTL=%d\n",
	    ++route_request, index, rt->rt_dst, CURRENT_TIME, 
	    rt->rt_req_timeout, ih->ttl_);
  }
  else {
    fprintf(stderr, 
	    "(%2d )\n%d - sending RREQ for %d at %f, timeout=%f\t TTL=%d\n",
	    ++route_request, index, rt->rt_dst, CURRENT_TIME, 
	    rt->rt_req_timeout, ih->ttl_);
  }
#endif // DEBUG
  
  // Fill out the RREQ packet 
  // ch->uid() = 0;
  ch->ptype() = PT_AODV;
  ch->size() = IP_HDR_LEN + rq->size();
  ch->iface() = -2;
  ch->error() = 0;
  ch->addr_type() = NS_AF_NONE;
  ch->prev_hop_ = index;          // AODV hack
 
  ih->saddr() = index;
  ih->daddr() = IP_BROADCAST;
  ih->sport() = RT_PORT;
  ih->dport() = RT_PORT;
  
  // Fill up some more fields 
  rq->rq_type = AODVTYPE_RREQ;
  rq->rq_hop_count = 1;
  rq->rq_bcast_id = bid++;
  rq->rq_dst = dst;
  rq->rq_dst_seqno = (rt ? rt->rt_seqno : 0);
  rq->rq_src = index;
  seqno += 2;
  assert ((seqno%2) == 0);
  rq->rq_src_seqno = seqno;
  rq->rq_timestamp = CURRENT_TIME;
  //My modification*******************//
  rq->rq_flags = flag;
  //**********************************//
  Scheduler::instance().schedule(target_, p, 0.);
}

void
AODV::sendReply(nsaddr_t ipdst, u_int32_t hop_count, nsaddr_t rpdst,
                u_int32_t rpseq, u_int32_t lifetime, double timestamp, 
		u_int8_t flag,u_int8_t ifqlen) {
  Packet *p = Packet::alloc();
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_aodv_reply *rp = HDR_AODV_REPLY(p);
  aodv_rt_entry *rt = rtable.rt_lookup(ipdst);
  assert(rt);
  rp->ifqlen = ifqlen;//ly修改
  rp->rp_type = AODVTYPE_RREP;
  //My modification*******************//
  rp->rp_flags = flag;
  //**********************************//
  rp->rp_hop_count = hop_count;
  rp->rp_dst = rpdst;
  rp->rp_dst_seqno = rpseq;
  //My modification*******************//
  //Bug fix from Riadh
  //rp->rp_src = index;
  rp->rp_src = ipdst;
  //**********************************//
  rp->rp_lifetime = lifetime;
  rp->rp_timestamp = timestamp;
  
  // ch->uid() = 0;
  ch->ptype() = PT_AODV;
  ch->size() = IP_HDR_LEN + rp->size();
  ch->iface() = -2;
  ch->error() = 0;
  ch->addr_type() = NS_AF_INET;
  ch->next_hop_ = rt->rt_nexthop;
  ch->prev_hop_ = index;          // AODV hack
  ch->direction() = hdr_cmn::DOWN;
  
  ih->saddr() = index;
  ih->daddr() = ipdst;
  ih->sport() = RT_PORT;
  ih->dport() = RT_PORT;
  ih->ttl_ = NETWORK_DIAMETER;
  
  //My modification**********************************************************//
  /*
    A mobile node (MN_S) that broadcasts a RREQ for another mobile node (MN_D)
    may receive both a RREP (from MN_D) and a RREP_I (from GW). If MN_D replies
    before GW, the RREP of MN_D can be dropped by ARP since ARP buffers only 
    one packet. Therefore, RREP_Is are delayed.
  */
  if(rp->rp_flags == RREP_IFLAG) {
    Scheduler::instance().schedule(target_, p, ARP_DELAY + 
				   0.001 * Random::uniform());
  }
  else {
    Scheduler::instance().schedule(target_, p, 0.);
  }
  //*************************************************************************//
}

void
AODV::sendError(Packet *p, bool jitter) {
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_aodv_error *re = HDR_AODV_ERROR(p);
  
  re->re_type = AODVTYPE_RERR;
  //re->reserved[0] = 0x00; re->reserved[1] = 0x00;
  // DestCount and list of unreachable destinations are already filled
  
  // ch->uid() = 0;
  ch->ptype() = PT_AODV;
  ch->size() = IP_HDR_LEN + re->size();
  ch->iface() = -2;
  ch->error() = 0;
  ch->addr_type() = NS_AF_NONE;
  ch->next_hop_ = 0;
  ch->prev_hop_ = index;             // AODV hack
  ch->direction() = hdr_cmn::DOWN;   //important: change the packet's direction
  
  ih->saddr() = index;
  ih->daddr() = IP_BROADCAST;
  ih->sport() = RT_PORT;
  ih->dport() = RT_PORT;
  ih->ttl_ = 1;
  
  // Do we need any jitter? Yes
  if(jitter) {
    Scheduler::instance().schedule(target_, p, 0.01*Random::uniform());
  }
  else {
    Scheduler::instance().schedule(target_, p, 0.0);
  }
}



/* ======================================================================
   Neighbor Management Functions
   ===================================================================== */
void
AODV::sendHello() {
  Packet *p = Packet::alloc();
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_aodv_reply *rh = HDR_AODV_REPLY(p);
  
#ifdef DEBUG
  fprintf(stderr, "sending Hello from %d at %.2f\n", index, CURRENT_TIME);
#endif // DEBUG
  
  rh->rp_type = AODVTYPE_HELLO;
  //rh->rp_flags = 0x00;
  rh->rp_hop_count = 1;
  rh->rp_dst = index;
  rh->rp_dst_seqno = seqno;
  rh->rp_lifetime = (1 + ALLOWED_HELLO_LOSS) * HELLO_INTERVAL;
  
  // ch->uid() = 0;
  ch->ptype() = PT_AODV;
  ch->size() = IP_HDR_LEN + rh->size();
  ch->iface() = -2;
  ch->error() = 0;
  ch->addr_type() = NS_AF_NONE;
  ch->prev_hop_ = index;          // AODV hack
  
  ih->saddr() = index;
  ih->daddr() = IP_BROADCAST;
  ih->sport() = RT_PORT;
  ih->dport() = RT_PORT;
  ih->ttl_ = 1;
  
  Scheduler::instance().schedule(target_, p, 0.0);
}

void
AODV::recvHello(Packet *p) {
  //struct hdr_ip *ih = HDR_IP(p);
  struct hdr_aodv_reply *rp = HDR_AODV_REPLY(p);
  AODV_Neighbor *nb;
  
  nb = nb_lookup(rp->rp_dst);
  if(nb == 0) {
    nb_insert(rp->rp_dst);
  }
  else {
    nb->nb_expire = CURRENT_TIME + (1.5 * ALLOWED_HELLO_LOSS * HELLO_INTERVAL);
  }
  
  Packet::free(p);
}


//My modification************************************************************//
/*
  Implementing proactive gateway discovery method...
*/
void
AODV::sendAdvertisement(int ttl) {
  if(index != thisnode->base_stn()) {
    return;
  }
  
  Packet *p = Packet::alloc();
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_aodv_advertisement *ad = HDR_AODV_ADVERTISEMENT(p);

  ad->ad_type = AODVTYPE_ADVERTISEMENT;
  //ad->ad_flags = 0x00;
  ad->ad_hop_count = 1;
  //ad->ad_dst = index; Is not necessary...
  //Not 100 % sure if seqno should be increased like this...
  seqno++;
  if(seqno%2) seqno++;
  ad->ad_dst_seqno = seqno;
  ad->ad_src = index;
  ad->ad_lifetime = (1 + ALLOWED_HELLO_LOSS) * (u_int32_t) 
    ADVERTISEMENT_INTERVAL;
  ad->ad_bcast_id = ad_bid++;
    
  // ch->uid() = 0;
  ch->ptype() = PT_AODV;
  ch->size() = IP_HDR_LEN + ad->size();
  ch->iface() = -2;
  ch->error() = 0;
  ch->addr_type() = NS_AF_NONE;
  ch->prev_hop_ = index;          // AODV hack
  
  ih->saddr() = index;
  ih->daddr() = IP_BROADCAST;
  ih->sport() = RT_PORT;
  ih->dport() = RT_PORT;
  //In the hybrid method TTL = ADVERTISEMENT_ZONE
  //In the proactive method TTL = NETWORK_DIAMETER
  ih->ttl_ = ttl;
  
  Scheduler::instance().schedule(target_, p, 0.0);
}


/*
  Implementing proactive gateway discovery method...
*/
void
AODV::recvAdvertisement(Packet *p) {
  struct hdr_aodv_advertisement *ad = HDR_AODV_ADVERTISEMENT(p);
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_cmn *ch = HDR_CMN(p);
 
  /*
   * Drop if I'm GW
   */
  if(index == thisnode->base_stn()) {
    Packet::free(p);
    return;
  }
 
  /*
    1. ADD OR UPDATE ROUTE TO GW
  */
  /*
    NOTE! ad->ad_src corresponds to rp->rp_dst! Both are the address of the
    gateway!
  */
  aodv_rt_entry *rt = rtable.rt_lookup(ad->ad_src);
  if(rt == 0) {
    rt = rtable.rt_add(ad->ad_src);
  }
  
  if((rt->rt_flags != RTF_UP) ||             // no route before
     (ad->ad_dst_seqno > rt->rt_seqno) ||    // newer route 
     ((ad->ad_dst_seqno == rt->rt_seqno) &&  
      (ad->ad_hop_count < rt->rt_hops)) ) {  // shorter or equal route
    
    // Update the rt entry
    rt_update(rt, ad->ad_dst_seqno, ad->ad_hop_count, ch->prev_hop_, 
	      max(rt->rt_expire, CURRENT_TIME + ad->ad_lifetime),0);
  }
  
  /*
    2. ADD OR UPDATE DEFAULT ROUTE
  */
  /*
    Gateway Selection
    1. If Default Route does not exist: Create and update route
    2. If Default Route already exists: Update route if number of hops to the
    new gateway is less than the number of hops to the current gateway
  */
  aodv_rt_entry *default_rt = rtable.rt_lookup(DEFAULT);
  if(default_rt == 0) {
    default_rt = rtable.rt_add(DEFAULT);
  }
  
  if((default_rt->rt_flags != RTF_UP) ||            // no route before
      
     (ad->ad_src == default_rt->rt_nexthop &&
      ad->ad_dst_seqno > default_rt->rt_seqno) ||   // newer route 
     
     (ad->ad_src == default_rt->rt_nexthop &&
      ad->ad_dst_seqno == default_rt->rt_seqno &&   // shorter route
      ad->ad_hop_count < default_rt->rt_hops) ||
     
     (ad->ad_src != default_rt->rt_nexthop &&
      ad->ad_hop_count < default_rt->rt_hops) ) {   // new GW & shorter route
    
    thisnode->set_base_stn(ad->ad_src);
    
    // Update the default_rt entry
    rt_update(default_rt, ad->ad_dst_seqno, ad->ad_hop_count, ad->ad_src,
	      max(default_rt->rt_expire, CURRENT_TIME + ad->ad_lifetime),0);
  }
  
  /*
   * 3. DETERMINE WHETHER THE ADVERTISEMENT SHOULD BE DROPPED OR FORWARDED
   */
  if(id_lookup(ad->ad_src, ad->ad_bcast_id) == ID_FOUND) {
    Packet::free(p);
  }
  else {
    //Cache the broadcast ID
    id_insert(ad->ad_src, ad->ad_bcast_id);
  
   
    //Forward the advertisement to other mobile nodes (your neighbors)
    ih->saddr() = index;
    ad->ad_hop_count += 1;
    
    ch->prev_hop_ = index;
    //Randomize the sending of broadcast packets to reduce collisions 
    //(100 ms jitter)
    //forward((aodv_rt_entry*) 0, p, DELAY);
    forward((aodv_rt_entry*) 0, p, 0.01 * Random::uniform());
  }
}
//***************************************************************************//


void
AODV::nb_insert(nsaddr_t id) {
  AODV_Neighbor *nb = new AODV_Neighbor(id);
  
  assert(nb);
  nb->nb_expire = CURRENT_TIME +
    (1.5 * ALLOWED_HELLO_LOSS * HELLO_INTERVAL);
  LIST_INSERT_HEAD(&nbhead, nb, nb_link);
  seqno += 2;             // set of neighbors changed
  assert ((seqno%2) == 0);
}


AODV_Neighbor*
AODV::nb_lookup(nsaddr_t id) {
  AODV_Neighbor *nb = nbhead.lh_first;
  
  for(; nb; nb = nb->nb_link.le_next) {
    if(nb->nb_addr == id) break;
  }
  return nb;
}


/*
 * Called when we receive *explicit* notification that a Neighbor is no longer
 * reachable.
 */
void
AODV::nb_delete(nsaddr_t id) {
  AODV_Neighbor *nb = nbhead.lh_first;
  
  log_link_del(id);
  seqno += 2;     // Set of neighbors changed
  assert ((seqno%2) == 0);
  
  for(; nb; nb = nb->nb_link.le_next) {
    if(nb->nb_addr == id) {
      LIST_REMOVE(nb,nb_link);
      delete nb;
      break;
    }
  }
  handle_link_failure(id);
}


/*
 * Purges all timed-out Neighbor Entries - runs every HELLO_INTERVAL * 1.5
 * seconds.
 */
void
AODV::nb_purge() {
  AODV_Neighbor *nb = nbhead.lh_first;
  AODV_Neighbor *nbn;
  double now = CURRENT_TIME;
  
  for(; nb; nb = nbn) {
    nbn = nb->nb_link.le_next;
    if(nb->nb_expire <= now) {
      nb_delete(nb->nb_addr);
    }
  }
}
