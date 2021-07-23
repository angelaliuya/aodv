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


#include <assert.h>

#include <cmu-trace.h>
#include <aodv/aodv_rqueue.h>

#define CURRENT_TIME    Scheduler::instance().clock()
#define QDEBUG

/*
  Packet Queue used by AODV.
*/

aodv_rqueue::aodv_rqueue() {
  head_ = tail_ = 0;
  len_ = 0;
  limit_ = AODV_RTQ_MAX_LEN;
  timeout_ = AODV_RTQ_TIMEOUT;
}

void
aodv_rqueue::enque(Packet *p) {
struct hdr_cmn *ch = HDR_CMN(p);

 /*
  * Purge any packets that have timed out.
  */
 purge();
 
 p->next_ = 0;
 ch->ts_ = CURRENT_TIME + timeout_;
 
 /*
  * The maximum number of packets that we allow a routing protocol to buffer:
  * limit_ = AODV_RTQ_MAX_LEN = 64 packets
  */
 if (len_ == limit_) {
 Packet *p0 = remove_head();	// decrements len_

   assert(p0);
   if(HDR_CMN(p0)->ts_ > CURRENT_TIME) {
     fprintf(stderr, "%d - DROP_RTR_QFULL uid=%d\n",HDR_IP(p0)->saddr(),
	     HDR_CMN(p0)->uid());
     drop(p0, DROP_RTR_QFULL);
   }
   else {
     fprintf(stderr, "%d - DROP_RTR_QTIMEOUT uid=%d\n",HDR_IP(p0)->saddr(),
	     HDR_CMN(p0)->uid());
     drop(p0, DROP_RTR_QTIMEOUT);
   }
 }
 
 if(head_ == 0) {
   head_ = tail_ = p;
 }
 else {
   tail_->next_ = p;
   tail_ = p;
 }
 len_++;
#ifdef QDEBUG
   verifyQueue();
#endif // QDEBUG
}
                

Packet*
aodv_rqueue::deque() {
Packet *p;

 /*
  * Purge any packets that have timed out.
  */
 purge();

 p = remove_head();
#ifdef QDEBUG
 verifyQueue();
#endif // QDEBUG
 return p;

}


Packet*
aodv_rqueue::deque(nsaddr_t dst) {
Packet *p, *prev;

 /*
  * Purge any packets that have timed out.
  */
 purge();

 findPacketWithDst(dst, p, prev);
 assert(p == 0 || (p == head_ && prev == 0) || (prev->next_ == p));

 if(p == 0) return 0;

 if (p == head_) {
   p = remove_head();
 }
 else if (p == tail_) {
   prev->next_ = 0;
   tail_ = prev;
   len_--;
 }
 else {
   prev->next_ = p->next_;
   len_--;
 }

#ifdef QDEBUG
 verifyQueue();
#endif // QDEBUG
 return p;

}

char 
aodv_rqueue::find(nsaddr_t dst) {
Packet *p, *prev;  
	
 findPacketWithDst(dst, p, prev);
 if (0 == p)
   return 0;
 else
   return 1;

}

	
	

/*
  Private Routines
*/

Packet*
aodv_rqueue::remove_head() {
Packet *p = head_;
        
 if(head_ == tail_) {
   head_ = tail_ = 0;
 }
 else {
   head_ = head_->next_;
 }

 if(p) len_--;

 return p;

}

void
aodv_rqueue::findPacketWithDst(nsaddr_t dst, Packet*& p, Packet*& prev) {
  
  p = prev = 0;
  for(p = head_; p; p = p->next_) {
	  //		if(HDR_IP(p)->dst() == dst) {
	       if(HDR_IP(p)->daddr() == dst) {
      return;
    }
    prev = p;
  }
}


void
aodv_rqueue::verifyQueue() {
Packet *p, *prev = 0;
int cnt = 0;

 for(p = head_; p; p = p->next_) {
   cnt++;
   prev = p;
 }
 assert(cnt == len_);
 assert(prev == tail_);

}


/*
void
aodv_rqueue::purge() {
Packet *p;

 while((p = head_) && HDR_CMN(p)->ts_ < CURRENT_TIME) {
   // assert(p == remove_head());     
   p = remove_head();     
   drop(p, DROP_RTR_QTIMEOUT);
 }

}
*/

bool
aodv_rqueue::findAgedPacket(Packet*& p, Packet*& prev) {
  
  p = prev = 0;
  for(p = head_; p; p = p->next_) {
    if(HDR_CMN(p)->ts_ < CURRENT_TIME) {
      return true;
    }
    prev = p;
  }
  return false;
}

void
aodv_rqueue::purge() {
Packet *p, *prev;

 while ( findAgedPacket(p, prev) ) {
 	assert(p == 0 || (p == head_ && prev == 0) || (prev->next_ == p));

 	if(p == 0) return;

 	if (p == head_) {
   		p = remove_head();
 	}
 	else if (p == tail_) {
   		prev->next_ = 0;
   		tail_ = prev;
   		len_--;
 	}
 	else {
   		prev->next_ = p->next_;
   		len_--;
 	}
#ifdef QDEBUG
 	verifyQueue();
#endif // QDEBUG

	p = prev = 0;
 }

}

