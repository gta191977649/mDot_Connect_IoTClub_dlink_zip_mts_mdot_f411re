#include "mbed.h"
#include "mDot.h"
#include "MTSLog.h"
#include <string>
#include <vector>
#include <algorithm>

const char TURNON[] = "turnon";
const char TURNOFF[] = "turnoff";

const char TURN_ON_ALARM[] = "TurnOnAlarm";
const char TURN_OFF_ALARM[] = "TurnOffAlarm";

const char _header[] = "PH Alarm";

// these options must match the settings on your Conduit
// uncomment the following lines and edit their values to match your configuration
static std::string config_network_name = "1145141919810";
static std::string config_network_pass = "1145141919810";
static uint8_t config_frequency_sub_band = 7; 

bool strCmp(char* str1, const char* str2, int len)
{
    int i;
    for (i=0; i<len; i++)
    {
        if(str1[i]!=str2[i])return false;
    }
    return true;    
}


int main() {
    int32_t ret; 
    mDot* dot;
    std::vector<uint8_t> sendData, recvData;
    std::vector<uint8_t> turnonData,turnoffData;
    char sendBuf[11],recvBuf[30];
    int  i;

    // get a mDot handle
    dot = mDot::getInstance();

    // print library version information
    logInfo("version: %s", dot->getId().c_str());

    //*******************************************
    // configuration
    //*******************************************
    // reset to default config so we know what state we're in
    dot->resetConfig();

    dot->setLogLevel(mts::MTSLog::INFO_LEVEL);

    // set up the mDot with our network information: frequency sub band, network name, and network password
    // these can all be saved in NVM so they don't need to be set every time - see mDot::saveConfig()

    // frequency sub band is only applicable in the 915 (US) frequency band
    // if using a MultiTech Conduit gateway, use the same sub band as your Conduit (1-8) - the mDot will use the 8 channels in that sub band
    // if using a gateway that supports all 64 channels, use sub band 0 - the mDot will use all 64 channels
    logInfo("setting frequency sub band");
    if ((ret = dot->setFrequencySubBand(config_frequency_sub_band)) != mDot::MDOT_OK) {
        logError("failed to set frequency sub band %d:%s", ret, mDot::getReturnCodeString(ret).c_str());
    }

    logInfo("setting network name");
    if ((ret = dot->setNetworkName(config_network_name)) != mDot::MDOT_OK) {
        logError("failed to set network name %d:%s", ret, mDot::getReturnCodeString(ret).c_str());
    }

    logInfo("setting network password");
    if ((ret = dot->setNetworkPassphrase(config_network_pass)) != mDot::MDOT_OK) {
        logError("failed to set network password %d:%s", ret, mDot::getReturnCodeString(ret).c_str());
    }

    // a higher spreading factor allows for longer range but lower throughput
    // in the 915 (US) frequency band, spreading factors 7 - 10 are available
    // in the 868 (EU) frequency band, spreading factors 7 - 12 are available
    logInfo("setting TX spreading factor");
    if ((ret = dot->setTxDataRate(mDot::SF_10)) != mDot::MDOT_OK) {
        logError("failed to set TX datarate %d:%s", ret, mDot::getReturnCodeString(ret).c_str());
    }

    // request receive confirmation of packets from the gateway
    logInfo("enabling ACKs");
    if ((ret = dot->setAck(1)) != mDot::MDOT_OK) {
        logError("failed to enable ACKs %d:%s", ret, mDot::getReturnCodeString(ret).c_str());
    }

    // save this configuration to the mDot's NVM
    logInfo("saving config");
    if (! dot->saveConfig()) {
        logError("failed to save configuration");
    }
    //*******************************************
    // end of configuration
    //*******************************************

    // attempt to join the network
    logInfo("joining network");
    while ((ret = dot->joinNetwork()) != mDot::MDOT_OK) {
        logError("failed to join network %d:%s", ret, mDot::getReturnCodeString(ret).c_str());
        // in the 868 (EU) frequency band, we need to wait until another channel is available before transmitting again
        osDelay(std::max((uint32_t)1000, (uint32_t)dot->getNextTxMs()));
    }
    // format data for sending to the gateway
    for(i=0; i< strlen(TURNON); i++ )
               turnonData.push_back( TURNON[i] );
    
    for(i=0; i< strlen(TURNOFF); i++ )
               turnoffData.push_back( TURNOFF[i] );                    
    // format data for sending to the gateway
    for(i=0; i< strlen(_header); i++ )
               sendData.push_back( _header[i] );
               
    // send the data to the gateway
    if ((ret = dot->send(sendData)) != mDot::MDOT_OK) {
        logError("failed to send", ret, mDot::getReturnCodeString(ret).c_str());
    } else {
            logInfo("successfully sent data to gateway");
    } 
    while (true) {
       for(i=0;i<30;i++)recvBuf[i]=0;
       if ((ret = dot->send(turnonData)) != mDot::MDOT_OK) {
          logError("failed to send", ret, mDot::getReturnCodeString(ret).c_str());
       } else {
          logInfo("successfully sent data to gateway");
          recvData.clear();
          if ((ret = dot->recv(recvData)) != mDot::MDOT_OK) {
             logError("failed to recv: [%d][%s]", ret, mDot::getReturnCodeString(ret).c_str());
          } else {
             logInfo("datasize = %d", recvData.size());
             for( int i=0; i< recvData.size(); i++ )
                recvBuf[i] = recvData[i];
             logInfo("%s", recvBuf);
             if(strCmp(recvBuf, TURN_ON_ALARM, recvData.size())){
                 logInfo("Turn on the alarm");
                 std::vector<uint8_t> data;
                 data.push_back('1');
                 dot->send(data);
             }
             if(strCmp(recvBuf, TURN_OFF_ALARM, recvData.size())){
                 logInfo("Turn off the alarm");
                std::vector<uint8_t> data;
                 data.push_back('0');
                 dot->send(data);
             }
          } 
       } 
      
    }

}
