#include <Arduino.h>
#include "USBHost_t36.h"
#include "AvaSpec.h"

#define AV_VID 0x1992
#define AV_PID 0x0668

/*
Beginning functions to initialize the AvaSpec Driver
init()
claim()
*/
void AvaSpec::init()
{
	contribute_Pipes(mypipes, sizeof(mypipes) / sizeof(Pipe_t));
	contribute_Transfers(mytransfers, sizeof(mytransfers) / sizeof(Transfer_t));
	driver_ready_for_device(this);
	rx_data_ready = false; 
	messageFound = false; 
}

bool AvaSpec::claim(Device_t *dev, int type, const uint8_t *descriptors, uint32_t len)
{
	Serial.println("\n Attempting to claim AvaSpec device");
	Serial.print("\n AvaSpec driver attempting to claim this = ");
	Serial.println((uint32_t)this, HEX);

	if (type == 0)
	{
		Serial.println("\nDevice Descriptors:");
		Serial.print("vid = ");
		Serial.println(dev->idVendor, HEX);
		Serial.print("pid = ");
		Serial.println(dev->idProduct, HEX);
		// println("bcdDevice=", dev->bcdDevice, HEX);
		Serial.print("bDeviceClass = ");
		Serial.println(dev->bDeviceClass, HEX);
		Serial.print("bDeviceSubClass = ");
		Serial.println(dev->bDeviceSubClass, HEX);
		// dump_hexbytes(descriptors, len);
	}

	// Descriptor length of Ava Spec is 23 bytes.
	if (len != 23)
		return false;
	// Check if the device is Ava Spec Mini specifically.
	if (dev->idVendor != AV_VID || dev->idProduct != AV_PID)
		return false;
	Serial.println("AvaSpec device found and claimed");

	Serial.print("\nInterface # = ");
	Serial.println(descriptors[2], HEX);
	Serial.print("Interface Class = ");
	Serial.println(descriptors[5], HEX);
	Serial.print("nEndpoints = ");
	Serial.println(descriptors[4], HEX);
	Serial.print("IN Endpoint Address = ");
	Serial.println(descriptors[11], HEX);
	tx_ep = descriptors[11];
	Serial.print("Endpoint Attribute = ");
	Serial.println(descriptors[12], HEX);
	Serial.print("OUT Endpoint Address = ");
	Serial.println(descriptors[18], HEX);
	rx_ep = descriptors[18];
	Serial.print("Endpoint Attribute = ");
	Serial.println(descriptors[19], HEX);

	rx_size = BUF_SIZE;
	tx_size = BUF_SIZE;

	rxpipe = new_Pipe(dev, 2, 6, 1, rx_size);
	rxpipe->callback_function = rx_callback;
	txpipe = new_Pipe(dev, 2, 2, 0, tx_size);
	txpipe->callback_function = tx_callback;

	return true;
}


/**
 * Prints the out n bytes of the given buffer
 * will print a newline before printing bytes
 */
void AvaSpec::printBuffer(uint8_t* buf, size_t n)
{
	for (size_t i = 0; i < n; i++)
	{
		if (i % 32 == 0)
		{ // Print a newline every 16 bytes for readability. will print a newline immediately
			Serial.println();
		}
		if (buf[i] < 0x10)
		{ // Add leading zero for single hex digits
			Serial.print("0");
		}
		Serial.print(buf[i], HEX);
		Serial.print(" ");
	}
	Serial.println();
}

void AvaSpec::getPipeBuffer()
{
	uint8_t response[BUF_SIZE];
	memset(response, 0, BUF_SIZE);
	queue_Data_Transfer(rxpipe, response, rx_size, this);

	// print out the response we got
	Serial.println("\nreading response...");

	bool dataFound = false;
	int initialTime = millis();

	while((millis() - initialTime) < 10000) {
		for (int i = 0; i < BUF_SIZE; i++) {
			if (response[i] == 0x21) {
				Serial.println("Data marker found! Printing buffer...");
				printBuffer(response, 512);
				dataFound = true; 
				break; 
			}
		}
		delay(1);
		if (dataFound) break; 
	}
	Serial.print("no response...");
}


// --------------------------------------------------------------------------
//	Functions called when queuing a data transfer or when a transfer is done.  
// --------------------------------------------------------------------------
void AvaSpec::rx_callback(const Transfer_t *transfer){
	if (transfer->driver) {
		((AvaSpec *)(transfer->driver))->process_rx_data(transfer);
	}
}

void AvaSpec::process_rx_data(const Transfer_t *transfer) {
	memcpy(rx_buffer, transfer->buffer, transfer->length);
	rx_data_ready = true; 

	if (rx_buffer[0] == 0x21 || rx_buffer[4] == 0xB1) {
		Serial.println("\nMeasurement Data Received:");
		printBuffer(rx_buffer, transfer->length);

		// Sending Acknowledgement
		uint8_t ack[6] = {0x21, 0x00, 0x02, 0x00, 0xC0, 0x00};
		queue_Data_Transfer(txpipe, ack, sizeof(ack), this); 
		messageFound = true; 
	}
	if (messageFound) {
		printBuffer(rx_buffer, transfer->length);
	}
}

void AvaSpec::tx_callback(const Transfer_t *transfer) {
	if (transfer->driver) {
		((AvaSpec *)(transfer->driver))->process_tx_data(transfer);
	}
}

void AvaSpec::process_tx_data(const Transfer_t *transfer) {
	// Tx Data has been sent.
}

void AvaSpec::handleUnsolicitatedData() {
	if (!rx_data_ready) {
		queue_Data_Transfer(rxpipe, rx_buffer, rx_size, this);
		return;
	}

	// Serial.println("\nProcessing received data...");
	// printBuffer(rx_buffer, 512);

	rx_data_ready = false; 
}

// --------------------------------------------------------------------------

void AvaSpec::getIdentification()
{

	uint32_t len = 0;
	// set commmand header
	tx_buffer[len++] = 0x20;
	tx_buffer[len++] = 0x00;
	tx_buffer[len++] = 0x02;
	tx_buffer[len++] = 0x00;
	tx_buffer[len++] = 0x13;
	tx_buffer[len++] = 0x00;

	// print out our to-be-sent command up to n bytes
	Serial.println("\nsending command: get_ident");
	printBuffer(tx_buffer, 6);

	__disable_irq();
	queue_Data_Transfer(txpipe, tx_buffer, tx_size, this);
	__enable_irq();

	rx_data_ready = false; 
	queue_Data_Transfer(rxpipe, rx_buffer, rx_size, this);

	int start_time = millis();
	while(!rx_data_ready && (millis() - start_time < 3000)) {
		delay(1);
	}

	if (rx_data_ready) {
		Serial.println("\nfetching response: get_ident_response");
		printBuffer(rx_buffer, 92);
	} else {
		Serial.println("\nNo response received.");
	}

	memset(tx_buffer, 0, BUF_SIZE);
	memset(rx_buffer, 0, BUF_SIZE);
}


/**
 * calls command start_measurement. may need to be called after prepareMeasurement()
 * Command ID: 0x06
 * Expected Response ID: 0x86
 */
void AvaSpec::startMeasurement()
{
	// command and its parameters
	// uint8_t command_id = 0x06;
	// signed short nmsr = 1;	// stands for number of measurements. Defined as an unsigned short on the manual

	uint32_t len = 0;
	// set commmand header
	tx_buffer[len++] = 0x20;
	tx_buffer[len++] = 0x00;
	tx_buffer[len++] = 0x04;
	tx_buffer[len++] = 0x00;
	tx_buffer[len++] = 0x06;
	tx_buffer[len++] = 0x00;
	tx_buffer[len++] = 0x00;
	tx_buffer[len++] = 0x04;


	// print out our to-be-sent command up to n bytes
	Serial.println("\nsending command: start_measurement");
	printBuffer(tx_buffer, 8);

	__disable_irq();
	queue_Data_Transfer(txpipe, tx_buffer, tx_size, this);
	__enable_irq();

	rx_data_ready = false; 
	queue_Data_Transfer(rxpipe, rx_buffer, rx_size, this);

	int start_time = millis();
	while (!rx_data_ready && (millis() - start_time < 1000)) {
		delay(1);
	}

	if (rx_data_ready) {
		Serial.println("\nfetching response: start_measurement_response");
		printBuffer(rx_buffer, 6);
	} else {
		Serial.println("\nNo response received.");
	}
	rx_data_ready = false; 

	memset(tx_buffer, 0, BUF_SIZE);
	memset(rx_buffer, 0, BUF_SIZE);
	queue_Data_Transfer(rxpipe, rx_buffer, rx_size, this);
	Serial.println("Queuing Data...");
}

void AvaSpec::prepareMeasurement()
{
    memset(tx_buffer, 0, BUF_SIZE);  // Clear previous command data
    tx_buffer[0] = 0x20;  // Type
    tx_buffer[1] = 0x00;  // Sequence number
    tx_buffer[2] = 0x2B;  // Length LSB (43 bytes total)
    tx_buffer[3] = 0x00;  // Length MSB
    tx_buffer[4] = 0x05;  // Command ID for prepare_measurement
    tx_buffer[5] = 0x00;  // Sequence

    // 🔹 Store values in Big Endian
    tx_buffer[6] = 0x00;  // Start Pixel MSB
	tx_buffer[7] = 0x00;  // Start Pixel LSB
	tx_buffer[8] = 0x07;  // Stop Pixel MSB
	tx_buffer[9] = 0xFF;  // Stop Pixel LSB

    tx_buffer[10] = 0x42;  // MSB
	tx_buffer[11] = 0xC8;
	tx_buffer[12] = 0x00;
	tx_buffer[13] = 0x00;  // LSB

    // Integration Delay (4 bytes) - Default to 0 (No Delay)
	tx_buffer[14] = 0x00;  // MSB
	tx_buffer[15] = 0x00;
	tx_buffer[16] = 0x00;
	tx_buffer[17] = 0x64;  // LSB

	// Trigger Mode (1 byte) - Software Trigger
	// Number of Averages (1) - Big Endian Unsigned Int
	tx_buffer[18] = 0x00;  // MSB
	tx_buffer[19] = 0x00;
	tx_buffer[20] = 0x00;
	tx_buffer[21] = 0x01;  // LSB

	memset(&tx_buffer[22], 0, 21);

    Serial.println("\nSending command: prepare_measurement");
    printBuffer(tx_buffer, 47);

    __disable_irq();
    queue_Data_Transfer(txpipe, tx_buffer, tx_size, this);
    __enable_irq();

	rx_data_ready = false; 
	queue_Data_Transfer(rxpipe, rx_buffer, rx_size, this);

	int start_time = millis();
	while(!rx_data_ready && (millis() - start_time < 3000)) {
		delay(1);
	}

	if (rx_data_ready) {
		Serial.println("\nfetching response: prep_measurements");
		printBuffer(rx_buffer, 8);
	} else {
		Serial.println("\nNo response received.");
	}

	memset(tx_buffer, 0, BUF_SIZE);
	memset(rx_buffer, 0, BUF_SIZE);
}

void AvaSpec::stopMeasurement()
{
    memset(tx_buffer, 0, BUF_SIZE);
    tx_buffer[0] = 0x20;
    tx_buffer[1] = 0x00;
    tx_buffer[2] = 0x02;
    tx_buffer[3] = 0x00;
    tx_buffer[4] = 0x0F;  // **Command ID for stop_measurement**
    tx_buffer[5] = 0x00;

    Serial.println("\n🔹 Sending command: stop_measurement");
    printBuffer(tx_buffer, 6);

    __disable_irq();
    queue_Data_Transfer(txpipe, tx_buffer, tx_size, this);
    __enable_irq();

	rx_data_ready = false; 
	queue_Data_Transfer(rxpipe, rx_buffer, rx_size, this);

	int start_time = millis();
	while(!rx_data_ready && (millis() - start_time < 3000)) {
		delay(1);
	}

	if (rx_data_ready) {
		Serial.println("\nfetching response: stop_measurement");
		printBuffer(rx_buffer, 12);
	} else {
		Serial.println("\nNo response received.");
	}

	memset(tx_buffer, 0, BUF_SIZE);
	memset(rx_buffer, 0, BUF_SIZE);
}






