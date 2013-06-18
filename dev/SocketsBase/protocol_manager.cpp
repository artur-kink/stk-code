#include "protocol_manager.hpp"

#include "protocol.hpp"

#include <assert.h>
#include <stdio.h>


ProtocolManager::ProtocolManager() 
{
}

ProtocolManager::~ProtocolManager()
{
}

void ProtocolManager::messageReceived(uint8_t* data)
{
    assert(data);
    m_messagesToProcess.push_back(data); 
}

void ProtocolManager::runProtocol(Protocol* protocol)
{
    ProtocolInfo protocolInfo;
    protocolInfo.paused = false;
    protocolInfo.protocol = protocol;
    m_protocols.push_back(protocolInfo);
    protocol->setListener(this);
    protocol->setup();
    protocol->start();
    printf("*** PROTOCOL MANAGER *** - A new protocol has been started. There are %ld protocols running.\n", m_protocols.size());
}
void ProtocolManager::stopProtocol(Protocol* protocol)
{
    
}
void ProtocolManager::pauseProtocol(Protocol* protocol)
{
    for (unsigned int i = 0; i < m_protocols.size(); i++)
    {
        if (m_protocols[i].protocol == protocol)
        {
            m_protocols[i].paused = true;
            m_protocols[i].protocol->pause();
        }
    }
}
void ProtocolManager::unpauseProtocol(Protocol* protocol)
{
    for (unsigned int i = 0; i < m_protocols.size(); i++)
    {
        if (m_protocols[i].protocol == protocol && m_protocols[i].paused == true)
        {
            m_protocols[i].paused = false;
            m_protocols[i].protocol->unpause();
        }
    }
}
void ProtocolManager::protocolTerminated(Protocol* protocol)
{
    int offset = 0;
    for (unsigned int i = 0; i < m_protocols.size(); i++)
    {
        if (m_protocols[i-offset].protocol == protocol)
        {
            delete m_protocols[i].protocol;
            m_protocols.erase(m_protocols.begin()+(i-offset), m_protocols.begin()+(i-offset)+1);
            offset++;
        }
    }
    printf("*** PROTOCOL MANAGER *** - A protocol has been terminated. There are %ld protocols running.\n", m_protocols.size());
}

void ProtocolManager::update()
{
    // before updating, notice protocols that they have received information
    int size = m_messagesToProcess.size();
    for (int i = 0; i < size; i++)
    {
        uint8_t* data = m_messagesToProcess.back();
        PROTOCOL_TYPE searchedProtocol = (PROTOCOL_TYPE)(data[0]);
        for (unsigned int i = 0; i < m_protocols.size() ; i++)
        {
            if (m_protocols[i].protocol->getProtocolType() == searchedProtocol) // pass data to them even when paused
                m_protocols[i].protocol->messageReceived(data+1);
        }
        m_messagesToProcess.pop_back();
    }
    // now update all protocols
    for (unsigned int i = 0; i < m_protocols.size(); i++)
    {
        if (m_protocols[i].paused == false)
            m_protocols[i].protocol->update();
    }
}

int ProtocolManager::runningProtocolsCount()
{
    return m_protocols.size();
}

