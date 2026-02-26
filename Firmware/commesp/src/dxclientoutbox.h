#ifndef DX_CLIENT_OUTBOX_H_FILE
#define DX_CLIENT_OUTBOX_H_FILE
#include <Arduino.h>
#include <Preferences.h>
#include "commespcfg.h"

// #include <dxConfig.h>
//max length of a parameters/argument received, length of RX buffer per argument, also max len of string received by serial, max 2 strings
static const size_t MSG_NUM_CHARS = 250; 



// MQTT outbox enqueued msg
typedef struct {
    int mqtt_id;    // mqtt publish msg id
    int nvm_id;     // nvm msg id
    char message[MSG_NUM_CHARS];
    bool write_to_file;
} EnqueueMsgStruct;

class DxClientOutbox
{
private:
    Preferences _outbox;
    int _current_nvm_id = -1;
    uint32_t _lookup = 0; // Bitmask to track used IDs
    static const size_t _SIZE_LOOKUP = 32;

    static const size_t _ENQUEUED_BOX_SIZE = 32;
    EnqueueMsgStruct _enqbox[_ENQUEUED_BOX_SIZE] = {};
    uint8_t _enqueued_count = 0;
    uint32_t _enqueued_ts = 0;          // millis time of last enqueue action

    void _save_lookup()
    {
        size_t res = _outbox.putULong(PREF_KEY_OUTBOX_LU, _lookup);
        Serial.printf("saved lu,lu:%u,res:%d\n",_lookup, res);
    }

public:


    // ===========================================================================
    // configs for MQTT NVM outbox
    void open()
    {
        bool ok = _outbox.begin(PREF_NAMESPACE_OUTBOX, false);
        if (ok)
        {
            if (!_outbox.isKey(PREF_KEY_OUTBOX_LU))  _save_lookup();
            else
            {
                _lookup = _outbox.getULong(PREF_KEY_OUTBOX_LU,_lookup);
                // Serial.printf("opened space,name:%s,lu:%u\n",PREF_NAMESPACE_OUTBOX,_lookup);
                Serial.printf("load namepsace,opened:%s,free:%d,lu:%u\n",PREF_NAMESPACE_OUTBOX,_outbox.freeEntries(),_lookup);
            }
            
        }
        else
        {
            Serial.printf("could not open space,name:%s,lu:%u\n",PREF_NAMESPACE_OUTBOX,_lookup);
        }
    }

    void enqueueMsg(const char *msg, int mqtt_msg_id, bool write_to_file, int nvm_msg_id, bool verbose)
    {
        size_t len = strlen(msg);
        if (len <= 2)
        {
            Serial.printf("enqueue msg empty,len:%d,msg:%s\n",len,msg);
        }
        else if (_enqueued_count < _ENQUEUED_BOX_SIZE) 
        {
            _enqueued_ts = millis();
            _enqbox[_enqueued_count].mqtt_id = mqtt_msg_id;
            _enqbox[_enqueued_count].write_to_file = write_to_file;
            _enqbox[_enqueued_count].nvm_id = nvm_msg_id;
            strncpy(_enqbox[_enqueued_count].message, msg, MSG_NUM_CHARS);
            _enqueued_count++;
            if (verbose) Serial.printf("enqueued,mqtt_id:%d,nvm_id:%d,ts:%u,count:%u,\n",mqtt_msg_id,nvm_msg_id,_enqueued_ts,_enqueued_count);
        }
        else Serial.printf("enqueue box full cannot add,size:%d\n",_enqueued_count);
    }


    void onPublished(int mqtt_msg_id)
    {
        _enqueued_ts = millis();
        for (int i = 0; i < _enqueued_count; ++i) 
        {
            if (_enqbox[i].mqtt_id == mqtt_msg_id) 
            {
                Serial.printf("PUBLISHED,msg_id:%d,outbox_cnt:%d, msg:--\n",mqtt_msg_id,_enqueued_count-1);
                Serial.printf(_enqbox[i].message);
                if (_enqbox[i].nvm_id != -1) clearId(_enqbox[i].nvm_id);
                // Shift remaining messages
                for (int j = i; j < _enqueued_count - 1; ++j)
                {
                    _enqbox[j] = _enqbox[j + 1];
                }
                _enqueued_count--;
                break;
            }
        }
    }

    void enqueueBoxToStore()
    {
        if (_enqueued_count == 0)
        {
            Serial.println("no queue msgs to move to NVM");
            return;
        }
        uint32_t cnt = 0;
        uint32_t queued = _enqueued_count;
        for (size_t ix = 0; ix < _enqueued_count; ix++)
        {
            if (_enqbox[ix].write_to_file)
            {
                storeMsg(_enqbox[ix].message);
                cnt++;
            }
        }
        _enqueued_count = 0;
        Serial.printf("moved queue to NVM,queued:%d,moved:%d\n",queued,cnt);
    }

    int8_t storeMsg(const char *msg)
    {
        int id = getOpenId();
        if (id == -1) {
            Serial.print("StoreMsg--No available ID\n");
            return -1;
        }
        char key[16];
        snprintf(key, sizeof(key), "msg_%d", id);
        size_t res = _outbox.putString(key, msg);
        size_t msg_len = strlen(msg);
        bool ok = res == msg_len;
        _lookup |= (1 << id); // Mark ID as used
        _save_lookup();
        if (!ok)
        {
            Serial.printf("store msg len difference,nvm_id:%d,res:%d,len:%d\n",id,res,msg_len);
        }
        // Serial.printf("stored msg,nvm_id:%d,res:%d,\n",id,_lookup);
        return id;
    }

    void clearId(int nvm_msg_id)
    {
        _lookup &= ~(1 << nvm_msg_id);
        Serial.printf("cleared nvm id,nvm_id:%d,lu:%u\n",nvm_msg_id,_lookup);
        _save_lookup();
    }

    void clearAll()
    {
        _lookup = 0;
        _enqueued_count = 0;
        Serial.print("cleared nvm ids and enqueued_count\n");
        _save_lookup();
        
    }

    void getMsg(int msg_id, char* buffer)
    {
        char key[16];
        snprintf(key, sizeof(key), "msg_%d", msg_id);
        String msg_str = "";
        if (_outbox.isKey(key))
        {
            msg_str = _outbox.getString(key, "");
            // Serial.printf("load msg, key:%s,id:%d,msg:%s\n",key, msg_id, msg_str.c_str());
        }
        else Serial.printf("get msg key not found, key:%s\n",key);

        msg_str.toCharArray(buffer,MSG_NUM_CHARS);
    }

    int getOpenId()
    {
        for (int i = 0; i < _SIZE_LOOKUP; i++)
        {
            if (!(_lookup & (1 << i))) 
            {
                return i;
            }
        }
        return -1; // No available ID
    }

    int getNextUnsentId()
    {
        for (int i = _current_nvm_id + 1; i < _SIZE_LOOKUP; i++)
        {
            if (_lookup & (1 << i)) 
            {
                _current_nvm_id = i;
                return i;
            }
        }
        _current_nvm_id = -1;
        return -1; // No more active IDs
    }

    uint8_t getNumUnsent()      
    {
        uint32_t num_unsent = 0;
        for (size_t ix = 0; ix < _SIZE_LOOKUP; ix++)
        {
            if (_lookup & (1 << ix)) num_unsent++;
        }
        
        return num_unsent;
    }
    
    uint8_t getNumEnqueued()    {return _enqueued_count;}
    

    bool isEnqueueBoxEmpty()                {return _enqueued_count == 0;}
    bool isEnqueueBoxFull()                 {return _enqueued_count >= (_ENQUEUED_BOX_SIZE-1);}
    bool isAnyUnsent()                      {return _lookup > 0;}
    bool isEnqueueTimeOut(uint32_t period)  {return millis() - _enqueued_ts > period;}


    void printLookUp()          {Serial.printf("outbox,lu:%u\n",_lookup);}
    void printNumUnsent()       
    {
        printLookUp(); 
        Serial.printf("outbox,enqueued:%d,unsent:%d\n",_enqueued_count,getNumUnsent());
        Serial.printf("outbox,isAnyUnsent:%d,isEnqueueBoxFull:%d\n",isAnyUnsent(),isEnqueueBoxFull());
    }
    void printMsg(int nvm_msg_id)
    {
        char buf[MSG_NUM_CHARS] = "";
        getMsg(nvm_msg_id, buf);
        Serial.printf("outbox,nvm_msg_id:%d,len:%d,msg:%s\n",nvm_msg_id,strlen(buf),buf);
    }


};


#endif