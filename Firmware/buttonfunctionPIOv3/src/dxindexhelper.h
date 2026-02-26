#ifndef DX_INDEX_HELPER_H_FILE
#define DX_INDEX_HELPER_H_FILE
#include <Arduino.h>

#include "dxConfigMain.h"
class DxIndexHelper
{
private:
    uint8_t _index = 0; //this is counting from 1, index == 0 means no dx activate

public:
    static bool isOk(uint8_t new_index) {return new_index > 0 && new_index <= NUM_DX;}

    uint8_t getIndex()           {return _index;}
    uint8_t getArrayIndex()      {return getNewArrayIx(_index);}

    static uint8_t getNewArrayIx(uint8_t new_index)   
                                            {if (isOk(new_index))
                                            {
                                                return new_index-1;
                                            }
                                            else return 0;
                                            }

    void setIndex(uint8_t new_index)  {if (new_index <= NUM_DX) _index = new_index;}
  
};



#endif