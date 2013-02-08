/**********************************************************\
Original Author: Andrew Ofisher (zaltar)

License:    GNU General Public License, version 3.0
            http://www.gnu.org/licenses/gpl-3.0.txt

Copyright 2012 Andrew Ofisher
\**********************************************************/

#include <string>
#include <sstream>
#include "JSAPIAuto.h"
#include "BrowserHost.h"
#include <boost/thread/recursive_mutex.hpp>
#include <pjlib.h>
#include <pjlib-util.h>
#include <pjnath.h>
#include <pjsip.h>
#include <pjsip_ua.h>
#include <pjsip_simple.h>
#include <pjsua-lib/pjsua.h>
#include <pjmedia.h>
#include <pjmedia-codec.h> 
#include "PjsuaManager.h"

#ifndef H_BlabbleAccount
#define H_BlabbleAccount

FB_FORWARD_PTR(BlabbleCall);
FB_FORWARD_PTR(PjsuaManager);
FB_FORWARD_PTR(BlabbleAccount);

typedef std::list<BlabbleCallPtr> BlabbleCallList;
#define INVALID_ACCOUNT -1

class BlabbleAccount : public FB::JSAPIAuto
{
public:
	/*! @Brief Create a new Account.
	 *  This is called from BlabbleAPI when JavaScript code
	 *  calls the `createAccount(options)` function on the 
	 *  plugin object.
	 */
	BlabbleAccount(PjsuaManagerPtr manager);
	
	/*! @Brief Deconstructor
	 *  Removes this account from the manager, unregisters
	 *  it from the server, and terminates any calls.
	 */
	virtual ~BlabbleAccount();

	/*! @Brief Called from JavaScript to make a new call
	 *  This function will setup a new call and return a new Call JavaScript
	 *  object. If destination is missing, a JavaScript exception is thrown.
	 *  If the call fails for whatever reason, a JavaScript object is returned
	 *  with valid set to false and error set to the text error reason.
	 */
	FB::variant MakeCall(const FB::VariantMap &params);
	
	/*! @Brief (Re)register with the server.
	 *  Force a reregistration to the server or register after a previous unregister
	 */
	void Register();
	
	/*! @Brief Unregister from the server.
	 *  This will not terminate any active calls.
	 */
	void Unregister();
	
	/*! @Brief Unregister the account and terminate all calls.
	 *  Called by the deconstructor, this will unregister the account
	 *  from the server and terminate all calls. A subsequent call to 
	 *  `Register()` will register this account again.
	 */
	void Destroy();
	
	/*! @Brief Return the ID of this account as used by PJSIP
	 */
	pjsua_acc_id id() { return id_; }

	/*! @Brief JavaScript property to return the active call (if one).
	 *  The active call is that call that is currently utilizing audio.
	 *  There is only one active call per account.  There is nothing
	 *  preventing multiple accounts from each having an active call.
	 *  If no call is active, null is returned.
	 */
	BlabbleCallWeakPtr active_call();
	
	/*! @Brief JavaScript property to return all current calls on this account.
	 *  Returns an array of any valid calls on this account.
	 */
	FB::VariantList calls();
	
	/*! @Brief JavaScript property to return true if currently registered.
	 */
	bool registered();

	/*! @Brief Called from PjsuaManager when a new incoming call arrives for this account.
	 */
	bool OnIncomingCall(pjsua_call_id call_id, pjsip_rx_data *rdata);
	
	/*! @Brief Called by PjsuManager when the registration state of this account changes.
	 */
	void OnRegState();
	
	/*! @Brief Called by PjsuaManager when the state of a call in this account changes.
	 */
	void OnCallState(pjsua_call_id call_id, pjsip_event *e);
	
	/*! @Brief Called by PjsuaManager when the media state of a call in this account changes.
	 */
	void OnCallMediaState(pjsua_call_id call_id);
	
	/*! @Brief Called by PjsuaManager when a call in this account has transfered.
	 */
	bool OnCallTransferStatus(pjsua_call_id call_id, int status);

	/*! Brief Called by BlabbleCall when a call begins or ends ringing.
	 */
	void OnCallRingChange(const BlabbleCallPtr& call, const pjsua_call_info& info);
	
	/*! @Brief Called by BlabbleCall when a call is ended by this side.
	 *  @sa OnRemoteCallEnd
	 */
	void OnCallEnd(const BlabbleCallPtr& call);
	
	/*! @Brief Called by BlabbleCall when a call is ended by the other side.
	 *  @sa OnCallEnd
	 */
	void OnRemoteCallEnd(BlabbleCallPtr call, pjsua_call_id call_id, const pjsua_call_info& info);

	bool use_tls() const { return use_tls_; }
	void set_use_tls(bool v) { use_tls_ = v; }
	std::string server() const { return server_; }
	void set_server(const std::string &v) { server_ = v; }
	void set_username(const std::string &v) { username_ = v; }
	std::string username() { return username_; }
	void set_password(const std::string &v) { password_ = v; }
	int timeout() const { return timeout_; }
	void set_timeout(int v) { timeout_ = v; }
	int retry_interval() const { return retry_; }
	void set_retry_interval(int v) { retry_ = v; }
	void set_on_incoming_call(const FB::JSObjectPtr &v) { on_incoming_call_ = v; }
	void set_on_reg_state(const FB::JSObjectPtr &v) { on_reg_state_ = v; }
	void set_default_identity(const std::string &i) { default_identity = i; }
	PjsuaManagerPtr GetManager();

private:
	pjsua_acc_id id_;
	std::string server_; //!< Server's IP or DNS name
	std::string default_identity;
	bool use_tls_;
	unsigned long ringing_call_;
	PjsuaManagerWeakPtr pjsua_manager_;
	boost::recursive_mutex calls_mutex_;
	BlabbleCallList calls_;
	std::string username_, password_;
	int timeout_, retry_;

	//Callback methods
	FB::JSObjectPtr on_incoming_call_;
	FB::JSObjectPtr on_reg_state_;

	BlabbleAccountPtr get_shared() { return boost::static_pointer_cast<BlabbleAccount>(this->shared_from_this()); }
	BlabbleCallPtr FindCall(pjsua_call_id call_id);
};

#endif // H_BlabbleAccount
