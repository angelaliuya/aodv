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

#include <aodv/aodv.h>
#include <aodv/aodv_packet.h>
#include <ip.h>

#define CURRENT_TIME    Scheduler::instance().clock()

static const int verbose = 0;

/* =====================================================================
   Logging Functions
   ===================================================================== */
void
AODV::log_link_del(nsaddr_t dst)
{
        static int link_del = 0;

        if(! logtarget || ! verbose) return;

        /*
         *  If "god" thinks that these two nodes are still
         *  reachable then this is an erroneous deletion.
         */
        sprintf(logtarget->pt_->buffer(),
                "A %.9f _%d_ deleting LL hop to %d (delete %d is %s)",
                CURRENT_TIME,
                index,
                dst,
                ++link_del,
                God::instance()->hops(index, dst) != 1 ? "VALID" : "INVALID");
        logtarget->pt_->dump();
}


void
AODV::log_link_broke(Packet *p)
{
        static int link_broke = 0;
        struct hdr_cmn *ch = HDR_CMN(p);

        if(! logtarget || ! verbose) return;

        sprintf(logtarget->pt_->buffer(),
                "A %.9f _%d_ LL unable to deliver packet %d to %d (%d) (reason = %d, ifqlen = %d)",
                CURRENT_TIME,
                index,
                ch->uid_,
                ch->next_hop_,
                ++link_broke,
                ch->xmit_reason_,
                ifqueue->length());
	logtarget->pt_->dump();
}

void
AODV::log_link_kept(nsaddr_t dst)
{
        static int link_kept = 0;

        if(! logtarget || ! verbose) return;


        /*
         *  If "god" thinks that these two nodes are now
         *  unreachable, then we are erroneously keeping
         *  a bad route.
         */
        sprintf(logtarget->pt_->buffer(),
                "A %.9f _%d_ keeping LL hop to %d (keep %d is %s)",
                CURRENT_TIME,
                index,
                dst,
                ++link_kept,
                God::instance()->hops(index, dst) == 1 ? "VALID" : "INVALID");
        logtarget->pt_->dump();
}

