const addon = require('./build/Release/exported_module');
const jstruct = require("js-struct");
// testing the random function that uses c 
const value = 99558;
console.log(`Input :${value}\nOutput:${addon.my_function(value)}`); // expected to add 200
let numEntries = 2, entrySize = 2048;
let myArrayBuffer = addon.mempoolTest(numEntries, entrySize);
let myTypedArray = new Uint8Array(myArrayBuffer);
// let's see how big this buffer is, shall we?
console.log(`My Array buffer parameters:
    Num Entries: ${numEntries}
    Entry Size: ${entrySize}
Returned buffer byte length is: ${myArrayBuffer.byteLength}
Created typed array byte length is: ${myTypedArray.byteLength}`);
//with more entries?
numEntries = 10;
entrySize = 1024;
myArrayBuffer = addon.mempoolTest(numEntries, entrySize);
myTypedArray = new Uint8Array(myArrayBuffer);
// let's see how big this buffer is, shall we?
console.log(`My Array buffer parameters:
    Num Entries: ${numEntries}
    Entry Size: ${entrySize}
Returned buffer byte length is: ${myArrayBuffer.byteLength}
Created typed array byte length is: ${myTypedArray.byteLength}`);

//trying to use js-struct to use the c struct correctly 
/* c struct:
struct mempool {
	void* base_addr;
	uint32_t buf_size;
	uint32_t num_entries;
	uint32_t free_stack_top;
	// the stack contains the entry id, i.e., base_addr + entry_id * buf_size is the address of the buf
	uint32_t free_stack[];
};
*/
const jsMempool = jstruct.Struct([
    jstruct.Type.byte('base_addr'),
    jstruct.Type.uint32('buf_size'),
    jstruct.Type.uint32('num_entries'),
    jstruct.Type.uint32('free_stack_top'),
    jstruct.Type.array(jstruct.Type.uint32, numEntries)('free_stack'),
]);
console.log("Now this should be the struct:");
console.log(jsMempool.read(myTypedArray, 0));
console.log(`length of my typed array: ${myTypedArray.length}`);


console.log('starting array test with input 5');
const myArrayTestBuffer = addon.arrayTest(5);
let myTypedArrayTest = new Uint32Array(myArrayTestBuffer);
console.log('this is the array we got: ');
console.log(myTypedArrayTest);

// testing if struct actually works the way i think it does

// todo

// endof testing struct stuff
/* temporarily deactivate this
// trying ixy_Device stuff
const klaipedaPci = "0000:02:00.0", narvaPci = "0000:03:00.0";
let myIxyDevice = addon.createIxyDevice(klaipedaPci, 1, 1);
myTypedArray = new Uint16Array(myIxyDevice);
*/
// original struct:
/*
struct ixy_device
{
	const char *pci_addr;
	const char *driver_name;
	uint16_t num_rx_queues;
	uint16_t num_tx_queues;
	uint32_t (*rx_batch)(struct ixy_device *dev, uint16_t queue_id, struct pkt_buf *bufs[], uint32_t num_bufs);
	uint32_t (*tx_batch)(struct ixy_device *dev, uint16_t queue_id, struct pkt_buf *bufs[], uint32_t num_bufs);
	void (*read_stats)(struct ixy_device *dev, struct device_stats *stats);
	void (*set_promisc)(struct ixy_device *dev, bool enabled);
	uint32_t (*get_link_speed)(const struct ixy_device *dev);
};
*/
/*
const jsIxyDevice = jstruct.Struct(
    //TODO
);*/
/* since struct is not yet defined we do not try using it yet:
console.log("Now this should be the ixy device struct:");
console.log(klaipedaPci.read(myTypedArray, 0));
*/
