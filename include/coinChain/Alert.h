#ifndef ALERT_H
#define ALERT_H

#include <string>
#include <vector>

#include "coin/serialize.h"
#include "coin/uint256.h"
#include "coin/util.h"

/// Alerts are for notifying old versions if they become too obsolete and
/// need to upgrade.  The message is displayed in the status bar.
/// Alert messages are broadcast as a vector of signed data.  Unserializing may
/// not read the entire buffer if the alert is for a newer version, but older
/// versions can still relay the original data.

class UnsignedAlert
{
public:
    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->_version);
        nVersion = this->_version;
        READWRITE(_relay_until);
        READWRITE(_expiration);
        READWRITE(_id);
        READWRITE(_cancel);
        READWRITE(_cancels);
        READWRITE(_min_version);
        READWRITE(_max_version);
        READWRITE(_subversions);
        READWRITE(_priority);

        READWRITE(_comment);
        READWRITE(_status_bar);
        READWRITE(_reserved);
    )

    void setNull()
    {
        _version = 1;
        _relay_until = 0;
        _expiration = 0;
        _id = 0;
        _cancel = 0;
        _cancels.clear();
        _min_version = 0;
        _max_version = 0;
        _subversions.clear();
        _priority = 0;

        _comment.clear();
        _status_bar.clear();
        _reserved.clear();
    }

    const int getPriority() const { return _priority; }
    
    const int64 until() const { return _relay_until; }
    
    std::string toString() const;

    void print() const
    {
        printf("%s", toString().c_str());
    }
    
    const std::string& getStatusBar() const { return _status_bar; }
    
protected:
    int _version;
    int64 _relay_until;      // when newer nodes stop relaying to newer nodes
    int64 _expiration;
    int _id;
    int _cancel;
    std::set<int> _cancels;
    int _min_version;            // lowest version inclusive
    int _max_version;            // highest version inclusive
    std::set<std::string> _subversions;  // empty matches all
    int _priority;
    
    // Actions
    std::string _comment;
    std::string _status_bar;
    std::string _reserved;
};

class Alert;

extern std::map<uint256, Alert> mapAlerts;

class Peer;

class Alert : public UnsignedAlert
{
public:
    Alert()
    {
        setNull();
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(_message);
        READWRITE(_signature);
    )

    void setNull()
    {
        UnsignedAlert::setNull();
        _message.clear();
        _signature.clear();
    }

    bool isNull() const
    {
        return (_expiration == 0);
    }

    uint256 getHash() const
    {
        return SerializeHash(*this);
    }

    bool isInEffect() const
    {
        return (GetAdjustedTime() < _expiration);
    }

    bool cancels(const Alert& alert) const
    {
        if (!isInEffect())
            return false; // this was a no-op before 31403
        return (alert._id <= _cancel || _cancels.count(alert._id));
    }

    bool appliesTo(int version, std::string subversion) const
    {
        return (isInEffect() &&
                _min_version <= _version && _version <= _max_version &&
                (_subversions.empty() || _subversions.count(subversion)));
    }

    bool appliesToMe() const
    {
        return appliesTo(VERSION, ::pszSubVer);
    }

    bool relayTo(Peer* peer) const;

    bool checkSignature();

    bool processAlert();
    
private:
    std::vector<unsigned char> _message;
    std::vector<unsigned char> _signature;
};

#endif // ALERT_H