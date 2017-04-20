
#define VTXdataPIN 0
#define VTXssPIN 1
#define VTXclkPIN 2

void InitVTX(void);
void channelUp(void);
void channelDown(void);
void VTXsetChannel(uint8_t chan);
void VTXsendTransmission(uint8_t adr, uint32_t channel);
void SPItransferWord(uint32_t WordOut);
void checkChannel(void);
