/**********************************************************\
Original Author: Andrew Ofisher (zaltar)

License:    GNU General Public License, version 3.0
            http://www.gnu.org/licenses/gpl-3.0.txt

Copyright 2012 Andrew Ofisher
\**********************************************************/

#include "PjsuaManager.h"
#include "variant_list.h"
#include "DOM/Document.h"

#include "BlabbleAPI.h"
#include "BlabbleAccount.h"
#include "PjsuaManager.h"
#include "BlabbleAudioManager.h"
#include "BlabbleLogging.h"

BlabbleAPIInvalid::BlabbleAPIInvalid(const char* err)
{ 
	error_ = std::string(err);
	registerProperty("error", make_property(this, &BlabbleAPIInvalid::error)); 
}

BlabbleAPI::BlabbleAPI(const FB::BrowserHostPtr& host, const PjsuaManagerPtr& manager) :
	browser_host_(host), manager_(manager)
{
	registerMethod("createAccount", make_method(this, &BlabbleAPI::CreateAccount));
	registerMethod("playWav", make_method(this, &BlabbleAPI::PlayWav));
	registerMethod("stopWav", make_method(this, &BlabbleAPI::StopWav));
	registerMethod("log", make_method(this, &BlabbleAPI::Log));
	registerProperty("tlsEnabled", make_property(this, &BlabbleAPI::has_tls));

	registerMethod("getAudioDevices", make_method(this, &BlabbleAPI::GetAudioDevices));
	registerMethod("setAudioDevice", make_method(this, &BlabbleAPI::SetAudioDevice));
	registerMethod("getCurrentAudioDevice", make_method(this, &BlabbleAPI::GetCurrentAudioDevice));
	registerMethod("getVolume", make_method(this, &BlabbleAPI::GetVolume));
	registerMethod("setVolume", make_method(this, &BlabbleAPI::SetVolume));
	registerMethod("getSignalLevel", make_method(this, &BlabbleAPI::GetSignalLevel));
}

BlabbleAPI::~BlabbleAPI()
{
	std::vector<BlabbleAccountWeakPtr>::iterator it;
	for (it = accounts_.begin(); it < accounts_.end(); it++) {
		BlabbleAccountPtr acct = it->lock();
		if (acct) {
			acct->Destroy();
		}
	}
	accounts_.clear();
}

void BlabbleAPI::Log(int level, const std::wstring& msg)
{
	BLABBLE_JS_LOG(level, msg);
}

BlabbleAccountWeakPtr BlabbleAPI::CreateAccount(const FB::VariantMap &params)
{
	BlabbleAccountPtr account = boost::make_shared<BlabbleAccount>(manager_);
	try 
	{
		FB::VariantMap::const_iterator iter = params.find("host");
		if (iter != params.end())
			account->set_server(iter->second.cast<std::string>());

		if ((iter = params.find("username")) != params.end())
			account->set_username(iter->second.cast<std::string>());

		if ((iter = params.find("password")) != params.end())
			account->set_password(iter->second.cast<std::string>());

		if ((iter = params.find("useTls")) != params.end() &&
			iter->second.is_of_type<bool>())
		{
			account->set_use_tls(iter->second.cast<bool>());
		}

		if ((iter = params.find("identity")) != params.end() &&
			iter->second.is_of_type<std::string>())
		{
			account->set_default_identity(iter->second.cast<std::string>());
		}

		if ((iter = params.find("onIncomingCall")) != params.end() &&
			iter->second.is_of_type<FB::JSObjectPtr>())
		{
			account->set_on_incoming_call(iter->second.cast<FB::JSObjectPtr>());
		}

		if ((iter = params.find("onRegState")) != params.end() &&
			iter->second.is_of_type<FB::JSObjectPtr>())
		{
			account->set_on_reg_state(iter->second.cast<FB::JSObjectPtr>());
		}
		
		account->Register();
	}
	catch (const std::exception &e)
	{
		throw FB::script_error(std::string("Unable to create account: ") + e.what());
	}

	accounts_.push_back(account);
	return BlabbleAccountWeakPtr(account);
}

BlabbleAccountPtr BlabbleAPI::FindAcc(int acc_id)
{
	return manager_->FindAcc(acc_id);
}

bool BlabbleAPI::PlayWav(const std::string& fileName)
{
	return manager_->audio_manager()->StartWav(fileName);
}

void BlabbleAPI::StopWav()
{
	manager_->audio_manager()->StopWav();
}

FB::VariantList BlabbleAPI::GetAudioDevices()
{
	unsigned int count = pjmedia_aud_dev_count();
	pjmedia_aud_dev_info* audio_info = new pjmedia_aud_dev_info[count];
	pj_status_t status = pjsua_enum_aud_devs(audio_info, &count);
	if (status == PJ_SUCCESS) {
		FB::VariantList devices;
		for (unsigned int i = 0; i < count; i++)
		{
			FB::VariantMap map;
			map["name"] = std::string(audio_info[i].name);
			map["driver"] = std::string(audio_info[i].driver);
			map["inputs"] = audio_info[i].input_count;
			map["outputs"] = audio_info[i].output_count;
			map["id"] = i;
			devices.push_back(map);
		}
		delete[] audio_info;
		return devices;
	}
	delete[] audio_info;
	return FB::VariantList();
}

FB::VariantMap BlabbleAPI::GetCurrentAudioDevice()
{
	int captureId, playbackId;
	FB::VariantMap map, capInfo, playInfo;
	pjmedia_aud_dev_info audio_info;

	pj_status_t status = pjsua_get_snd_dev(&captureId, &playbackId);
	if (status == PJ_SUCCESS)
	{
		status = pjmedia_aud_dev_get_info(captureId, &audio_info);
		if (status == PJ_SUCCESS) {
			capInfo["name"] = std::string(audio_info.name);
			capInfo["driver"] = std::string(audio_info.driver);
			capInfo["inputs"] = audio_info.input_count;
			capInfo["outputs"] = audio_info.output_count;
			capInfo["id"] = captureId;
			map["capture"] = capInfo;
		}
		status = pjmedia_aud_dev_get_info(playbackId, &audio_info);
		if (status == PJ_SUCCESS) {
			playInfo["name"] = std::string(audio_info.name);
			playInfo["driver"] = std::string(audio_info.driver);
			playInfo["inputs"] = audio_info.input_count;
			playInfo["outputs"] = audio_info.output_count;
			playInfo["id"] = playbackId;
			map["playback"] = playInfo;
		}
	} else {
		map["error"] = status;
	}
	return map;
}

bool BlabbleAPI::SetAudioDevice(int capture, int playback)
{
	return PJ_SUCCESS == pjsua_set_snd_dev(capture, playback);
}

FB::VariantMap BlabbleAPI::GetVolume()
{
	pjmedia_conf_port_info info;
	FB::VariantMap map;
	pj_status_t status = pjmedia_conf_get_port_info(pjsua_var.mconf, 0, &info);
	if (status == PJ_SUCCESS)
	{
		map["outgoingVolume"] = ((float)info.rx_adj_level) / 128 + 1;
		map["incomingVolume"] = ((float)info.tx_adj_level) / 128 + 1;
	} 
	else 
	{
		map["error"] = status;
	}

	return map;
}

void BlabbleAPI::SetVolume(FB::variant outgoingVolume, FB::variant incomingVolume)
{
	if (outgoingVolume.is_of_type<double>())
	{
		pjsua_conf_adjust_rx_level(0, (float)outgoingVolume.cast<double>());
	}
	if (incomingVolume.is_of_type<double>())
	{
		pjsua_conf_adjust_tx_level(0, (float)incomingVolume.cast<double>());
	}
}

FB::VariantMap BlabbleAPI::GetSignalLevel()
{
	unsigned int txLevel, rxLevel;
	FB::VariantMap map;
	pj_status_t status = pjsua_conf_get_signal_level(0, &txLevel, &rxLevel);
	if (status == PJ_SUCCESS)
	{
		map["outgoingLevel"] = rxLevel;
		map["incomingLevel"] = txLevel;
	} 
	else 
	{
		map["error"] = status;
	}

	return map;
}