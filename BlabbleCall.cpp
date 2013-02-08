/**********************************************************\
Original Author: Andrew Ofisher (zaltar)

License:    GNU General Public License, version 3.0
            http://www.gnu.org/licenses/gpl-3.0.txt

Copyright 2012 Andrew Ofisher
\**********************************************************/

#include "BlabbleAudioManager.h"
#include "BlabbleCall.h"
#include "BlabbleAccount.h"
#include "Blabble.h"
#include "JSObject.h"
#include "variant_list.h"
#include "BlabbleLogging.h"
#include "FBWriteOnlyProperty.h"

/*! @Brief Static call counter to keep track of calls.
 */
unsigned int BlabbleCall::id_counter_ = 0;
unsigned int BlabbleCall::GetNextId()
{
	return ATOMIC_INCREMENT(&BlabbleCall::id_counter_);
}

BlabbleCall::BlabbleCall(const BlabbleAccountPtr& parent_account)
	: call_id_(-1), ringing_(false)
{
	if (parent_account) 
	{
		acct_id_ = parent_account->id();
		audio_manager_ = parent_account->GetManager()->audio_manager();
		parent_ = BlabbleAccountWeakPtr(parent_account);
	}
	else 
	{
		acct_id_ = -1;
	}
	
	id_ = BlabbleCall::GetNextId();
	BLABBLE_LOG_DEBUG("New call created. Global id: " << id_);

	registerMethod("answer", make_method(this, &BlabbleCall::Answer));
	registerMethod("hangup", make_method(this, &BlabbleCall::LocalEnd));
	registerMethod("hold", make_method(this, &BlabbleCall::Hold));
	registerMethod("unhold", make_method(this, &BlabbleCall::Unhold));
	registerMethod("sendDTMF", make_method(this, &BlabbleCall::SendDTMF));
	registerMethod("transferReplace", make_method(this, &BlabbleCall::TransferReplace));
	registerMethod("transfer", make_method(this, &BlabbleCall::Transfer));

	registerProperty("callerId", make_property(this, &BlabbleCall::caller_id));
	registerProperty("isActive", make_property(this, &BlabbleCall::is_active));
	registerProperty("status", make_property(this, &BlabbleCall::status));

	registerProperty("onCallConnected", make_write_only_property(this, &BlabbleCall::set_on_call_connected));
	registerProperty("onCallEnd", make_write_only_property(this, &BlabbleCall::set_on_call_end));
}

void BlabbleCall::StopRinging()
{
	if (ringing_)
	{
		ringing_ = false;
		audio_manager_->StopRings();
	}
}

void BlabbleCall::StartInRinging()
{
	if (!ringing_)
	{
		ringing_ = true;
		audio_manager_->StartInRing();
	}
}

void BlabbleCall::StartOutRinging()
{
	if (!ringing_)
	{
		ringing_ = true;
		audio_manager_->StartRing();
	}
}

//Ended by us
void BlabbleCall::LocalEnd()
{
	pjsua_call_id old_id = INTERLOCKED_EXCHANGE((volatile long *)&call_id_, (long)INVALID_CALL);
	if (old_id == INVALID_CALL || 
		old_id < 0 || old_id >= (long)pjsua_call_get_max_count())
	{
		return;
	}

	StopRinging();

	pjsua_call_info info;
	if (pjsua_call_get_info(old_id, &info) == PJ_SUCCESS &&
		info.conf_slot > 0) 
	{
		//Kill the audio
		pjsua_conf_disconnect(info.conf_slot, 0);
		pjsua_conf_disconnect(0, info.conf_slot);
	}

	pjsua_call_hangup(old_id, 0, NULL, NULL);
	
	if (on_call_end_)
	{
		BlabbleCallPtr call = get_shared();
		on_call_end_->getHost()->ScheduleOnMainThread(call, boost::bind(&BlabbleCall::CallOnCallEnd, call));
	}

	BlabbleAccountPtr p = parent_.lock();
	if (p)
		p->OnCallEnd(get_shared());
}

void BlabbleCall::CallOnCallEnd()
{
	on_call_end_->Invoke("", FB::variant_list_of(BlabbleCallWeakPtr(get_shared())));
}

void BlabbleCall::CallOnCallEnd(pjsip_status_code status)
{
	on_call_end_->Invoke("", FB::variant_list_of(BlabbleCallWeakPtr(get_shared()))(status));
}

void BlabbleCall::CallOnTransferStatus(int status)
{
	on_transfer_status_->Invoke("", FB::variant_list_of(BlabbleCallWeakPtr(get_shared()))(status));
}

//Ended by remote, could be becuase of an error
void BlabbleCall::RemoteEnd(const pjsua_call_info &info)
{
	pjsua_call_id old_id = INTERLOCKED_EXCHANGE((volatile long *)&call_id_, (long)INVALID_CALL);
	if (old_id == INVALID_CALL || 
		old_id < 0 || old_id >= (long)pjsua_call_get_max_count())
	{
		return;
	}

	StopRinging();

	//Kill the audio
	if (info.conf_slot > 0) 
	{
		pjsua_conf_disconnect(info.conf_slot, 0);
		pjsua_conf_disconnect(0, info.conf_slot);
	}

	pjsua_call_hangup(old_id, 0, NULL, NULL);

	if (info.last_status > 400)
	{
		std::string callerId;
		if (info.remote_contact.ptr == NULL)
		{
			callerId =std::string(info.remote_info.ptr, info.remote_info.slen);
		} 
		else
		{
			callerId = std::string(info.remote_contact.ptr, info.remote_contact.slen);
		}

		if (on_call_end_)
		{
			BlabbleCallPtr call = get_shared();
			on_call_end_->getHost()->ScheduleOnMainThread(call, boost::bind(&BlabbleCall::CallOnCallEnd, call, info.last_status));
		}
	} else {
		if (on_call_end_)
		{
			BlabbleCallPtr call = get_shared();
			on_call_end_->getHost()->ScheduleOnMainThread(call, boost::bind(&BlabbleCall::CallOnCallEnd, call));
		}
	}

	BlabbleAccountPtr p = parent_.lock();
	if (p)
		p->OnCallEnd(get_shared());
}

BlabbleCall::~BlabbleCall(void)
{
	BLABBLE_LOG_DEBUG("Call Deleted. Global id: " << id_);
	on_call_end_.reset();
	LocalEnd();
}

bool BlabbleCall::RegisterIncomingCall(pjsua_call_id call_id)
{
	BlabbleAccountPtr p = parent_.lock();
	if (!p)
		return false;

	if (call_id_ == INVALID_CALL && call_id != INVALID_CALL &&
		call_id >= 0 && call_id < (long)pjsua_call_get_max_count())
	{
		call_id_ = call_id;
		pjsua_call_set_user_data(call_id, &id_);

		/* Automatically answer incoming calls with 180/RINGING */
		pjsua_call_answer(call_id, 180, NULL, NULL);
		
		StartInRinging();
		return true;
	}

	return false;
}

pj_status_t BlabbleCall::MakeCall(const std::string& dest, const std::string& identity)
{
	BlabbleAccountPtr p = parent_.lock();
	if (!p)
		return false;

	if (call_id_ != INVALID_CALL)
		return false;

	pj_status_t status;
	pj_str_t desturi;
	desturi.ptr = const_cast<char*>(dest.c_str());
	desturi.slen = dest.length();

	if (!identity.empty())
	{
		pjsua_msg_data msgData;
		pjsua_msg_data_init(&msgData);
		pjsip_generic_string_hdr cidHdr;
		pj_str_t name = pj_str(const_cast<char*>("P-Asserted-Identity"));
		pj_str_t value = pj_str(const_cast<char*>(identity.c_str()));

		pjsip_generic_string_hdr_init2(&cidHdr, &name, &value);
		pj_list_push_back(&msgData.hdr_list, &cidHdr);

		status = pjsua_call_make_call(acct_id_, &desturi, 0,
			&id_, &msgData, (pjsua_call_id*)&call_id_);
	}
	else 
	{
		status = pjsua_call_make_call(acct_id_, &desturi, 0,
			&id_, NULL, (pjsua_call_id*)&call_id_);
	}

	if (status == PJ_SUCCESS) 
	{
		destination_ = dest;
		StartOutRinging();
	} 

	return status;
}

bool BlabbleCall::Answer()
{
	BlabbleAccountPtr p = CheckAndGetParent();
	if (!p)
		return false;

	StopRinging();
	pj_status_t status = pjsua_call_answer(call_id_, 200, NULL, NULL);

	return status == PJ_SUCCESS;
}

bool BlabbleCall::Hold()
{
	BlabbleAccountPtr p = CheckAndGetParent();
	if (!p)
		return false;

	pj_status_t status = pjsua_call_set_hold(call_id_, NULL);
	return status == PJ_SUCCESS;
}

bool BlabbleCall::SendDTMF(const std::string& dtmf)
{
	BlabbleAccountPtr p;
	char c;
	pj_str_t digits;
	pj_status_t status;

	if (dtmf.length() != 1)
	{
		throw FB::script_error("SendDTMF may only send one character!");
	}
	
	c = dtmf[0];
	if (c != '#' && c != '*' && c < '0' && c > '9')
	{
		throw FB::script_error("SendDTMF may only send numbers, # and *");
	}

	if (!(p = CheckAndGetParent()))
		return false;

	digits.ptr = &c;
	digits.slen = 1;
	status = pjsua_call_dial_dtmf(call_id_, &digits);

	return status == PJ_SUCCESS;
}

bool BlabbleCall::Unhold()
{
	BlabbleAccountPtr p = CheckAndGetParent();
	if (!p)
		return false;

	pj_status_t status = pjsua_call_reinvite(call_id_, PJ_TRUE, NULL);
	return status == PJ_SUCCESS;
}

bool BlabbleCall::TransferReplace(const BlabbleCallPtr& otherCall)
{
	BlabbleAccountPtr p = CheckAndGetParent();
	if (!p)
		return false;

	if (!otherCall)
		return false;

	pj_status_t status = pjsua_call_xfer_replaces(call_id_, otherCall->call_id_, 
		PJSUA_XFER_NO_REQUIRE_REPLACES, NULL);
	if (status == PJ_SUCCESS)
	{
		LocalEnd();
	}

	return status == PJ_SUCCESS;
}

bool BlabbleCall::Transfer(const FB::VariantMap &params)
{
	std::string destination;
	pj_str_t desturi;
	BlabbleAccountPtr p = parent_.lock();
	if (!p)
		return false;

	if (call_id_ == INVALID_CALL)
		return false;

	FB::VariantMap::const_iterator iter = params.find("destination");
	if (iter == params.end())
	{
		throw FB::script_error("No destination given!");
	}
	else
	{
		destination = iter->second.cast<std::string>();
		if (destination.size() <= 4 || destination.substr(0, 4) != "sip:")
			destination = "sip:" + destination + "@" + p->server();

		if (p->use_tls() && destination.find("transport=TLS") == std::string::npos)
			destination += ";transport=TLS";
	}

	if ((iter = params.find("onStatusChange")) != params.end() &&
		iter->second.is_of_type<FB::JSObjectPtr>())
	{
		set_on_transfer_status(iter->second.cast<FB::JSObjectPtr>());
	}

	desturi.ptr = const_cast<char*>(destination.c_str());
	desturi.slen = destination.length();
	return pjsua_call_xfer(call_id_, &desturi, NULL) == PJ_SUCCESS;
}

std::string BlabbleCall::caller_id()
{
	BlabbleAccountPtr p = CheckAndGetParent();
	if (!p)
		return "INVALID CALL";

	pjsua_call_info info;
	if (pjsua_call_get_info(call_id_, &info) == PJ_SUCCESS) 
	{
		return std::string(info.remote_contact.ptr, info.remote_contact.slen);
	}
	return "";
}

bool BlabbleCall::get_valid()
{
	return (bool)CheckAndGetParent();
}

FB::VariantMap BlabbleCall::status()
{
	FB::VariantMap map = FB::VariantMap();
	map["state"] = (int)CALL_INVALID;
	pjsua_call_info info;
	pj_status_t status;

	if (call_id_ != INVALID_CALL &&
		(status = pjsua_call_get_info(call_id_, &info)) == PJ_SUCCESS)
	{
		if (info.media_status == PJSUA_CALL_MEDIA_LOCAL_HOLD ||
			info.media_status == PJSUA_CALL_MEDIA_REMOTE_HOLD)
		{
				map["state"] = (int)CALL_HOLD;
				map["callerId"] = std::string(info.remote_contact.ptr, info.remote_contact.slen);
		} 
		else if (info.media_status == PJSUA_CALL_MEDIA_ACTIVE ||
			info.media_status == PJSUA_CALL_MEDIA_ERROR ||
			(info.media_status == PJSUA_CALL_MEDIA_NONE && 
				info.state == PJSIP_INV_STATE_CONFIRMED))
		{
			map["state"] = (int)CALL_ACTIVE;
			map["duration"] = info.connect_duration.sec;
			map["callerId"] = std::string(info.remote_contact.ptr, info.remote_contact.slen);
		}
		else if (info.media_status == PJSUA_CALL_MEDIA_NONE &&
			(info.state == PJSIP_INV_STATE_CALLING ||
				info.state == PJSIP_INV_STATE_INCOMING ||
				info.state == PJSIP_INV_STATE_EARLY))
		{
			map["state"] = info.state == PJSIP_INV_STATE_INCOMING ? (int)CALL_RINGING_IN : (int)CALL_RINGING_OUT;
			map["duration"] = info.connect_duration.sec;
			if (info.remote_contact.ptr == NULL)
			{
				map["callerId"] = std::string(info.remote_info.ptr, info.remote_info.slen);
			} 
			else
			{
				map["callerId"] = std::string(info.remote_contact.ptr, info.remote_contact.slen);
			}
		} 
	}

	return map;
}

void BlabbleCall::OnCallMediaState()
{
	if (call_id_ == INVALID_CALL)
		return;

	pjsua_call_info info;
	pj_status_t status;
	if ((status = pjsua_call_get_info(call_id_, &info)) != PJ_SUCCESS) {
		BLABBLE_LOG_ERROR("Unable to get call info. PJSIP call id: " << call_id_ << ", global id: " << id_ << ", pjsua_call_get_info returned " << status);
		StopRinging();
		return;
	}

	if (info.media_status == PJSUA_CALL_MEDIA_ACTIVE) 
	{
		StopRinging();

		// When media is active, connect call to sound device.
		pjsua_conf_connect(info.conf_slot, 0);
		pjsua_conf_connect(0, info.conf_slot);
	}
}

void BlabbleCall::OnCallState(pjsua_call_id call_id, pjsip_event *e)
{
	pjsua_call_info info;
	if (pjsua_call_get_info(call_id, &info) == PJ_SUCCESS)
	{
		if (info.state == PJSIP_INV_STATE_DISCONNECTED) 
		{
			RemoteEnd(info);
		}
		else if (info.state == PJSIP_INV_STATE_CALLING)
		{
			if (on_call_ringing_)
				on_call_ringing_->InvokeAsync("", FB::variant_list_of(BlabbleCallWeakPtr(get_shared())));

			BlabbleAccountPtr p = parent_.lock();
			if (p)
				p->OnCallRingChange(get_shared(), info);
		}
		else if (info.state == PJSIP_INV_STATE_CONFIRMED)
		{
			if (on_call_connected_)
				on_call_connected_->InvokeAsync("", FB::variant_list_of(BlabbleCallWeakPtr(get_shared())));

			BlabbleAccountPtr p = parent_.lock();
			if (p)
				p->OnCallRingChange(get_shared(), info);
		}
	}
}

bool BlabbleCall::OnCallTransferStatus(int status)
{
	if (call_id_ != INVALID_CALL) 
	{
		if (on_transfer_status_)
		{
			BlabbleCallPtr call = get_shared();
			on_transfer_status_->getHost()->ScheduleOnMainThread(call, boost::bind(&BlabbleCall::CallOnTransferStatus, call, status));
		}
	}

	return false;
}

BlabbleAccountPtr BlabbleCall::CheckAndGetParent()
{
	if (call_id_ != INVALID_CALL)
	{
		return parent_.lock();
	}

	return BlabbleAccountPtr();
}

bool BlabbleCall::is_active()
{ 
	return ((bool)CheckAndGetParent() && pjsua_call_is_active(call_id_)); 
}
