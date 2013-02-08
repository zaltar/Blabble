/**********************************************************\
Original Author: Andrew Ofisher (zaltar)

License:    GNU General Public License, version 3.0
            http://www.gnu.org/licenses/gpl-3.0.txt

Copyright 2012 Andrew Ofisher
\**********************************************************/

#ifndef H_PjsuaManagerPLUGIN
#define H_PjsuaManagerPLUGIN

#include "JSAPIAuto.h"
#include <string>
#include <map>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <pjlib.h>
#include <pjlib-util.h>
#include <pjnath.h>
#include <pjsip.h>
#include <pjsip_ua.h>
#include <pjsip_simple.h>
#include <pjsua-lib/pjsua.h>
#include <pjmedia.h>
#include <pjmedia-codec.h> 

FB_FORWARD_PTR(BlabbleCall)
FB_FORWARD_PTR(BlabbleAccount)
FB_FORWARD_PTR(BlabbleAudioManager)
FB_FORWARD_PTR(PjsuaManager)

typedef std::map<int, BlabbleAccountPtr> BlabbleAccountMap;

/*! @class  PjsuaManager
 *
 *  @brief  Singleton used to manage accounts, audio, and callbacks from PJSIP.
 *
 *  @author Andrew Ofisher (zaltar)
*/
class PjsuaManager : public boost::enable_shared_from_this<PjsuaManager>
{
public:
	
	static PjsuaManagerPtr GetManager(const std::string& path, bool enableIce,
		const std::string& stunServer);
	virtual ~PjsuaManager();

	/*! @Brief Retrive the current audio manager.
	 *  The audio manager allows control of ringing, busy signals, and ringtones.
	 */
	BlabbleAudioManagerPtr audio_manager() { return audio_manager_; }

	void AddAccount(const BlabbleAccountPtr &account);
	void RemoveAccount(pjsua_acc_id acc_id);
	BlabbleAccountPtr FindAcc(int accId);
	
	/*! Return true if we have TLS/SSL capability.
	 */
	bool has_tls() { return has_tls_; }
	
public:
	/*! @Brief Callback for PJSIP.
	 *  Called when an incoming call comes in on an account.
	 */
	static void OnIncomingCall(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata);
	
	/*! @Brief Callback for PJSIP.
	 *  Called when the media state of a call changes.
	 */
	static void OnCallMediaState(pjsua_call_id call_id);
	
	/*! @Brief Callback for PJSIP.
	 *  Called when the state of a call changes.
	 */
	static void OnCallState(pjsua_call_id call_id, pjsip_event *e);
	
	/*! @Brief Callback for PJSIP.
	 *  Called when the registration status of an account changes.
	 */
	static void OnRegState(pjsua_acc_id acc_id);
	
	/*! @Brief Callback for PJSIP.
	 *  Called if there is a change in the transport. This is usually for TLS.
	 */
	static void OnTransportState(pjsip_transport *tp, pjsip_transport_state state, const pjsip_transport_state_info *info);
	
	/*! @Brief Callback for PJSIP.
	 *  Called after a call is transfer to report the status.
	 */
	static void OnCallTransferStatus(pjsua_call_id call_id, int st_code, const pj_str_t *st_text, pj_bool_t final, pj_bool_t *p_cont);

private:
	BlabbleAccountMap accounts_;
	BlabbleAudioManagerPtr audio_manager_;
	pjsua_transport_id udp_transport, tls_transport;
	bool has_tls_;

	static PjsuaManagerWeakPtr instance_;
	//PjsuaManager is a singleton. Only one should ever exist so that PjSip callbacks work.
	PjsuaManager(const std::string& executionPath, bool enableIce,
		const std::string& stunServer);
};

#endif // H_PjsuaManagerPLUGIN
