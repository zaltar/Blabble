/**********************************************************\
Original Author: Andrew Ofisher (zaltar)

License:    GNU General Public License, version 3.0
            http://www.gnu.org/licenses/gpl-3.0.txt

Copyright 2012 Andrew Ofisher
\**********************************************************/

#ifndef H_BlabbleAPI
#define H_BlabbleAPI

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
#include <pjsua-lib/pjsua_internal.h>
#include <pjmedia.h>
#include <pjmedia-codec.h> 

FB_FORWARD_PTR(BlabbleCall)
FB_FORWARD_PTR(BlabbleAccount)
FB_FORWARD_PTR(BlabbleAudioManager)
FB_FORWARD_PTR(PjsuaManager)

/*! @class
 *  @Brief The object to return if initialization of PJSIP fails for any reason.
 */
class BlabbleAPIInvalid : public FB::JSAPIAuto
{
public:
	BlabbleAPIInvalid(const char* err);

	bool virtual get_valid() { return false; }
	std::string error() { return error_; }
private:
	std::string error_;
};

/*! @class
 *  @Brief Serves as the JSAPIAuto class returned to each page the plugin is loaded on
 *
 *  This is the primary API for the JavaScript plugin object.  This object allows users
 *  to create accounts, check for TLS support, play/stop a busy signal, and log using
 *  the BlabbleLogger/log4cplus.
 *
 *  There will be one instance of this class for each plugin object in the browser.
 */
class BlabbleAPI : public FB::JSAPIAuto
{
public:
	BlabbleAPI(const FB::BrowserHostPtr& host, const PjsuaManagerPtr& manager);
	virtual ~BlabbleAPI();

	/*! @Brief Create and return a new account.
	 *  CreateAccount is called by JavaScript to register a new SIP account.
	 *  See the JavaScript documentation for more information.
	 */
	BlabbleAccountWeakPtr CreateAccount(const FB::VariantMap &params);
	
	/*! @Brief Unsupported. Plays a wave file.
	 */
	bool PlayWav(const std::string& fileName);
	
	/*! @Brief Unsupported. 
	 *  Stops playing a wave file preivously played by 
	 *  `PlayWav(fileName)`
	 */
	void StopWav();
	
	/*! @Brief Play a busy signal on the speakers.
	 *  When a SIP call returns an error code of busy it is left
	 *  to the user whether or not they want to play a busy signal
	 *  tone like a good old fashioned phone would.
	 */
	void PlayBusySignal();
	
	/*! @Brief Stops playing a busy signal.
	 *  @sa PlayBusySignal
	 */
	void StopBusySignal();
	
	/*! @Brief Allows JavaScript code to utilize BlabbleLogging
	 */
	void Log(int level, const std::wstring& msg);
	
	/*! @Brief JavaScript property to determine if TLS support is available
	 */
	bool has_tls() { return manager_->has_tls(); }

	/*! @Brief JavaScript function to return an array of audio devices in the system
	 */
	FB::VariantList GetAudioDevices();

	/*! @Brief JavaScript function to get the current audio device.
	 *  This will return a JavaScript object with "capture" and "playback"
	 *  properties set to the current audio device ids as can be
	 *  seen from GetAudioDevices().  If a failure occurrs for whatever
	 *  reason, the object will have an "error" property with the error
	 *  code return by PJSIP.
	 */
	FB::VariantMap GetCurrentAudioDevice();

	/*! @Brief JavaScript function to set the current audio device.
	 *  This function takes the device id as returned from GetAudioDevices().
	 *  Will return true on success, false otherwise.
	 */
	bool SetAudioDevice(int capture, int playback);

	/*! @Brief JavaScript function to get the current volume adjustment levels.
	 *  This function returns a JavaScript object with "outgoingVolume" and
	 *  "incomingVolume" properties. Their values are from 0 to 2 where 0 is
	 *  muted and 1 is no adjustment. If an error occurrs, the map will contain
	 *  an "error" property with the error code returned by PJSIP.
	 */
	FB::VariantMap GetVolume();

	/*! @Brief Javascript function to set the volume adjustment levels.
	 *  Either parameter can be null to not modify that level.  The level
	 *  passed in should be in the range of 0 to 2 where 0 is mute and 1 is 
	 *  no adjustment.
	 */
	void SetVolume(FB::variant outgoingVolume, FB::variant incomingVolume);

	/*! @Brief Javascript function to get the current signal levels.
	 *  This function returns a JavaScript object with "outgoingLevel"
	 *  and "incomingLevel" properties. The properties are ints with a range
	 *  of 0 to 255 with 0 meaning no signal.
	 */
	FB::VariantMap GetSignalLevel();

	//functions to retrieve objects from userdata
	BlabbleAccountPtr FindAcc(int accId);
private:
	FB::BrowserHostPtr browser_host_;
	PjsuaManagerPtr manager_;
	
	/*! @Brief Keep track of all account objects so we can 
	 *  destroy them when this plugin is destroyed
	 */
	std::vector<BlabbleAccountWeakPtr> accounts_;
};

#endif // H_BlabbleAPI
