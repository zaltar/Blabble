/**********************************************************\
Original Author: Andrew Ofisher (zaltar)

License:    GNU General Public License, version 3.0
            http://www.gnu.org/licenses/gpl-3.0.txt

Copyright 2012 Andrew Ofisher
\**********************************************************/

#include "PjsuaManager.h"
#include "Blabble.h"
#include "BlabbleAccount.h"
#include "BlabbleCall.h"
#include "BlabbleAudioManager.h"
#include "BlabbleLogging.h"

PjsuaManagerWeakPtr PjsuaManager::instance_;

PjsuaManagerPtr PjsuaManager::GetManager(const std::string& path, bool enableIce,
		const std::string& stunServer)
{
	PjsuaManagerPtr tmp = instance_.lock();
	if(!tmp) 
	{ 
		tmp = PjsuaManagerPtr(new PjsuaManager(path, enableIce, stunServer));
		instance_ = boost::weak_ptr<PjsuaManager>(tmp);
	}
	
	return tmp;
}

PjsuaManager::PjsuaManager(const std::string& executionPath, bool enableIce,
		const std::string& stunServer)
{
	pj_status_t status;
	pjsua_config cfg;
	pjsua_logging_config log_cfg;
	pjsua_media_config media_cfg;
	pjsua_transport_config tran_cfg, tls_tran_cfg;

	pjsua_transport_config_default(&tran_cfg);
	pjsua_transport_config_default(&tls_tran_cfg);
	pjsua_media_config_default(&media_cfg);
	pjsua_config_default(&cfg);
	pjsua_logging_config_default(&log_cfg);

	cfg.max_calls = 511;

	cfg.cb.on_incoming_call = &PjsuaManager::OnIncomingCall;
	cfg.cb.on_call_media_state = &PjsuaManager::OnCallMediaState;
	cfg.cb.on_call_state = &PjsuaManager::OnCallState;
	cfg.cb.on_reg_state = &PjsuaManager::OnRegState;
	cfg.cb.on_transport_state = &PjsuaManager::OnTransportState;
	cfg.cb.on_call_transfer_status = &PjsuaManager::OnCallTransferStatus;

	log_cfg.console_level = 4;
	log_cfg.level = 4;
	log_cfg.msg_logging = PJ_FALSE;
	log_cfg.decor = PJ_LOG_HAS_SENDER | PJ_LOG_HAS_SPACE | PJ_LOG_HAS_LEVEL_TEXT;
	log_cfg.cb = BlabbleLogging::blabbleLog;

	tls_tran_cfg.port = 0;
	//tran_cfg.tls_setting.verify_server = PJ_TRUE;
	tls_tran_cfg.tls_setting.timeout.sec = 5;
	tls_tran_cfg.tls_setting.method = PJSIP_TLSV1_METHOD;
	tran_cfg.port = 0;

	media_cfg.no_vad = 1;
	media_cfg.enable_ice = enableIce ? PJ_TRUE : PJ_FALSE;
	if (!stunServer.empty()) 
	{
		cfg.stun_srv_cnt = 1;
		cfg.stun_srv[0] = pj_str(const_cast<char*>(stunServer.c_str()));
	}

	status = pjsua_create();
	if (status != PJ_SUCCESS)
		throw std::runtime_error("pjsua_create failed");

	status = pjsua_init(&cfg, &log_cfg, &media_cfg);
	if (status != PJ_SUCCESS) 
		throw std::runtime_error("Error in pjsua_init()");

	try
	{
		status = pjsua_transport_create(PJSIP_TRANSPORT_TLS, &tls_tran_cfg, &this->tls_transport);
		has_tls_ = status == PJ_SUCCESS;
		if (!has_tls_) {
			BLABBLE_LOG_DEBUG("Error in tls pjsua_transport_create. Tls will not be enabled");
			this->tls_transport = -1;
		}

		status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &tran_cfg, &this->udp_transport);
		if (status != PJ_SUCCESS)
			throw std::runtime_error("Error in pjsua_transport_create for UDP transport");

		status = pjsua_start();
		if (status != PJ_SUCCESS)
			throw std::runtime_error("Error in pjsua_start()");

		std::string path = executionPath;
		unsigned int tmp = path.find("plugins");
		if (tmp != std::string::npos)
		{
			path = path.substr(0, tmp + 7);
		}
		tmp = path.find("npBlabblePhone.");
		if (tmp != std::string::npos)
		{
			path = path.substr(0, tmp - 1);
		}
		audio_manager_ = boost::make_shared<BlabbleAudioManager>(path);

		BLABBLE_LOG_DEBUG("PjsuaManager startup complete.");
	}
	catch (std::runtime_error& e)
	{
		BLABBLE_LOG_ERROR("Error in PjsuaManager. " << e.what());
		pjsua_destroy();
		throw e;
	}
}

PjsuaManager::~PjsuaManager()
{
	pjsua_call_hangup_all();

	accounts_.clear();

	if (audio_manager_)
		audio_manager_.reset();

	pjsua_destroy();
}

void PjsuaManager::AddAccount(const BlabbleAccountPtr &account)
{
	if (account->id() == INVALID_ACCOUNT)
		throw std::runtime_error("Attempt to add uninitialized account.");

	accounts_[account->id()] = account;
}

void PjsuaManager::RemoveAccount(pjsua_acc_id acc_id)
{
	accounts_.erase(acc_id);
}

BlabbleAccountPtr PjsuaManager::FindAcc(int acc_id)
{
	if (pjsua_acc_is_valid(acc_id) == PJ_TRUE) 
	{
		BlabbleAccountMap::iterator it = accounts_.find(acc_id);
		if (it != accounts_.end()) 
		{
			return it->second;
		}
	}

	return BlabbleAccountPtr();
}

//Event handlers

//Static
void PjsuaManager::OnTransportState(pjsip_transport *tp, pjsip_transport_state state, 
	const pjsip_transport_state_info *info)
{
	if (state == PJSIP_TP_STATE_DISCONNECTED) 
	{
		pjsip_tls_state_info *tmp = ((pjsip_tls_state_info*)info->ext_info);
		if (tmp->ssl_sock_info->verify_status != PJ_SSL_CERT_ESUCCESS) 
		{
			//bad cert
		}
	}
}

//Static
void PjsuaManager::OnIncomingCall(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata)
{
	BLABBLE_LOG_TRACE("OnIncomingCall called for PJSIP account id: " << acc_id << 
		", PJSIP call id: " << call_id);
	PjsuaManagerPtr manager = PjsuaManager::instance_.lock();

	if (!manager)
	{
		//How is this even possible?
		pjsua_call_hangup(call_id, 0, NULL, NULL);
		return;
	}

	BlabbleAccountPtr acc = manager->FindAcc(acc_id);
	if (acc && acc->OnIncomingCall(call_id, rdata))
	{
		return;
	}
	
	//Otherwise we respond busy if no one wants the call
	pjsua_call_hangup(call_id, 486, NULL, NULL);
}

//Static
void PjsuaManager::OnCallMediaState(pjsua_call_id call_id)
{
	PjsuaManagerPtr manager = PjsuaManager::instance_.lock();

	if (!manager)
		return;
	
	pjsua_call_info info;
	pj_status_t status;
	if ((status = pjsua_call_get_info(call_id, &info)) == PJ_SUCCESS) 
	{
		BLABBLE_LOG_TRACE("PjsuaManager::OnCallMediaState called with PJSIP call id: " 
			<< call_id << ", state: " << info.state);
		BlabbleAccountPtr acc = manager->FindAcc(info.acc_id);
		if (acc)
		{
			acc->OnCallMediaState(call_id);
		}
	}
	else
	{
		BLABBLE_LOG_ERROR("PjsuaManager::OnCallMediaState failed to call pjsua_call_get_info for PJSIP call id: " 
			<< call_id << ", got status: " << status);
	}

}

//Static
void PjsuaManager::OnCallState(pjsua_call_id call_id, pjsip_event *e)
{
	PjsuaManagerPtr manager = PjsuaManager::instance_.lock();

	if (!manager)
		return;

	pjsua_call_info info;
	pj_status_t status;
	if ((status = pjsua_call_get_info(call_id, &info)) == PJ_SUCCESS) 
	{
		BLABBLE_LOG_TRACE("PjsuaManager::OnCallState called with PJSIP call id: " 
			<< call_id << ", state: " << info.state);
		BlabbleAccountPtr acc = manager->FindAcc(info.acc_id);
		if (acc)
		{
			acc->OnCallState(call_id, e);
		}

		if (info.state == PJSIP_INV_STATE_DISCONNECTED)
		{
			//Just make sure we get rid of the call
			pjsua_call_hangup(call_id, 0, NULL, NULL);
		}
	}
	else
	{
		BLABBLE_LOG_ERROR("PjsuaManager::OnCallState failed to call pjsua_call_get_info for PJSIP call id: " 
			<< call_id << ", got status: " << status);
	}
}

//Static
void PjsuaManager::OnRegState(pjsua_acc_id acc_id)
{
	PjsuaManagerPtr manager = PjsuaManager::instance_.lock();

	if (!manager)
		return;

	BlabbleAccountPtr acc = manager->FindAcc(acc_id);
	if (acc)
	{
		acc->OnRegState();
	}
	else
	{
		BLABBLE_LOG_ERROR("PjsuaManager::OnRegState failed to find account PJSIP account id: " 
			<< acc_id);
	}
}

//Static
void PjsuaManager::OnCallTransferStatus(pjsua_call_id call_id, int st_code, const pj_str_t *st_text, pj_bool_t final, pj_bool_t *p_cont)
{
	BLABBLE_LOG_TRACE("PjsuaManager::OnCallTransferState called with PJSIP call id: " 
		<< call_id << ", state: " << st_code);
	PjsuaManagerPtr manager = PjsuaManager::instance_.lock();

	if (!manager)
		return;

	pjsua_call_info info;
	pj_status_t status;
	if ((status = pjsua_call_get_info(call_id, &info)) == PJ_SUCCESS) 
	{
		BlabbleAccountPtr acc = manager->FindAcc(info.acc_id);
		if (acc)
		{
			if (acc->OnCallTransferStatus(call_id, st_code))
				(*p_cont) = PJ_TRUE;
		}
	}
	else
	{
		BLABBLE_LOG_ERROR("PjsuaManager::OnCallTransferStatus failed to call pjsua_call_get_info for PJSIP call id: " 
			<< call_id << ", got status: " << status);
	}
}
