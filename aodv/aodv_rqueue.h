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


#ifndef __aodv_rqueue_h__
#define __aodv_rqueue_h_

//#include <packet.h>
#include <ip.h>
#include <agent.h>

/*
 * The maximum number of packets that we allow a routing protocol to buffer.
 */
#define AODV_RTQ_MAX_LEN     64      // packets

/*
 *  The maximum period of time that a routing protocol is allowed to buffer
 *  a packet for.
 */
#define AODV_RTQ_TIMEOUT     30	// seconds

class aodv_rqueue : public Connector {
 public:
        aodv_rqueue();

        void            recv(Packet *, Handler*) { abort(); }

        void            enque(Packet *p);

	inline int      command(int argc, const char * const* argv) 
	  { return Connector::command(argc, argv); }

        /*
         *  Returns a packet from the head of the queue.
         */
        Packet*         deque(void);

        /*
         * Returns a packet for destination "D".
         */
        Packet*         deque(nsaddr_t dst);
  /*
   * Finds whether a packet with destination dst exists in the queue
   */
        char            find(nsaddr_t dst);

 private:
        Packet*         remove_head();
        void            purge(void);
	void		findPacketWithDst(nsaddr_t dst, Packet*& p, Packet*& prev);
	bool 		findAgedPacket(Packet*& p, Packet*& prev); 
	void		verifyQueue(void);

        Packet          *head_;
        Packet          *tail_;

        int             len_;

        int             limit_;
        double          timeout_;

};

#endif /* __aodv_rqueue_h__ */
