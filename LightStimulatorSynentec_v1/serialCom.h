#define MY_MAX_COM_CMD_BUF_LEN  99
#define MY_MAX_COM_CMD_INT_LEN  6
//---------------------------------------------------------------------------

//Toller Kommentar !!!

typedef struct _MY_CMD{
    char cCMD=NULL;
    int  nCount;
    int  nData[MY_MAX_COM_CMD_INT_LEN + 1];
}  MY_CMD, *pMY_CMD;

bool myGET_CMD(MY_CMD &cc);

