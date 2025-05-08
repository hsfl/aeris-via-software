#ifndef __AvaSpec_h_
#define __AvaSpec_h_
#include <Arduino.h>
#include <USBHost_t36.h>
#include <SD.h>
#include <Logger.h>

class AvaSpec : public USBDriver {
public:
    AvaSpec(USBHost &host) { init(); }

    void getIdentification();
    void getPipeBuffer();
    void printBuffer(uint8_t* buf, size_t n);

    /**
     * calls command start_measurement. may need to be called after prepareMeasurement()
     * Command ID: 0x06
     * Expected Response ID: 0x86
     */
    void startMeasurement();
    void handleUnsolicitatedData();
    void prepareMeasurement();
    void stopMeasurement();
    void measurementAcknowledgement(); 

protected:
    virtual bool claim(Device_t *device, int type, const uint8_t *descriptors, uint32_t len);
    // virtual void control(const Transfer_t *transfer);
    void init();
    virtual void disconnect() {};
    void process_rx_data(const Transfer_t *transfer);
    void process_tx_data(const Transfer_t *transfer);
    // void dump_hexbytes(const void *ptr, uint32_t len);
private:
    Pipe_t *rxpipe;
    Pipe_t *txpipe;
    uint16_t rx_size;
    uint16_t tx_size;
    uint8_t rx_ep;
    uint8_t tx_ep;
	Pipe_t mypipes[3] __attribute__ ((aligned(32)));
	Transfer_t mytransfers[7] __attribute__ ((aligned(32)));
    static void tx_callback(const Transfer_t *transfer);
    static void rx_callback(const Transfer_t *transfer);
    static const int BUF_SIZE = 512;
    static const int MEAS_SIZE = 4106;  
    bool rx_data_ready;
    uint8_t rx_buffer[BUF_SIZE];
    uint8_t tx_buffer[BUF_SIZE];
    bool messageFound;
    uint8_t measAmount; 
    uint8_t measurement[MEAS_SIZE]; 
    int appendIndex; 
};
#endif
