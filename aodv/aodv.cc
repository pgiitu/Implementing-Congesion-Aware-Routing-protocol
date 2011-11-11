/*
Copyright (c) 1997, 1998 Carnegie Mellon University.  All Rights
Reserved. 

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The AODV code developed by the CMU/MONARCH group was optimized and tuned by Samir Das and Mahesh Marina, University of Cincinnati. The work was partially done in Sun Microsystems. Modified for gratuitous replies by Anant Utgikar, 09/16/02.

*/

//#include <ip.h>
#include <iostream>
#include <aodv/aodv.h>
#include <aodv/aodv_packet.h>
#include <random.h>
#include <cmu-trace.h>
#include <iostream>
//#include <energy-model.h>

#define max(a,b)        ( (a) > (b) ? (a) : (b) )
#define CURRENT_TIME    Scheduler::instance().clock()

//#define DEBUG
//#define ERROR
//-----------------------Prateek---------------------------------
#define infinity 30000
//--------------------***Prateek***------------------------------
#ifdef DEBUG
static int extra_route_reply = 0;
static int limit_route_request = 0;
static int route_request = 0;
#endif


/*
  TCL Hooks
*/


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
//-----------------------Prateek--------------------
	if(strcmp(argv[1], "start_mesh") == 0) {
		//cout<<"setting the priority";		
		send_mesh_request();
      		return TCL_OK;		
	}	
	if(strcmp(argv[1], "high_priority") == 0) {
		//cout<<"setting the priority";		
		priority=1;
		//on_conzone=true;
		//send_conzone_request();
      		return TCL_OK;		
	}	
	if(strcmp(argv[1], "print_mesh") == 0) {
	
		//print_all_lists();
	if(on_conzone==true)
	{
		cout<<index<<"\n";
	}
		//print_off_parent();
		//print_on_parent();
	      return TCL_OK;		
	}	
//-----------------------***Prateek***--------------------

    
    if(strncasecmp(argv[1], "start", 2) == 0) {
      btimer.handle((Event*) 0);

#ifndef AODV_LINK_LAYER_DETECTION
      htimer.handle((Event*) 0);
      ntimer.handle((Event*) 0);
#endif // LINK LAYER DETECTION

      rtimer.handle((Event*) 0);
      return TCL_OK;
     }               
  }
  else if(argc == 3) {
    if(strcmp(argv[1], "index") == 0) {
      index = atoi(argv[2]);
      return TCL_OK;
    }

    else if(strcmp(argv[1], "log-target") == 0 || strcmp(argv[1], "tracetarget") == 0) {
      logtarget = (Trace*) TclObject::lookup(argv[2]);
      if(logtarget == 0)
	return TCL_ERROR;
      return TCL_OK;
    }
    else if(strcmp(argv[1], "drop-target") == 0) {
    int stat = rqueue.command(argc,argv);
      if (stat != TCL_OK) return stat;
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

/* 
   Constructor
*/
//----------------------- constructor for initialising the various variables for the node following the protocol
AODV::AODV(nsaddr_t id) : Agent(PT_AODV),
			  btimer(this), htimer(this), ntimer(this), 
			  rtimer(this), lrtimer(this), rqueue() {
 
 //-----------------Prateek---------------------------------------------------------
 bind("priority",&priority); //binding the priority in TCL and C++     
 bind("mesh",&mesh);
 //-----------------***Prateek***---------------------------------------------------------
                
  index = id;
  seqno = 2;
  bid = 1;
d_mssgs=0;
//(jabran)
threshold=1;
	//priority=0;
conzone_started=false;
on_conzone=false;
  LIST_INIT(&nbhead);
  LIST_INIT(&bihead);
//-----------------------Prateek----------------------------------------------------------------------------------------------
	depth=infinity;	
	//priority=0;
	LIST_INIT(&on_parent);
	LIST_INIT(&on_sibling);
	LIST_INIT(&off_parent);
	LIST_INIT(&off_sibling);
	LIST_INIT(&off_children);
//-----------------------***Prateek***-------------------------------------------------------------------------------------
  logtarget = 0;
  ifqueue = 0;
}

/*
  Timers
*/

void
BroadcastTimer::handle(Event*) {
  agent->id_purge();
  Scheduler::instance().schedule(this, &intr, BCAST_ID_SAVE);
}

void
HelloTimer::handle(Event*) {
   agent->sendHello();
   double interval = MinHelloInterval + 
                 ((MaxHelloInterval - MinHelloInterval) * Random::uniform());
   assert(interval >= 0);
   Scheduler::instance().schedule(this, &intr, interval);
}

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
	
    if (rt && rt->rt_flags != RTF_UP) {
    // route is yet to be repaired
    // I will be conservative and bring down the route
    // and send route errors upstream.
    /* The following assert fails, not sure why */
    /* assert (rt->rt_flags == RTF_IN_REPAIR); */
		
      //rt->rt_seqno++;
      agent->rt_down(rt);
      // send RERR
#ifdef DEBUG
      fprintf(stderr,"Node %d: Dst - %d, failed local repair\n",index, rt->rt_dst);
#endif      
    }
    Packet::free((Packet *)p);
}


/*
   Broadcast ID Management  Functions
*/


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
   if ((b->src == id) && (b->id == bid))
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

/*
  Helper Functions
*/

double
AODV::PerHopTime(aodv_rt_entry *rt) {
int num_non_zero = 0, i;
double total_latency = 0.0;

 if (!rt)
   return ((double) NODE_TRAVERSAL_TIME );
	
 for (i=0; i < MAX_HISTORY; i++) {
   if (rt->rt_disc_latency[i] > 0.0) {
      num_non_zero++;
      total_latency += rt->rt_disc_latency[i];
   }
 }
 if (num_non_zero > 0)
   return(total_latency / (double) num_non_zero);
 else
   return((double) NODE_TRAVERSAL_TIME);

}

/*
  Link Failure Management Functions
*/

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

#ifndef AODV_LINK_LAYER_DETECTION
 drop(p, DROP_RTR_MAC_CALLBACK);
#else 

 /*
  * Non-data packets and Broadcast Packets can be dropped.
  */
  if(! DATA_PACKET(ch->ptype()) ||
     (u_int32_t) ih->daddr() == IP_BROADCAST) {
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


  if (ch->num_forwards() > rt->rt_hops) {
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

 re->DestCount = 0;
 for(rt = rtable.head(); rt; rt = rtn) {  // for each rt entry
   rtn = rt->rt_link.le_next; 
   if ((rt->rt_hops != INFINITY2) && (rt->rt_nexthop == id) ) {
     assert (rt->rt_flags == RTF_UP);
     assert((rt->rt_seqno%2) == 0);
     rt->rt_seqno++;
     re->unreachable_dst[re->DestCount] = rt->rt_dst;
     re->unreachable_dst_seqno[re->DestCount] = rt->rt_seqno;
#ifdef DEBUG
     fprintf(stderr, "%s(%f): %d\t(%d\t%u\t%d)\n", __FUNCTION__, CURRENT_TIME,
		     index, re->unreachable_dst[re->DestCount],
		     re->unreachable_dst_seqno[re->DestCount], rt->rt_nexthop);
#endif // DEBUG
     re->DestCount += 1;
     rt_down(rt);
   }
   // remove the lost neighbor from all the precursor lists
   rt->pc_delete(id);
 }   

 if (re->DestCount > 0) {
#ifdef DEBUG
   fprintf(stderr, "%s(%f): %d\tsending RERR...\n", __FUNCTION__, CURRENT_TIME, index);
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
  // Buffer the packet 
  rqueue.enque(p);

  // mark the route as under repair 
  rt->rt_flags = RTF_IN_REPAIR;

  sendRequest(rt->rt_dst);

  // set up a timer interrupt
  Scheduler::instance().schedule(&lrtimer, p->copy(), rt->rt_req_timeout);
}

void
AODV::rt_update(aodv_rt_entry *rt, u_int32_t seqnum, u_int16_t metric,
	       	nsaddr_t nexthop, double expire_time) {

     rt->rt_seqno = seqnum;
     rt->rt_hops = metric;
     rt->rt_flags = RTF_UP;
     rt->rt_nexthop = nexthop;
     rt->rt_expire = expire_time;
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

/*
  Route Handling Functions
*/


void
AODV::rt_resolve(Packet *p) {
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
aodv_rt_entry *rt;

 /*
  *  Set the transmit failure callback.  That
  *  won't change.
  */
 ch->xmit_failure_ = aodv_rt_failed_callback;
 ch->xmit_failure_data_ = (void*) this;
	rt = rtable.rt_lookup(ih->daddr());
 if(rt == 0) {
	  rt = rtable.rt_add(ih->daddr());
 }

 /*
  * If the route is up, forward the packet 
  */
	
 if(rt->rt_flags == RTF_UP) {
   assert(rt->rt_hops != INFINITY2);
   forward(rt, p, NO_DELAY);
 }
 /*
  *  if I am the source of the packet, then do a Route Request.
  */




	else if(ih->saddr() == index) {
   rqueue.enque(p);		//storing the packet in the queue to be able to route it when the route is available
   sendRequest(rt->rt_dst);
 }
 /*
  *	A local repair is in progress. Buffer the packet. 
  */
 else if (rt->rt_flags == RTF_IN_REPAIR) {
   rqueue.enque(p);
 }

 /*
  * I am trying to forward a packet for someone else to which
  * I don't have a route.
  */
 else {
 Packet *rerr = Packet::alloc();
 struct hdr_aodv_error *re = HDR_AODV_ERROR(rerr);
 /* 
  * For now, drop the packet and send error upstream.
  * Now the route errors are broadcast to upstream
  * neighbors - Mahesh 09/11/99
  */	
 
   assert (rt->rt_flags == RTF_DOWN);
   re->DestCount = 0;
   re->unreachable_dst[re->DestCount] = rt->rt_dst;
   re->unreachable_dst_seqno[re->DestCount] = rt->rt_seqno;
   re->DestCount += 1;
#ifdef DEBUG
   fprintf(stderr, "%s: sending RERR...\n", __FUNCTION__);
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

 for(rt = rtable.head(); rt; rt = rtn) {  // for each rt entry
   rtn = rt->rt_link.le_next;
   if ((rt->rt_flags == RTF_UP) && (rt->rt_expire < now)) {
   // if a valid route has expired, purge all packets from 
   // send buffer and invalidate the route.                    
	assert(rt->rt_hops != INFINITY2);
     while((p = rqueue.deque(rt->rt_dst))) {
#ifdef DEBUG
       fprintf(stderr, "%s: calling drop()\n",
                       __FUNCTION__);
#endif // DEBUG
       drop(p, DROP_RTR_NO_ROUTE);
     }
     rt->rt_seqno++;
     assert (rt->rt_seqno%2);
     rt_down(rt);
   }
   else if (rt->rt_flags == RTF_UP) {
   // If the route is not expired,
   // and there are packets in the sendbuffer waiting,
   // forward them. This should not be needed, but this extra 
   // check does no harm.
     assert(rt->rt_hops != INFINITY2);
     while((p = rqueue.deque(rt->rt_dst))) {
       forward (rt, p, delay);
       delay += ARP_DELAY;
     }
   } 
   else if (rqueue.find(rt->rt_dst))
   // If the route is down and 
   // if there is a packet for this destination waiting in
   // the sendbuffer, then send out route request. sendRequest
   // will check whether it is time to really send out request
   // or not.
   // This may not be crucial to do it here, as each generated 
   // packet will do a sendRequest anyway.

     sendRequest(rt->rt_dst);
   }

}

/*
  Packet Reception Routines
*/

void
AODV::recv(Packet *p, Handler*) {
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);

//cout<<"the value of priority is"<<priority<<"\n"; 
assert(initialized());
 //assert(p->incoming == 0);
 // XXXXX NOTE: use of incoming flag has been depracated; In order to track direction of pkt flow, direction_ in hdr_cmn is used instead. see packet.h for details.

 if(ch->ptype() == PT_AODV) {
   ih->ttl_ -= 1;
   recvAODV(p);
   return;
 }

//---------------------------------Prateek(we can handle the code for creating conzone)-----------------------------------
 

if((ih->saddr()==index)&&(priority==1)&&(conzone_started==false))//&&(on_conzone==true))  //checking the condition for the conzone starting mssg(on_conzone==true may not be true(doubt))
{
	rqueue.enque(p);		//storing the packet in the queue to be able to route it when the route is available      
	conzone_started=true;
	on_conzone=true;
	cout<<"starting conzone request \n";//<<ih->daddr();
	send_conzone_request(ih->daddr());
	return;
}
//else
//{

/***
if some data packet is received by a high priority node and the conzone fromation has been started then just try to route the data packet to according to the priority of the data. For that i have to add a value so as to have the piority of the data packet.
***/

/*
	if((ih->saddr()==index)&&(priority==1)&&(conzone_started==true)&&(on_conzone==true))
	{
		route_data(p);
	}
	else
	{
		if((priority==1)&&(on_conzone==true)&&(conzone_started==false))
		{
			rqueue.enque(p);  //just queue the packet			
		}
	}	
}
*/
//------------------------------------***Prateek***-----------------------------------------------


//--------------------------------***Prateek***-----------------------------------------------------------------------

/*
  *  Must be a packet I'm originating...
  */
if((ih->saddr() == index) && (ch->num_forwards() == 0)) {
 /*
  * Add the IP Header.  
  * TCP adds the IP header too, so to avoid setting it twice, we check if
  * this packet is not a TCP or ACK segment.
  */
  if (ch->ptype() != PT_TCP && ch->ptype() != PT_ACK) {
    ch->size() += IP_HDR_LEN;
  }
   // Added by Parag Dadhania && John Novatnack to handle broadcasting
  if ( (u_int32_t)ih->daddr() != IP_BROADCAST) {
    ih->ttl_ = NETWORK_DIAMETER;
  }
}
 /*
  *  I received a packet that I sent.  Probably
  *  a routing loop.
  */
else if(ih->saddr() == index) {
   drop(p, DROP_RTR_ROUTE_LOOP);
   return;
 }
 /*
  *  Packet I'm forwarding...
  */
 else {
 /*
  *  Check the TTL.  If it is zero, then discard.
  */
   if(--ih->ttl_ == 0) {
     drop(p, DROP_RTR_TTL);
     return;
   }
 }
// Added by Parag Dadhania && John Novatnack to handle broadcasting
 if ( (u_int32_t)ih->daddr() != IP_BROADCAST)
   rt_resolve(p);
 else
   forward((aodv_rt_entry*) 0, p, NO_DELAY);
}


void
AODV::recvAODV(Packet *p) {
 struct hdr_aodv *ah = HDR_AODV(p);

 assert(HDR_IP (p)->sport() == RT_PORT);
 assert(HDR_IP (p)->dport() == RT_PORT);

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
//-----------------------------------------------------------Prateek-----------------------------------------------------------------------------
 case AODVTYPE_RMESH:			// defining the case to handle when the node receive a Mesh Request packet
   recv_mesh_request(p);
   break;
case AODVTYPE_RCONZONE:
   recv_conzone_request(p);
   break;
//-----------------------------------------------------------***Prateek***-------------------------------------------------------
        
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
   * Drop if:
   *      - I'm the source
   *      - I recently heard this request.
   */

  if(rq->rq_src == index) {
#ifdef DEBUG
    fprintf(stderr, "%s: got my own REQUEST\n", __FUNCTION__);
#endif // DEBUG
    Packet::free(p);
    return;
  } 

 if (id_lookup(rq->rq_src, rq->rq_bcast_id)) {

#ifdef DEBUG
   fprintf(stderr, "%s: discarding request\n", __FUNCTION__);
#endif // DEBUG
 
   Packet::free(p);
   return;
 }

 /*
  * Cache the broadcast ID
  */
 id_insert(rq->rq_src, rq->rq_bcast_id);



 /* 
  * We are either going to forward the REQUEST or generate a
  * REPLY. Before we do anything, we make sure that the REVERSE
  * route is in the route table.
  */
 aodv_rt_entry *rt0; // rt0 is the reverse route 
   
   rt0 = rtable.rt_lookup(rq->rq_src);
   if(rt0 == 0) { /* if not in the route table */
   // create an entry for the reverse route.
     rt0 = rtable.rt_add(rq->rq_src);
   }
  
   rt0->rt_expire = max(rt0->rt_expire, (CURRENT_TIME + REV_ROUTE_LIFE));

   if ( (rq->rq_src_seqno > rt0->rt_seqno ) ||
    	((rq->rq_src_seqno == rt0->rt_seqno) && 
	 (rq->rq_hop_count < rt0->rt_hops)) ) {
   // If we have a fresher seq no. or lesser #hops for the 
   // same seq no., update the rt entry. Else don't bother.
rt_update(rt0, rq->rq_src_seqno, rq->rq_hop_count, ih->saddr(),
     	       max(rt0->rt_expire, (CURRENT_TIME + REV_ROUTE_LIFE)) );
     if (rt0->rt_req_timeout > 0.0) {
     // Reset the soft state and 
     // Set expiry time to CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT
     // This is because route is used in the forward direction,
     // but only sources get benefited by this change
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
     while ((buffered_pkt = rqueue.deque(rt0->rt_dst))) {
       if (rt0 && (rt0->rt_flags == RTF_UP)) {
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

 // First check if I am the destination ..

 if(rq->rq_dst == index) {

#ifdef DEBUG
   fprintf(stderr, "%d - %s: destination sending reply\n",
                   index, __FUNCTION__);
#endif // DEBUG

               
   // Just to be safe, I use the max. Somebody may have
   // incremented the dst seqno.
   seqno = max(seqno, rq->rq_dst_seqno)+1;
   if (seqno%2) seqno++;

   sendReply(rq->rq_src,           // IP Destination
             1,                    // Hop Count
             index,                // Dest IP Address
             seqno,                // Dest Sequence Num
             MY_ROUTE_TIMEOUT,     // Lifetime
             rq->rq_timestamp);    // timestamp
 
   Packet::free(p);
 }

 // I am not the destination, but I may have a fresh enough route.

 else if (rt && (rt->rt_hops != INFINITY2) && 
	  	(rt->rt_seqno >= rq->rq_dst_seqno) ) {

   //assert (rt->rt_flags == RTF_UP);
   assert(rq->rq_dst == rt->rt_dst);
   //assert ((rt->rt_seqno%2) == 0);	// is the seqno even?
   sendReply(rq->rq_src,
             rt->rt_hops + 1,
             rq->rq_dst,
             rt->rt_seqno,
	     (u_int32_t) (rt->rt_expire - CURRENT_TIME),
	     //             rt->rt_expire - CURRENT_TIME,
             rq->rq_timestamp);
   // Insert nexthops to RREQ source and RREQ destination in the
   // precursor lists of destination and source respectively
   rt->pc_insert(rt0->rt_nexthop); // nexthop to RREQ source
   rt0->pc_insert(rt->rt_nexthop); // nexthop to RREQ destination

#ifdef RREQ_GRAT_RREP  

   sendReply(rq->rq_dst,
             rq->rq_hop_count,
             rq->rq_src,
             rq->rq_src_seqno,
	     (u_int32_t) (rt->rt_expire - CURRENT_TIME),
	     //             rt->rt_expire - CURRENT_TIME,
             rq->rq_timestamp);
#endif
   
// TODO: send grat RREP to dst if G flag set in RREQ using rq->rq_src_seqno, rq->rq_hop_counT
   
// DONE: Included gratuitous replies to be sent as per IETF aodv draft specification. As of now, G flag has not been dynamically used and is always set or reset in aodv-packet.h --- Anant Utgikar, 09/16/02.

	Packet::free(p);
 }
 /*
  * Can't reply. So forward the  Route Request
  */
 else {				//--------Prateek(This is how we broadcast a packet)--------------------------------
   ih->saddr() = index;
   ih->daddr() = IP_BROADCAST;
   rq->rq_hop_count += 1;
   // Maximum sequence number seen en route
   if (rt) rq->rq_dst_seqno = max(rt->rt_seqno, rq->rq_dst_seqno);
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
 fprintf(stderr, "%d - %s: received a REPLY\n", index, __FUNCTION__);
#endif // DEBUG


 /*
  *  Got a reply. So reset the "soft state" maintained for 
  *  route requests in the request table. We don't really have
  *  have a separate request table. It is just a part of the
  *  routing table itself. 
  */
 // Note that rp_dst is the dest of the data packets, not the
 // the dest of the reply, which is the src of the data packets.

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

 if ( (rt->rt_seqno < rp->rp_dst_seqno) ||   // newer route 
      ((rt->rt_seqno == rp->rp_dst_seqno) &&  
       (rt->rt_hops > rp->rp_hop_count)) ) { // shorter or better route
	
  // Update the rt entry 
  rt_update(rt, rp->rp_dst_seqno, rp->rp_hop_count,
		rp->rp_src, CURRENT_TIME + rp->rp_lifetime);

  // reset the soft state
  rt->rt_req_cnt = 0;
  rt->rt_req_timeout = 0.0; 
  rt->rt_req_last_ttl = rp->rp_hop_count;
  
if (ih->daddr() == index) { // If I am the original source
  // Update the route discovery latency statistics
  // rp->rp_timestamp is the time of request origination
		
    rt->rt_disc_latency[(unsigned char)rt->hist_indx] = (CURRENT_TIME - rp->rp_timestamp)
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
  while((buf_pkt = rqueue.deque(rt->rt_dst))) {
    if(rt->rt_hops != INFINITY2) {
          assert (rt->rt_flags == RTF_UP);
    // Delay them a little to help ARP. Otherwise ARP 
    // may drop packets. -SRD 5/23/99
      forward(rt, buf_pkt, delay);
      delay += ARP_DELAY;
    }
  }
 }
 else {
  suppress_reply = 1;
 }

 /*
  * If reply is for me, discard it.
  */

if(ih->daddr() == index || suppress_reply) {
   Packet::free(p);
 }
 /*
  * Otherwise, forward the Route Reply.
  */
 else {
 // Find the rt entry
aodv_rt_entry *rt0 = rtable.rt_lookup(ih->daddr());
   // If the rt is up, forward
   if(rt0 && (rt0->rt_hops != INFINITY2)) {
        assert (rt0->rt_flags == RTF_UP);
     rp->rp_hop_count += 1;
     rp->rp_src = index;
     forward(rt0, p, NO_DELAY);
     // Insert the nexthop towards the RREQ source to 
     // the precursor list of the RREQ destination
     rt->pc_insert(rt0->rt_nexthop); // nexthop to RREQ source
     
   }
   else {
   // I don't know how to forward .. drop the reply. 
#ifdef DEBUG
     fprintf(stderr, "%s: dropping Route Reply\n", __FUNCTION__);
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

 for (i=0; i<re->DestCount; i++) {
 // For each unreachable destination
   rt = rtable.rt_lookup(re->unreachable_dst[i]);
   if ( rt && (rt->rt_hops != INFINITY2) &&
	(rt->rt_nexthop == ih->saddr()) &&
     	(rt->rt_seqno <= re->unreachable_dst_seqno[i]) ) {
	assert(rt->rt_flags == RTF_UP);
	assert((rt->rt_seqno%2) == 0); // is the seqno even?
#ifdef DEBUG
     fprintf(stderr, "%s(%f): %d\t(%d\t%u\t%d)\t(%d\t%u\t%d)\n", __FUNCTION__,CURRENT_TIME,
		     index, rt->rt_dst, rt->rt_seqno, rt->rt_nexthop,
		     re->unreachable_dst[i],re->unreachable_dst_seqno[i],
	             ih->saddr());
#endif // DEBUG
     	rt->rt_seqno = re->unreachable_dst_seqno[i];
     	rt_down(rt);

   // Not sure whether this is the right thing to do
   Packet *pkt;
	while((pkt = ifqueue->filter(ih->saddr()))) {
        	drop(pkt, DROP_RTR_MAC_CALLBACK);
     	}

     // if precursor list non-empty add to RERR and delete the precursor list
     	if (!rt->pc_empty()) {
     		nre->unreachable_dst[nre->DestCount] = rt->rt_dst;
     		nre->unreachable_dst_seqno[nre->DestCount] = rt->rt_seqno;
     		nre->DestCount += 1;
		rt->pc_delete();
     	}
   }
 } 

 if (nre->DestCount > 0) {
#ifdef DEBUG
   fprintf(stderr, "%s(%f): %d\t sending RERR...\n", __FUNCTION__, CURRENT_TIME, index);
#endif // DEBUG
   sendError(rerr);
 }
 else {
   Packet::free(rerr);
 }

 Packet::free(p);
}


/*
   Packet Transmission Routines
*/
void
AODV::forward(aodv_rt_entry *rt, Packet *p, double delay) {
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);

 if(ih->ttl_ == 0) {

#ifdef DEBUG
  fprintf(stderr, "%s: calling drop()\n", __PRETTY_FUNCTION__);
#endif // DEBUG
 
  drop(p, DROP_RTR_TTL);
  return;
 }

 if (ch->ptype() != PT_AODV && ch->direction() == hdr_cmn::UP &&
	((u_int32_t)ih->daddr() == IP_BROADCAST)
		|| (ih->daddr() == here_.addr_)) {
	dmux_->recv(p,0);
	return;
 }
//-----------Prateek---------(here is all we have to do forward(change the direction) the packet to the next hop)---------
 if (rt) {
   assert(rt->rt_flags == RTF_UP);
   rt->rt_expire = CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT;
   ch->next_hop_ = rt->rt_nexthop;
   ch->addr_type() = NS_AF_INET;
   ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
 }
 else { // if it is a broadcast packet
   // assert(ch->ptype() == PT_AODV); // maybe a diff pkt type like gaf
   assert(ih->daddr() == (nsaddr_t) IP_BROADCAST);
   ch->addr_type() = NS_AF_NONE;
   ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
 }

if (ih->daddr() == (nsaddr_t) IP_BROADCAST) {
 // If it is a broadcast packet
   assert(rt == 0);
   if (ch->ptype() == PT_AODV) {
     /*
      *  Jitter the sending of AODV broadcast packets by 10ms
      */
     Scheduler::instance().schedule(target_, p,			//prateek-------function for broadcasting a packet--------
      				   0.01 * Random::uniform());
   } else {
     Scheduler::instance().schedule(target_, p, 0.);  // No jitter
   }
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
//--------------------------------------------Prateek-----------------------------------------------------------------------
void AODV::send_mesh_request(void){                  // sending a packet to start the mesh formation
Packet *p=Packet::alloc();

struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_mesh_request *rq = HDR_MESH_REQUEST(p);

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

 depth=0;			//setting the depth of the node
 // Fill up some more fields. 
 rq->mesh_type = AODVTYPE_RMESH;
 rq->depth = 0;

 rq->mesh_bcast_id = bid++;
 rq->mesh_hop = index;
 rq->mesh_src = index;

// Scheduler::instance().schedule(target_, p, 0.);
     Scheduler::instance().schedule(target_, p,			//prateek-------function for broadcasting a packet--------
      				   0.01 * Random::uniform());
}

/*
*	defining the function to receive a mesh formation request
*
*/
void AODV::recv_mesh_request(Packet *p){

struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_mesh_request *rq = HDR_MESH_REQUEST(p);

if(rq->depth==infinity)			// CASE: when the node is not a part of the mesh
{
	depth=rq->depth+1;
	list_insert(1,rq->mesh_hop);
		
	   rq->mesh_hop=index;
	   rq->depth=rq->depth+1;

	   ih->saddr() = index;			// Doubt(why is this done)
	   ih->daddr() = IP_BROADCAST;
 
  	   ch->addr_type() = NS_AF_NONE;
   	   ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction

     /*
      *  Jitter the sending of mesh request broadcast packets by 10ms
      */
     Scheduler::instance().schedule(target_, p,			//prateek-------function for broadcasting a packet--------
      				   0.01 * Random::uniform());

	//Scheduler::instance().schedule(target_, p, 0.);    // DOUBT: broadcast the packet but i am nt sure whether it is correct or not
}
else
{
	if(rq->depth==depth)		//CASE: when the node receives a mesh formation request from a sibling
	{
		//cout<<"inserting in sibling list:"<<index<<"::::: "<<rq->mesh_hop<<"\n";			
		list_insert(2,rq->mesh_hop);
		Packet::free(p);            // discard the packet
		return;
	}
	else
	{
		if(rq->depth+1<depth)	//CASE: when the node has received a shorter path to Sink
		{
			list_purge(3);	// delete the list of children
			list_purge(1);		// delete the list of parent
			list_purge(2);	// delete the list of sibling
			depth=rq->depth+1;
			//cout<<"inserting in parent list:"<<index<<"::::: "<<rq->mesh_hop<<"\n";			
			list_insert(1,rq->mesh_hop); 			// insert the new parent in the list
			rq->depth++;
			rq->mesh_hop=index;		// setting the prev hop as its IpAddress(index)
	   		ih->saddr() = index;			// Doubt(why is this done)
	   		ih->daddr() = IP_BROADCAST;
 
  	   		ch->addr_type() = NS_AF_NONE;
   	   		ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
     		/*
     		 *  Jitter the sending of AODV broadcast packets by 10ms
     		 */
     		Scheduler::instance().schedule(target_, p,			//prateek-------function for broadcasting a packet--------
      				   0.01 * Random::uniform());

//			Scheduler::instance().schedule(target_, p, 0.);    // DOUBT: broadcast the packet but i am nt sure whether it is correct or not
			
		}
		else
		{
			if(rq->depth-1==depth)		//CASE: when the node has received the request from its child
			{
				//cout<<"inserting in child list:"<<index<<"::::: "<<rq->mesh_hop<<"\n";			
				list_insert(3,rq->mesh_hop);       // insert in the child list
				Packet::free(p);            			// discard the packet
				return;
			}
			else
			{
				if(rq->depth+1==depth)			
				{
					list_insert(1,rq->mesh_hop);	//CASE: adding multiple parents		
					Packet::free(p);
				}				
				if(rq->depth+1>depth)   //CASE: when the node has received a packet with a greater depth
				{
					Packet::free(p);            // just discard the packet
					return;
				}
			}
		}
	}
}

}
/*
*	defining the function to send a conzone formation request
*
*/
void AODV::send_conzone_request(nsaddr_t dst){
Packet *p=Packet::alloc();

struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_conzone_request *rq = HDR_CONZONE_REQUEST(p);

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

 // Fill up some more fields. 
 rq->conzone_type = AODVTYPE_RCONZONE;
 rq->depth = depth;
 rq->conzone_bcast_id = bid++;
 rq->conzone_hop = index;
 rq->conzone_dst=dst;
 rq->conzone_src = index;

// Scheduler::instance().schedule(target_, p, 0.);		// broadcast the packet(have to check the function)
     /*
      *  Jitter the sending of conzone request broadcast packets by 10ms
      */
     Scheduler::instance().schedule(target_, p,			//prateek-------function for broadcasting a packet--------
      				   0.01 * Random::uniform());
}

/*
*	defining the function to receive a conzone formation request
*
*/
void AODV::recv_conzone_request(Packet *p){

//cout<<"I HAVE RECEIVED A CONZONE PACKET::  "<<index<<"  and number of d_mssgs are::  "<<d_mssgs<<"\n";
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_conzone_request *rq = HDR_CONZONE_REQUEST(p);

	// if the recieving node is the required destination then handle the case here

if(rq->depth==depth+1) 	//if node receives the mssg from a children
{
	if(on_conzone==false)
	{
		//cout<<"from a children \n";
		d_mssgs++;		
		if(d_mssgs>threshold)
		{
			cout<<"node going on conzone \n";			
			on_conzone=true;
			if(rq->conzone_dst!=index)		// if the node is not the sink
			{
				rq->depth=depth;		//setting the depth
				rq->conzone_hop=index;		//setting the next/prev hop	 
				
		   		ih->saddr() = index;			// Doubt(why is this done)
		   		ih->daddr() = IP_BROADCAST;
				 
	  	   		ch->addr_type() = NS_AF_NONE;
	   	   		ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
				conzone_started=true;			     	
				/*
	      			*  Jitter the sending of AODV broadcast packets by 10ms
	      			*/
				//cout<<"BROADCASTING CONZONE REQUEST AGAIN 1\n";	     			
				Scheduler::instance().schedule(target_, p,0.01 * Random::uniform());  //prateek-------function 												for broadcasting a packet--------

				//Scheduler::instance().schedule(target_, p, 0.);// broadcast the packet(have to check d function)
			}
			else
			{
				on_conzone=true;				
				//cout<<"PACKET REACHED DESTINATION \n";
			}
		}
		else
		{
			//cout<<"incrementing the d_mssgs numbers \n";			
			//d_mssgs++;
			Packet::free(p);            // just discard the packet
		}
	}
}
else
{
	if(rq->depth==depth-1)		//if the mssg is from a parent
	{
		//cout<<"PARENT:from a parent \n";	
		list_delete(1,rq->conzone_hop);
		list_insert(4,rq->conzone_hop);
		
		if(on_conzone==true)	//if I am also on conzone then add a on conzone parent
		{			
			Packet::free(p);            // just discard the packet
			//cout<<"PARENT: routing the queued packets \n";
			//route_waiting_packets();  //function which will try to route the waiting packets on a conzone node	
		}
		/*		
		else
		{
			//cout<<"incrementing the d_mssgs numbers \n";			
			d_mssgs++;		
			if(d_mssgs>threshold)
			{
			//	cout<<"node going on conzone due to parent \n";			
				on_conzone=true;
				if(rq->conzone_dst!=index)		// if the node is not the sink
				{
					rq->depth=depth;		//setting the depth
					rq->conzone_hop=index;		//setting the next/prev hop	 
					
			   		ih->saddr() = index;			// Doubt(why is this done)
			   		ih->daddr() = IP_BROADCAST;
					 
		  	   		ch->addr_type() = NS_AF_NONE;
		   	   		ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
					conzone_started=true;			     	
					
		      		  //Jitter the sending of AODV broadcast packets by 10ms
		      		
					//cout<<"BROADCASTING CONZONE REQUEST AGAIN 2 \n";	     			
				Scheduler::instance().schedule(target_, p,0.01 * Random::uniform());  //prateek-------function 												for broadcasting a packet--------

				//Scheduler::instance().schedule(target_, p, 0.);// broadcast the packet(have to check d function)
				}
				else
				{
					//cout<<"PACKET REACHED DESTINATION \n";
					Packet::free(p);	
				}
			}
			else
			{
				Packet::free(p);	
			}
		}*/

	}	
	else
	{
		if(rq->depth==depth)		// if the mssg is from a sibling
		{	
				list_delete(2,rq->conzone_hop);
				list_insert(5,rq->conzone_hop);
			//cout<<"SIBLING:from a sibling \n";
			if(on_conzone==true)    //if i am also on conzone then add a on conzone sibling
			{	
				Packet::free(p);            // just discard the packet
				//cout<<"SIBLING: routing the queued packets \n";
				//route_waiting_packets();  //function which will try to route the waiting packets on a conzone node
			}
			/*
			else
			{
				//cout<<"incrementing the d_mssgs numbers \n";			
				d_mssgs++;		
				if(d_mssgs>threshold)
				{
					//cout<<"node going on conzone \n";			
					on_conzone=true;
					if(rq->conzone_dst!=index)		// if the node is not the sink
					{
						rq->depth=depth;		//setting the depth
						rq->conzone_hop=index;		//setting the next/prev hop	 
						
				   		ih->saddr() = index;			// Doubt(why is this done)
				   		ih->daddr() = IP_BROADCAST;
						 
		  		   		ch->addr_type() = NS_AF_NONE;
		   		   		ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
						conzone_started=true;			     	
					
		      			 // Jitter the sending of AODV broadcast packets by 10ms
		      			
						//cout<<"BROADCASTING CONZONE REQUEST AGAIN 3 \n";	     			
					Scheduler::instance().schedule(target_, p,0.01 * Random::uniform());  //prateek-------function 												for broadcasting a packet--------

				//Scheduler::instance().schedule(target_, p, 0.);// broadcast the packet(have to check d function)
					}
					else
					{
						//cout<<"PACKET REACHED DESTINATION \n";
						
					}

				}
				else
				{
					Packet::free(p);	
				}
			}*/
		}
	}
}

}
//-------------------------------------------
void AODV::route_data(Packet *p)	//function to route a data from a on conzone node
{

struct hdr_cmn *ch = HDR_CMN(p);
//if(high priority data)		//if the data is high priority
if(priority==1)
{
	if(!list_empty(4))			//on_parent list is not empty
	{
		node *nb = on_parent.lh_first;				//then send to any on_parent node
		ch->next_hop_ = nb->nb_addr;
   		ch->addr_type() = NS_AF_INET;
   		ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
     		Scheduler::instance().schedule(target_, p, 0.);

	}
	else
	{
		if(!list_empty(5))				//on_sibling is not empty
		{
		node *nb = on_sibling.lh_first;				//then send to any on_sibling node
		ch->next_hop_ = nb->nb_addr;
   		ch->addr_type() = NS_AF_INET;
   		ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
     		Scheduler::instance().schedule(target_, p, 0.);
		}
		else
		{
		node *nb = off_parent.lh_first;				//then send to any off conzone parent node
		ch->next_hop_ = nb->nb_addr;
   		ch->addr_type() = NS_AF_INET;
   		ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
     		Scheduler::instance().schedule(target_, p, 0.);
		}
	}
}
else		//for low priority data 
{
	if(!list_empty(1))			//off_parent list is not empty
	{
		node *nb = off_parent.lh_first;				//then send to any off_parent node
		ch->next_hop_ = nb->nb_addr;
   		ch->addr_type() = NS_AF_INET;
   		ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
     		Scheduler::instance().schedule(target_, p, 0.);
	}
	else
	{
		if(!list_empty(2))				//off_sibling is not empty
		{
			node *nb = off_sibling.lh_first;				//then send to any off_sibling node
			ch->next_hop_ = nb->nb_addr;
	   		ch->addr_type() = NS_AF_INET;
	   		ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
	     		Scheduler::instance().schedule(target_, p, 0.);
		}
		else
		{
			//send data to any on conzone parent    // here i have left the concept of divided line
		node *nb = on_parent.lh_first;	//send data to any on conzone parent.here i have left the concept of divided line
		ch->next_hop_ = nb->nb_addr;
   		ch->addr_type() = NS_AF_INET;
   		ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
     		Scheduler::instance().schedule(target_, p, 0.);
		}
	}

}
//*/
}
//--------------------------------------------------------
void AODV::route_waiting_packets()		//routing the waiting packets
{
Packet *p;
while(p=rqueue.deque())
{
	
	route_data(p);
}
}
//--------------------------------------------***Prateek***----------------------------------------------------------------------

void
AODV::sendRequest(nsaddr_t dst) {
// Allocate a RREQ packet 
Packet *p = Packet::alloc();
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_aodv_request *rq = HDR_AODV_REQUEST(p);
aodv_rt_entry *rt = rtable.rt_lookup(dst);

 assert(rt);

 /*
  *  Rate limit sending of Route Requests. We are very conservative
  *  about sending out route requests. 
  */

 if (rt->rt_flags == RTF_UP) {
   assert(rt->rt_hops != INFINITY2);
   Packet::free((Packet *)p);
   return;
 }

 if (rt->rt_req_timeout > CURRENT_TIME) {
   Packet::free((Packet *)p);
   return;
 }

 // rt_req_cnt is the no. of times we did network-wide broadcast
 // RREQ_RETRIES is the maximum number we will allow before 
 // going to a long timeout.

 if (rt->rt_req_cnt > RREQ_RETRIES) {
   rt->rt_req_timeout = CURRENT_TIME + MAX_RREQ_TIMEOUT;
   rt->rt_req_cnt = 0;
 Packet *buf_pkt;
   while ((buf_pkt = rqueue.deque(rt->rt_dst))) {
       drop(buf_pkt, DROP_RTR_NO_ROUTE);
   }
   Packet::free((Packet *)p);
   return;
 }

#ifdef DEBUG
   fprintf(stderr, "(%2d) - %2d sending Route Request, dst: %d\n",
                    ++route_request, index, rt->rt_dst);
#endif // DEBUG

 // Determine the TTL to be used this time. 
 // Dynamic TTL evaluation - SRD

 rt->rt_req_last_ttl = max(rt->rt_req_last_ttl,rt->rt_last_hop_count);

 if (0 == rt->rt_req_last_ttl) {
 // first time query broadcast
   ih->ttl_ = TTL_START;
 }
 else {
 // Expanding ring search.
   if (rt->rt_req_last_ttl < TTL_THRESHOLD)
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
 if (rt->rt_req_cnt > 0)
   rt->rt_req_timeout *= rt->rt_req_cnt;
 rt->rt_req_timeout += CURRENT_TIME;

 // Don't let the timeout to be too large, however .. SRD 6/8/99
 if (rt->rt_req_timeout > CURRENT_TIME + MAX_RREQ_TIMEOUT)
   rt->rt_req_timeout = CURRENT_TIME + MAX_RREQ_TIMEOUT;
 rt->rt_expire = 0;

#ifdef DEBUG
 fprintf(stderr, "(%2d) - %2d sending Route Request, dst: %d, tout %f ms\n",
	         ++route_request, 
		 index, rt->rt_dst, 
		 rt->rt_req_timeout - CURRENT_TIME);
#endif	// DEBUG
	

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

 // Fill up some more fields. 
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

 Scheduler::instance().schedule(target_, p, 0.);

}

void
AODV::sendReply(nsaddr_t ipdst, u_int32_t hop_count, nsaddr_t rpdst,
                u_int32_t rpseq, u_int32_t lifetime, double timestamp) {
Packet *p = Packet::alloc();
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_aodv_reply *rp = HDR_AODV_REPLY(p);
aodv_rt_entry *rt = rtable.rt_lookup(ipdst);

#ifdef DEBUG
fprintf(stderr, "sending Reply from %d at %.2f\n", index, Scheduler::instance().clock());
#endif // DEBUG
 assert(rt);

 rp->rp_type = AODVTYPE_RREP;
 //rp->rp_flags = 0x00;
 rp->rp_hop_count = hop_count;
 rp->rp_dst = rpdst;
 rp->rp_dst_seqno = rpseq;
 rp->rp_src = index;
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

 Scheduler::instance().schedule(target_, p, 0.);

}

void
AODV::sendError(Packet *p, bool jitter) {
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_aodv_error *re = HDR_AODV_ERROR(p);
    
#ifdef ERROR
fprintf(stderr, "sending Error from %d at %.2f\n", index, Scheduler::instance().clock());
#endif // DEBUG

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
 ch->prev_hop_ = index;          // AODV hack
 ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction

 ih->saddr() = index;
 ih->daddr() = IP_BROADCAST;
 ih->sport() = RT_PORT;
 ih->dport() = RT_PORT;
 ih->ttl_ = 1;

 // Do we need any jitter? Yes
 if (jitter)
 	Scheduler::instance().schedule(target_, p, 0.01*Random::uniform());
 else
 	Scheduler::instance().schedule(target_, p, 0.0);

}


/*
   Neighbor Management Functions
*/

void
AODV::sendHello() {
Packet *p = Packet::alloc();
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_aodv_reply *rh = HDR_AODV_REPLY(p);

#ifdef DEBUG
fprintf(stderr, "sending Hello from %d at %.2f\n", index, Scheduler::instance().clock());
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
   nb->nb_expire = CURRENT_TIME +
                   (1.5 * ALLOWED_HELLO_LOSS * HELLO_INTERVAL);
 }

 Packet::free(p);
}

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
//------------------------------------------------Prateek---------------------------------------------------------------

//--------------------Functions for insertion and deletion from the various lists-------------------------------------

/*
function to insert a node in a list
*/
/*void AODV::list_insert(node_list *list,nsaddr_t id){
node *n =new node(id);
assert(n);
LIST_INSERT_HEAD(list,n,list_link);
}*/

void AODV::list_insert(int number,nsaddr_t id){
node *n =new node(id);
assert(n);
switch(number)
{
case 1:
LIST_INSERT_HEAD(&off_parent,n,list_link);
break;

case 2:
LIST_INSERT_HEAD(&off_sibling,n,list_link);
break;

case 3:
LIST_INSERT_HEAD(&off_children,n,list_link);
break;

case 4:
LIST_INSERT_HEAD(&on_parent,n,list_link);
break;

case 5:
LIST_INSERT_HEAD(&on_sibling,n,list_link);
break;

}
}

//--------------------------------------------------------------------------------------------------------------------------------------------
/*
function to delete a node from a list
*/
/*
void AODV::list_delete(node_list *list,nsaddr_t id){
node *n = list.lh_first;

 //log_link_del(id);

 for(; n; n = n->list_link.le_next) {
   if(n->nb_addr == id) {
     LIST_REMOVE(n,list_link);
     delete n;
     break;
   }
}
}
*/
void AODV::list_delete(int number,nsaddr_t id){
switch(number)
{
case 1:
	{	
	node *n = off_parent.lh_first;


	 for(; n; n = n->list_link.le_next) {
	   if(n->nb_addr == id) {
	     LIST_REMOVE(n,list_link);
	     delete n;
	     break;
	   }
	 }
	 //handle_link_failure(id);
break;
}
case 2:
	{
	node *n = off_sibling.lh_first;


	 for(; n; n = n->list_link.le_next) {
	   if(n->nb_addr == id) {
	     LIST_REMOVE(n,list_link);
	     delete n;
	     break;
	   }
	 }
	 //handle_link_failure(id);
break;
}
case 3:
{	
	node *n = off_children.lh_first;


	 for(; n; n = n->list_link.le_next) {
	   if(n->nb_addr == id) {
	     LIST_REMOVE(n,list_link);
	     delete n;
	     break;
	   }
	 }
	 //handle_link_failure(id);
break;
}
case 4:
{
	node *n = on_parent.lh_first;


	 for(; n; n = n->list_link.le_next) {
	   if(n->nb_addr == id) {
	     LIST_REMOVE(n,list_link);
	     delete n;
	     break;
	   }
	 }
	 //handle_link_failure(id);
break;
}
case 5:
{
	node *n = on_sibling.lh_first;


	 for(; n; n = n->list_link.le_next) {
	   if(n->nb_addr == id) {
	     LIST_REMOVE(n,list_link);
	     delete n;
	     break;
	   }
	 }
	 //handle_link_failure(id);
break;
}
}
}
//--------------------------------------------------------------------------------------------------------------------------------------------

/*
function to lookup a entry in a list
*/
/*node* AODV::list_lookup(node_list list,nsaddr_t id) {
node *nb = list.lh_first;

 for(; nb; nb = nb->list_link.le_next) {
   if(nb->nb_addr == id) break;
 }
 return nb;
}*/
node* AODV::list_lookup(int number,nsaddr_t id) {
switch(number)
{
case 1:
	{node *nb1 = off_parent.lh_first;
	
	 for(; nb1; nb1 = nb1->list_link.le_next) {
	   if(nb1->nb_addr == id) break;
	 }
	 return nb1;
	break;}
case 2:
	{node *nb2 = off_sibling.lh_first;
	
	 for(; nb2; nb2 = nb2->list_link.le_next) {
	   if(nb2->nb_addr == id) break;
	 }
	 return nb2;
	break;}
case 3:
	{node *nb3 = off_children.lh_first;
	
	 for(; nb3; nb3 = nb3->list_link.le_next) {
	   if(nb3->nb_addr == id) break;
	 }
	 return nb3;
	break;}
case 4:
	{node *nb4 = on_parent.lh_first;
	
	 for(; nb4; nb4 = nb4->list_link.le_next) {
	   if(nb4->nb_addr == id) break;
	 }
	 return nb4;
	break;}
case 5:
	{node *nb5 = on_sibling.lh_first;
	
	 for(; nb5; nb5 = nb5->list_link.le_next) {
	   if(nb5->nb_addr == id) break;
	 }
	 return nb5;
	break;}

}
}
//--------------------------------------------------------------------------------------------------------------------------------------------

/*
function to purge a list
*/

/*void
AODV::list_purge(node_list list) {
node *nb = list.lh_first;
node *nbn;

 for(; nb; nb = nbn) {
   nbn = nb->list_link.le_next;
     nb_delete(nb->nb_addr);
      }
}*/
void
AODV::list_purge(int number) {
switch(number)
{
case 1:
	{
	node *nb1 = off_parent.lh_first;
	node *nbn1;

	 for(; nb1; nb1 = nbn1) {
	   nbn1 = nb1->list_link.le_next;
	     list_delete(1,nb1->nb_addr);
	      }
	break;
	}	
case 2:
	{	
	node *nb2 = off_sibling.lh_first;
	node *nbn2;

	 for(; nb2; nb2 = nbn2) {
	   nbn2 = nb2->list_link.le_next;
	     list_delete(2,nb2->nb_addr);
	      }
	break;
	}
case 3:
	{	
	node *nb3 = off_children.lh_first;
	node *nbn3;

	 for(; nb3; nb3 = nbn3) {
	   nbn3 = nb3->list_link.le_next;
	     list_delete(3,nb3->nb_addr);
	      }
	break;
	}
case 4:
	{	
	node *nb4 = on_parent.lh_first;
	node *nbn4;

	 for(; nb4; nb4 = nbn4) {
	   nbn4 = nb4->list_link.le_next;
	     list_delete(4,nb4->nb_addr);
	      }
	break;
	}
case 5:
	{	
	node *nb5 = on_sibling.lh_first;
	node *nbn5;

	 for(; nb5; nb5 = nbn5) {
	   nbn5 = nb5->list_link.le_next;
	     list_delete(5,nb5->nb_addr);
	      }
	break;
	}
}
}
//-----------------------------------------------------------------------------------------------------------------

// i donot think that i require this function anymore
/*
bool
AODV::list_empty(node_list list) {	//i am not confirmed about the correctness of this function
node *nb = list.lh_first;

 for(; nb; nb = nb->list_link.le_next) {
 if(nb)
 { 
	return false;
 }
 else
 {
	return true;
 }
}
}

*/
bool AODV::list_empty(int number)
{
	switch(number)
	{
	case 1:
		{
			node *nb=off_parent.lh_first;
			if(!nb)
			{
				return true;
			}
			else
			{
				return false;
			}
			break;
		}
	case 2:
		{
			node *nb=off_sibling.lh_first;
			if(!nb)
			{
				return true;
			}
			else
			{
				return false;
			}
			break;
		}
	case 3:
		{
			node *nb=off_children.lh_first;
			if(!nb)
			{
				return true;
			}
			else
			{
				return false;
			}
			break;
		}
	case 4:
		{
			node *nb=on_parent.lh_first;
			if(!nb)
			{
				return true;
			}
			else
			{
				return false;
			}
			break;
		}
	case 5:
		{
			node *nb=on_sibling.lh_first;
			if(!nb)
			{
				return true;
			}
			else
			{
				return false;
			}
			break;
		}
	}
}

//----------------------------------------------
void AODV::print_off_parent()
{
	node *nb1 = off_parent.lh_first;
	cout<<"the OFF CONZONE parent of node with depth: "<<depth<<" :IP Address: "<<index<<" are:- ";
	 for(; nb1; nb1 = nb1->list_link.le_next) {
		cout<<nb1->nb_addr<<"  ";   
	 }
		cout<<"\n";

}
void AODV::print_off_sibling()
{
	node *nb1 = off_sibling.lh_first;
	cout<<"the OFF CONZONE sibling of node with depth: "<<depth<<" :IP Address: "<<index<<" are:- ";
	 for(; nb1; nb1 = nb1->list_link.le_next) {
		cout<<nb1->nb_addr<<"  ";   
	 }
		cout<<"\n";

}
void AODV::print_off_children()
{
	node *nb1 = off_children.lh_first;
	cout<<"the OFF CONZONE children of node with depth: "<<depth<<" :IP Address: "<<index<<" are:- ";
	 for(; nb1; nb1 = nb1->list_link.le_next) {
		cout<<nb1->nb_addr<<"  ";   
	 }
		cout<<"\n";

}
void AODV::print_on_parent()
{
	node *nb1 = on_parent.lh_first;
	cout<<"the ON CONZONE parent of node with depth: "<<depth<<" :IP Address: "<<index<<" are:- ";
	 for(; nb1; nb1 = nb1->list_link.le_next) {
		cout<<nb1->nb_addr<<"  ";   
	 }
		cout<<"\n";

}
void AODV::print_on_sibling()
{
	node *nb1 = on_sibling.lh_first;
	cout<<"the ON CONZONE sibling of node with depth: "<<depth<<" :IP Address: "<<index<<" are:- ";
	 for(; nb1; nb1 = nb1->list_link.le_next) {
		cout<<nb1->nb_addr<<"  ";   
	 }
		cout<<"\n";

}
void AODV::print_all_lists()
{
	print_off_parent();
	print_off_children();
	print_off_sibling();
	print_on_parent();
	print_on_sibling();
}
//-------------------------------------------------***Prateek***------------------------------------------------------------------

AODV_Neighbor*
AODV::nb_lookup(nsaddr_t id) {
AODV_Neighbor *nb = nbhead.lh_first;

 for(; nb; nb = nb->nb_link.le_next) {
   if(nb->nb_addr == id) break;
 }
 return nb;
}


/*
 * Called when we receive *explicit* notification that a Neighbor
 * is no longer reachable.
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
 * Purges all timed-out Neighbor Entries - runs every
 * HELLO_INTERVAL * 1.5 seconds.
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
