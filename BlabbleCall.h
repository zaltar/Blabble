/**********************************************************\
Original Author: Andrew Ofisher (zaltar)

License:	GNU General Public License, version 3.0
			http://www.gnu.org/licenses/gpl-3.0.txt

Copyright 2012 Andrew Ofisher
\**********************************************************/

#include <string>
#include <sstream>
#include "JSAPIAuto.h"
#include "BrowserHost.h"
#include <pjlib.h>
#include <pjlib-util.h>
#include <pjnath.h>
#include <pjsip.h>
#include <pjsip_ua.h>
#include <pjsip_simple.h>
#include <pjsua-lib/pjsua.h>
#include <pjmedia.h>
#include <pjmedia-codec.h> 

#ifndef H_BlabbleCallAPI
#define H_BlabbleCallAPI

#ifdef WIN32
	#include <Windows.h>
	#define ATOMIC_INCREMENT(a) InterlockedIncrement(a)
	#define INTERLOCKED_EXCHANGE(a, b) InterlockedExchange(a, b)
#else
	#define ATOMIC_INCREMENT(a) __sync_add_and_fetch(a, 1)
	#define INTERLOCKED_EXCHANGE(a, b) __sync_lock_test_and_set(a, b)
#endif

FB_FORWARD_PTR(BlabbleAccount);
FB_FORWARD_PTR(BlabbleAudioManager);
FB_FORWARD_PTR(BlabbleCall);

#define INVALID_CALL -1

enum CallState
{
	CALL_INVALID = -1,
	CALL_ACTIVE = 0,
	CALL_HOLD = 1,
	CALL_RINGING_OUT = 2,
	CALL_RINGING_IN = 3,
	CALL_BUSY = 4,
	CALL_ERROR_CANNOTCOMPLETE = 5, //404, 503
	CALL_ERROR_DISCONNECTED = 6 //603
};

class BlabbleCall : public FB::JSAPIAuto
{
	public:
		BlabbleCall(const BlabbleAccountPtr& parent_account);
		virtual ~BlabbleCall(void);

		/*! @Brief JavaScript method to answer a ringing call
		 */
		bool Answer();
		
		/*! @Brief JavaScript method to put a call on hold (disconnect audio)
		 */
		bool Hold();
		
		/*! @Brief Unhold a previously held call.
		 *  @sa Hold
		 */
		bool Unhold();
		
		/*! @Brief Called when the call was ended by us.
		 */
		void LocalEnd(); //Call ended by us, such as JS end
		
		/*! @Brief Send a single DTMF tone (0-9, #, *).
		 */
		bool SendDTMF(const std::string& dtmf);
		
		/*! @Brief JavaScript method to join this call and arg together and remove us.
		 */
		bool TransferReplace(const BlabbleCallPtr& arg);
		
		/*! @Brief JavaScript method to Blind transfer a call
		 */
		bool Transfer(const FB::VariantMap &params);

		/*! @Brief JavaScript property to expose the incoming caller id
		 */
		std::string caller_id();
		
		/*! @Brief JavaScript property to return a JavaScript object with the call's status, caller ID, and duration.
		 */
		FB::VariantMap status();
		
		/*! @Brief JavaScript property that returns true if this call is active (audio is bridged to sound card)
		 */
		bool is_active();
		
		/*! @Brief Used by JavaScript to determine if the call object is valid
		 */
		virtual bool get_valid();

		/*! @Brief A write only JavaScript property used to set the callback function for when a call is connected.
		 */
		void set_on_call_connected(const FB::JSObjectPtr& v) { on_call_connected_ = v; }
		
		/*! @Brief A write only JavaScript property used to set the callback function for when a outgoing call is ringing.
		 */
		void set_on_call_ringing(const FB::JSObjectPtr& v) { on_call_ringing_ = v; }
		
		/*! @Brief A write only JavaScript property used to set the callback function for when a call has ended.
		 */
		void set_on_call_end(const FB::JSObjectPtr& v) { on_call_end_ = v; }
		
		/*! @Brief A write only JavaScript property used to set the callback function to notify of the status of a transfer.
		 */
		void set_on_transfer_status(const FB::JSObjectPtr& v) { on_transfer_status_ = v; }
		
		/*! @Brief Returns the PJSIP account id
		 */
		int acct_id() const { return acct_id_; }
		
		/*! @Brief Returns the PJSIP call id
		 */
		pjsua_call_id callId() const { return call_id_; }
		
		/*! @Brief Returns the target of this call
		 */
		std::string destination() const { return destination_; }

		/*! @Brief Called by BlabbleAccount when PJSIP notifies us of a change in the media state.
		 */
		void OnCallMediaState();
		
		/*! @Brief Called by BlabbleAccount when PJSIP notifies us of a change in the call state.
		 */
		void OnCallState(pjsua_call_id call_id, pjsip_event *e);
		
		/*! @Brief Called by BlabbleAccount when PJSIP notifies us of the status of a transfer.
		 */
		bool OnCallTransferStatus(int status);
		
		/*! @Brief Called by BlabbleAccount to make an outbound call.
		 */
		pj_status_t MakeCall(const std::string& dest, const std::string& identity = "");
		
		/*! @Brief Called by BlabbleAccount to buildup a BlabbleCall for an incoming call.
		 */
		bool RegisterIncomingCall(pjsua_call_id callId);  
		
		/*! @Brief A globally unique id for this call. 
		 *
		 *  Used to identify a call between accounts and even after it has been
		 *  destroyed by PJSIP.
		 */
		unsigned int id() const { return id_; }

	private:
		std::string destination_;
		unsigned int id_;
		volatile pjsua_call_id call_id_;
		pjsua_acc_id acct_id_;
		bool ringing_;

		BlabbleAudioManagerPtr audio_manager_;
		BlabbleAccountWeakPtr parent_;
  
		FB::JSObjectPtr on_call_connected_;
		FB::JSObjectPtr on_call_ringing_;
		FB::JSObjectPtr on_call_end_;
		FB::JSObjectPtr on_transfer_status_;

		void StopRinging();
		void StartInRinging();
		void StartOutRinging();

		BlabbleAccountPtr CheckAndGetParent();
		//Ended by system
		void RemoteEnd(const pjsua_call_info &info);
		BlabbleCallPtr get_shared() { return boost::static_pointer_cast<BlabbleCall>(this->shared_from_this()); }
		
	private:
		static unsigned int id_counter_;
		static unsigned int GetNextId();

		void CallOnCallEnd();
		void CallOnCallEnd(pjsip_status_code status);
		void CallOnTransferStatus(int status);
};

#endif //H_BlabbleCallAPI
