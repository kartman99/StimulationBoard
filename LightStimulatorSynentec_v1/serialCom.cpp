//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#include <Arduino.h>
#include "serialCom.h"



//---------------------------------------------------------------------------
unsigned long myWaitCOM(unsigned long msTimeOut);

//---------------------------------------------------------------------------

const  unsigned long timeOut_ = 1000;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// all following are help functions
//---------------------------------------------------------------------------
inline char myCAP(const char cOne){
    return (cOne & 0xdf); // 0x61 -> 0x41
}

int myUsedChar(const char cOne){
    while (1){
        if (cOne == '\\')
            return 1; // '\\'
        if (cOne <= 'z'&&cOne >= 'a')
            return 2; // small char
        if (cOne <= 'Z'&&cOne >= 'A')
            return 3; // big char
        if (cOne <= '9'&&cOne >= '0')
            return 4; // number
        if (cOne == ',' || cOne == '.' || cOne == ' ' || cOne == ';' || cOne == ':')
            return 5; // spaces
        break;
    };
    return 0; // no need char
}

bool mySkip_COM(int &nStart, char &cOne){
    while (myWaitCOM(timeOut_)){
        int nRet = myUsedChar((cOne = Serial.read()));
        if (nRet == 0 || nRet == 5)
            nStart++;
        else
            return true;
    }
    return false;
}

// after mySkip, if return is true, pIn[nStart] is 1,2,3,4
bool mySkip(const char *pIn, int &nStart, const int nTotalCount){
  char cOne;
  while (nStart < nTotalCount){
    int nRet = myUsedChar((cOne = pIn[nStart]));
    if (nRet == 0 || nRet == 5)
      nStart++;
    else
      return true;
  }
  return false;
}
int myNextPart(const char *pIn, int &nStart, const int nTotalCount, char *pOut, const int nMax){
    // return pOut the next part, nMax is like pOut buffer max length-1, we need to put 0 in the end
    // part rule: space, ',', '.' ';' ':'
    // data rule: asc will traslate to hex_8bit
    // data rule, '\', \xNum, will be direct hex number, \num will be normal number
    // \DNum  will not translate, send program will use it to delay some time
    // translate rule e.g.: d \1 \xff \D255 -> 'd' 01 ff \D255
    // '\' 'a'-'z' 'A'-'Z' '0'-'9' ',', '.' ';' ':' are used, other char are treat as also break, and no use
    // return length
    // leading no meaning char will be skip
    // return matches 1 part is get, following 1 part or end, middle no meaning char will be skip
    
    // e.g.
    // int nLen = myNextPart(pIn, nStart, nTotalCount, pOut, nMax);
    // if (nLen == 0) return; // end
    // if (nLen < 0) return;  // with error, nothing is in the pOut
    // if (pOut[0] == 0) ; // we meet breaks, there is no char, nLen>0, num of continues breaks
    // Do(pOut)
    // continue with
    // nStart+=nLen; nLen = myNextPart(pIn, nStart, pOut, nMax);
    
    
    int nLen = 0; *pOut = 0;
    // remove leadings
    if (!mySkip(pIn, nStart, nTotalCount))
        return nLen; // meat the end, or all char till end is no used char
    // if return is true, pIn[nStart] is 1, 2, 3, 4, we copy to pOut
    if (nLen < nMax){
        *pOut++ = pIn[nStart]; nLen++;
    }
    else{
        *pOut = 0; return nLen;
    }
    char cOne; int nCur;
    while ((nCur = nStart + nLen) < nTotalCount){
        int nRet = myUsedChar((cOne = pIn[nCur]));
        if (nRet == 0 || nRet == 5){
            // now is end
            break;
        }
        else{
            // we copy to pOut
            if (nLen < nMax){
                *pOut++ = cOne; nLen++;
            }
            else{
                *pOut = 0; return nLen;
            }
        }
    }
    *pOut = 0; return nLen;
}

bool myStr2Double(const char *pIn, int nLen, double &dOut){
    if (pIn==NULL)
        return false;
    if(nLen<0)
        nLen = strlen(pIn);
    double dd = 0;
    for (int i = 0; i < nLen; i++){
        char cOne = pIn[i];
        if (myUsedChar(cOne) != 4) return false;
        double d2 = (cOne - '0');
        dd = dd * 10 + d2;
    }
    dOut = dd;
    return true;
}
bool myStr2DoubleX(const char *pIn, int nLen, double &dOut){
    if (pIn==NULL)
        return false;
    if(nLen<0)
        nLen = strlen(pIn);
    double dd = 0;
    for (int i = 0; i < nLen; i++){
        char cOne = pIn[i];
        int nRet = myUsedChar(cOne);
        if (nRet == 4){
            double d2 = (cOne - '0');
            dd = dd * 16 + d2;
        }
        else if (nRet==1||nRet==2&&myCAP(cOne)<='F'){
            double d2 = (myCAP(cOne) - 'A')+10;
            dd = dd * 16 + d2;
        }
        else return false;
    }
    dOut = dd;
    return true;
}
bool myGET_CMD(MY_CMD &cc){
    int nRead;
    cc.nCount = 0;
    cc.cCMD = 0;
    // skip the beginning no useful char
    while(mySkip_COM(nRead, cc.cCMD)){
        nRead = myWaitCOM(timeOut_);
        if(nRead<1)
            break;
        char sCMD[MY_MAX_COM_CMD_BUF_LEN+1]; // 1 more for end 0 and display
        if(nRead>MY_MAX_COM_CMD_BUF_LEN)
            nRead=MY_MAX_COM_CMD_BUF_LEN;
        for(int i=0;i<nRead;i++)
            sCMD[i] = Serial.read();
        
        int nBufLen;
        int nStart=0;
        char sBuf1[17];
        
        while ((nBufLen = myNextPart(sCMD, nStart, nRead, sBuf1, 16))>0){
            nStart += nBufLen;
            if (sBuf1[0] == 0)
                continue; // we meet breaks, there is no char, nLen>0, num of continues breaks
            if (sBuf1[0] == '\\'){
                if (nBufLen > 1 && myCAP(sBuf1[1]) == 'D'){
                    // copy directly, if next part is not number treat as 1ms
                }
                else if (nBufLen > 1 && myCAP(sBuf1[1]) == 'X'){
                    // get next part as hex, if next part is not number, skip
                    double dd;
                    if (myStr2DoubleX(sBuf1 + 2, nBufLen - 2, dd))
                        if(cc.nCount<MY_MAX_COM_CMD_BUF_LEN)
                            cc.nData[cc.nCount++]=dd;
                    continue;
                }
                else if (nBufLen > 1){
                    // get next part, if next part is not number, skip
                    double dd;
                    if (myStr2Double(sBuf1 + 1, nBufLen - 1, dd))
                        if(cc.nCount<MY_MAX_COM_CMD_BUF_LEN)
                            cc.nData[cc.nCount++]=dd;
                    continue;
                }
                else{ // doing nothing, no left part
                    continue;
                }
            }
            //myAddn(sBuf1, nBufLen);
        };
        return true;
    }
    return false;
}
#define MY_MAX_PEINT_LEN        117

// unsigned long nRead = myWaitCOM(msTimeOut);
// waiting for msTimeOut ms, return buffer length, e.g.
// if(myWaitCOM(msTimeOut)>0){
// }
unsigned long myWaitCOM(unsigned long msTimeOut)
{
    unsigned long ss = millis();
    while (Serial.available() == 0 && (millis() - ss < msTimeOut)) {}
    return Serial.available();
}

