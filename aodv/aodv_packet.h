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
 * This code was enhanced by Ali Hamidian, Department of Communication Systems,
 * Lund Institute of Technology, Lund University.
 * ns-2.1b9a/ns-2.26 enhancements for simulations of Internet connectivity for
 * mobile ad hoc networks with AODV.
 * e-mail: alex.hamidian@telecom.lth.se
 * ############################################################################
 */

#ifndef __aodv_packet_h__
#define __aodv_packet_h__

//#include <config.h>
//#include "aodv.h"
#define AODV_MAX_ERRORS 100


/* =====================================================================
   Packet Formats...
   ===================================================================== */
#define AODVTYPE_HELLO  	0x01
#define AODVTYPE_RREQ   	0x02
#define AODVTYPE_RREP   	0x04
#define AODVTYPE_RERR   	0x08
#define AODVTYPE_RREP_ACK  	0x10
//My modification***************************************//
/*
  Implementing proactive gateway discovery...
*/
#define AODVTYPE_ADVERTISEMENT  0x20
//******************************************************//

/*
 * AODV Routing Protocol Header Macros
 */
#define HDR_AODV(p)            ((struct hdr_aodv*)hdr_aodv::access(p))
#define HDR_AODV_REQUEST(p)    ((struct hdr_aodv_request*)hdr_aodv::access(p))
#define HDR_AODV_REPLY(p)      ((struct hdr_aodv_reply*)hdr_aodv::access(p))
#define HDR_AODV_ERROR(p)      ((struct hdr_aodv_error*)hdr_aodv::access(p))
#define HDR_AODV_RREP_ACK(p)   ((struct hdr_aodv_rrep_ack*)hdr_aodv::access(p))
//My modification************************************************************//
/*
  Implementing proactive gateway discovery...
*/
#define HDR_AODV_ADVERTISEMENT(p) ((struct hdr_aodv_advertisement*)hdr_aodv::access(p))
//***************************************************************************//

/*
 * General AODV Header - shared by all formats
 */
struct hdr_aodv {
  u_int8_t        ah_type;
  /*
    u_int8_t        ah_reserved[2];
    u_int8_t        ah_hopcount;
  */
  // Header access methods
  static int offset_; // required by PacketHeaderManager
  inline static int& offset() { return offset_; }
  inline static hdr_aodv* access(const Packet* p) {
    return (hdr_aodv*) p->access(offset_);
  }
};

struct hdr_aodv_request {
  u_int8_t        rq_type;	// Packet Type
  //My modification**********************************************************//
  /*
    I changed "reserved[2]" to "reserved[1]", to have a field for flags where
    I can put the I-flag. 
  */
  u_int8_t        rq_flags;       // The I flag
#define RREQ_IFLAG 0x10
  //u_int8_t        reserved[2];
  u_int8_t        reserved[1];
  //*************************************************************************//
  u_int8_t        rq_hop_count;   // Hop Count
  u_int32_t       rq_bcast_id;    // Broadcast ID
  
  nsaddr_t        rq_dst;         // Destination IP Address
  u_int32_t       rq_dst_seqno;   // Destination Sequence Number
  nsaddr_t        rq_src;         // Source IP Address
  u_int32_t       rq_src_seqno;   // Source Sequence Number
  
  double          rq_timestamp;   // when REQUEST sent;
  // used to compute route discovery latency
  
#define RREQ_GRAT_RREP	0x80
  
  inline int size() { 
    int sz = 0;
    /*
      sz = sizeof(u_int8_t)		// rq_type
      + 2*sizeof(u_int8_t) 	// reserved
      + sizeof(u_int8_t)		// rq_hop_count
      + sizeof(double)		// rq_timestamp
      + sizeof(u_int32_t)	// rq_bcast_id
      + sizeof(nsaddr_t)		// rq_dst
      + sizeof(u_int32_t)	// rq_dst_seqno
      + sizeof(nsaddr_t)		// rq_src
      + sizeof(u_int32_t);	// rq_src_seqno
    */
    sz = 7*sizeof(u_int32_t);
    assert (sz >= 0);
    return sz;
  }
};

struct hdr_aodv_reply {
  u_int8_t        rp_type;        // Packet Type
  //My modification**********************************************************//
  /*
    I changed "reserved[2]" to "reserved[1]", to have a field for flags where
    I can put the I-flag.
  */
  u_int8_t        rp_flags;              // The I flag
#define RREP_IFLAG 0x20
  //u_int8_t        reserved[2];
  //u_int8_t        reserved[1];
   u_int8_t          ifqlen;                //数据包队列长度
  //*************************************************************************//
  u_int8_t        rp_hop_count;           // Hop Count
  nsaddr_t        rp_dst;                 // Destination IP Address
  u_int32_t       rp_dst_seqno;           // Destination Sequence Number
  nsaddr_t        rp_src;                 // Source IP Address
  double	  rp_lifetime;            // Lifetime
  
  double          rp_timestamp;           // when corresponding REQ sent;
  // used to compute route discovery latency
  
  inline int size() { 
    int sz = 0;
    
      sz = sizeof(u_int8_t)		// rp_type
      + 2*sizeof(u_int8_t) 	// rp_flags + ifqlen
      + sizeof(u_int8_t)		// rp_hop_count
      + 2*sizeof(double)		// rp_timestamp
      + sizeof(nsaddr_t)		// rp_dst
      + sizeof(u_int32_t)	// rp_dst_seqno
      + sizeof(nsaddr_t);		// rp_src
     
    
   // sz = 6*sizeof(u_int32_t);
    assert (sz >= 0);
    return sz;
  }
  
};

struct hdr_aodv_error {
  u_int8_t        re_type;                // Type
  u_int8_t        reserved[2];            // Reserved
  u_int8_t        DestCount;                 // DestCount
  // List of Unreachable destination IP addresses and sequence numbers
  nsaddr_t        unreachable_dst[AODV_MAX_ERRORS];   
  u_int32_t       unreachable_dst_seqno[AODV_MAX_ERRORS];   
  
  inline int size() { 
    int sz = 0;
    /*
      sz = sizeof(u_int8_t)		// type
      + 2*sizeof(u_int8_t) 	// reserved
      + sizeof(u_int8_t)		// length
      + length*sizeof(nsaddr_t); // unreachable destinations
    */
    sz = (DestCount*2 + 1)*sizeof(u_int32_t);
    assert(sz);
    return sz;
  }

};

struct hdr_aodv_rrep_ack {
  u_int8_t	rpack_type;
  u_int8_t	reserved;
};

//My modification************************************************************//
/*
  Implementing proactive gateway discovery...
  A RREP packet extended with the "Broadcast ID" field from the RREQ packet.
  The "Broadcast ID" is renamed to "RREQ ID" in draft-ietf-manet-aodv-13.txt.
*/
struct hdr_aodv_advertisement {
  u_int8_t        ad_type;                // Packet Type
  u_int8_t        ad_flags;               // Flags
  u_int8_t        reserved;               // Reserved
  u_int8_t        ad_hop_count;           // Hop Count
  
  nsaddr_t        ad_dst;                 // Destination IP Address
  u_int32_t       ad_dst_seqno;           // Destination Sequence Number
  nsaddr_t        ad_src;                 // Source IP Address
  u_int32_t       ad_lifetime;            // Lifetime
  
  u_int32_t       ad_bcast_id;            // Broadcast ID
  double          ad_timestamp;           // when corresponding REQ sent
  // used to compute route discovery latency

  inline int size() { 
    int sz = 0;
    sz = 7*sizeof(u_int32_t);
    assert (sz >= 0);
    return sz;
  }
};
//***************************************************************************//

// for size calculation of header-space reservation
union hdr_all_aodv {
  hdr_aodv          ah;
  hdr_aodv_request  rreq;
  hdr_aodv_reply    rrep;
  hdr_aodv_error    rerr;
  hdr_aodv_rrep_ack rrep_ack;
  //My modification**********************************************************//
  /*
    Implementing proactive gateway discovery...
  */
  hdr_aodv_advertisement ad;
  //*************************************************************************//
};

#endif /* __aodv_packet_h__ */
