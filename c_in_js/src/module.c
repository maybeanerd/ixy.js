#include <node_api.h>
#include "memory.h"
// we're going to use a whole lot of c code at first, and then work our way down
#include "device.h"

napi_value MyFunction(napi_env env, napi_callback_info info)
{
  napi_status status;
  size_t argc = 1;
  int number = 0;
  napi_value argv[1];
  status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Failed to parse arguments");
  }

  status = napi_get_value_int32(env, argv[0], &number);

  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Invalid number was passed as argument");
  }
  napi_value myNumber;
  number = number + 200;
  status = napi_create_int32(env, number, &myNumber);

  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to create return value");
  }

  return myNumber;
}

// start testing struct stuff
struct Book
{
  char title[50];
  char author[50];
  uint32_t book_id;
};
typedef struct Book Book;

#define member_size(type, member) sizeof(((type *)0)->member)

napi_value changeAuthor(napi_env env, napi_callback_info info)
{
  napi_status status;
  napi_value returnVal;

  size_t argc = 2;
  struct Book book1;
  napi_value argv[2];

  status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Failed to parse arguments");
  }
  char16_t newAuthor[50];
  status = napi_get_value_string_utf8(env, argv[0], newAuthor, sizeof(newAuthor), NULL);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Invalid authorname was passed as first argument");
  }
  size_t bytelength;
  status = napi_get_arraybuffer_info(env,
                                     argv[1],
                                     &book1,
                                     &bytelength);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Invalid book was passed as second argument");
  }

  printf("size of book class: %d\nSize of our arraybuffer: %d\n", sizeof(Book), bytelength);
  printf("The author name we got: %s\n", newAuthor);
  printf("old author name of book1: %s\n", book1.author);
  memcpy(&(book1.author), &(newAuthor), member_size(Book, author));
  printf("new author name of book1: %s\n", book1.author);
  // TODO get output working
  status = napi_create_external_arraybuffer(env,
                                            (void *)&book1,
                                            sizeof(Book),
                                            NULL,
                                            NULL,
                                            &returnVal);
  // napi_create_arraybuffer(env, sizeof( Book), &book1, &returnVal);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to create return value");
  }
  printf("todo print return value here\n");
  return returnVal;
}
/* */

int isLittleEndian()
{
  int i = 1;
  char *p = (char *)&i;

  if (p[0] == 1)
    return 1 /*LITTLE_ENDIAN*/;
  else
    return 0 /*BIG_ENDIAN*/;
}
//function to magically map memory
//including just everything so that nothings missing
#include <assert.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int pci_open_resource(const char *pci_addr, const char *resource)
{
  char path[PATH_MAX];
  snprintf(path, PATH_MAX, "/sys/bus/pci/devices/%s/%s", pci_addr, resource);
  debug("Opening PCI resource at %s", path);
  int fd = check_err(open(path, O_RDWR), "open pci resource");
  return fd;
}
//endof magic memory
napi_value getIDs(napi_env env, napi_callback_info info)
{
  napi_status stat;
  napi_value returnValue;
  size_t argc = 2;
  napi_value argv[2];

  stat = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  if (stat != napi_ok)
  {
    napi_throw_error(env, NULL, "Failed to parse arguments");
  }
  char *pci_addr = malloc(12); // "0000:03:00.0"
  size_t size;
  stat = napi_get_value_string_utf8(env, argv[0], pci_addr, 13, &size); // for some reason we need to use length 13 not 12, to get 12 bytes
  if (stat != napi_ok)
  {
    napi_throw_error(env, NULL, "Invalid string of length 12, the PCI adress, was passed as first argument");
  }
  printf("the pci_address we got: %s ; we copied %d chars\n", pci_addr, size);

  //check if we want to actually give JS the raw adress or already parse (to compare the values we get)
  bool returnRawPointer;
  stat = napi_get_value_bool(env, argv[1], &returnRawPointer);
  if (stat != napi_ok)
  {
    napi_throw_error(env, NULL, "Failed to get the 2nd argument, a boolean");
  }

  //The file handle can be found by typing lscpi -v
  //and looking for your device.
  int config = pci_open_resource(pci_addr, "config");
  // now lets create this as a buffer we give JS
  void *buf = malloc(4);
  stat = napi_create_arraybuffer(env, 4, &buf, &returnValue);
  if (stat != napi_ok)
  {
    napi_throw_error(env, NULL, "Failed our buffer creation");
  }
  // fill empty buffer inside of C
  if (!returnRawPointer)
  {
    pread(config, buf, 4, 0);

    return returnValue;
  }
  else
  {

    FILE *filepointer = fdopen(config, "w+");
    napi_value testReturnVal;
    stat = napi_create_external_arraybuffer(env, filepointer, sizeof(filepointer), NULL, NULL, &testReturnVal);
    if (stat != napi_ok)
    {
      napi_throw_error(env, NULL, "Failed our external buffer creation");
    }
    return testReturnVal;
  }
}
#define IXGBE_EIMC 0x00888

napi_value getReg(napi_env env, napi_callback_info info)
{
  napi_status stat;
  napi_value returnValue;
  size_t argc = 1;
  napi_value argv[1];

  // lets try to make this work in JS:
  /*set_reg32(dev->addr, IXGBE_EIMC, 0x7FFFFFFF);
  defined as:
  static inline void set_reg32(uint8_t *addr, int reg, uint32_t value)
{
	__asm__ volatile(""
					 :
					 :
					 : "memory");
	*((volatile uint32_t *)(addr + reg)) = value;
}
  */
  void **get_reg(uint8_t * addr, int reg)
  {
    __asm__ volatile(""
                     :
                     :
                     : "memory"); // i dont think we need this but lets just keep this here before changing too much
    void **regPointer = *((volatile uint32_t *)(addr + reg));
    return regPointer;
  }

  // endof trying

  stat = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  if (stat != napi_ok)
  {
    napi_throw_error(env, NULL, "Failed to parse arguments");
  }
  char *pci_addr = malloc(12); // "0000:03:00.0"
  size_t size;
  stat = napi_get_value_string_utf8(env, argv[0], pci_addr, 13, &size); // for some reason we need to use length 13 not 12, to get 12 bytes
  if (stat != napi_ok)
  {
    napi_throw_error(env, NULL, "Invalid string of length 12, the PCI adress, was passed as first argument");
  }
  printf("the pci_address we got: %s ; we copied %d chars\n", pci_addr, size);

  //The file handle can be found by typing lscpi -v
  //and looking for your device.
  int fd = pci_open_resource(pci_addr, "resource0");
  // now lets create this as a buffer we give JS
  void *buf = malloc(4);
  stat = napi_create_arraybuffer(env, 4, &buf, &returnValue);
  if (stat != napi_ok)
  {
    napi_throw_error(env, NULL, "Failed our buffer creation");
  }
  //this is what we need to get the root adress
  struct stat stat2;
  check_err(fstat(fd, &stat2), "stat pci resource");
  printf("Size of the stat: %d\n", stat2.st_size);
  uint8_t *pci_map_resource_js = check_err(mmap(NULL, stat2.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0), "mmap pci resource"); // we get the error Invalid Argument here

  void **filepointer = get_reg(pci_map_resource_js, IXGBE_EIMC);
  napi_value testReturnVal;
  stat = napi_create_external_arraybuffer(env, filepointer, sizeof(filepointer), NULL, NULL, &testReturnVal);
  if (stat != napi_ok)
  {
    napi_throw_error(env, NULL, "Failed our external buffer creation");
  }
  return testReturnVal;
}
// Try writing a string into the buf (WORKS finally!)
napi_value writeString(napi_env env, napi_callback_info info)
{

  printf("c says is this little endian?: %d\n", isLittleEndian());
  napi_status status;
  napi_value returnVal;

  size_t argc = 2;
  napi_value argv[2];

  status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Failed to parse arguments");
  }
  char inputString[8];
  status = napi_get_value_string_utf8(env, argv[0], inputString, sizeof(inputString), NULL);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Invalid string of length 10 was passed as first argument");
  }
  bool result;
  status = napi_is_arraybuffer(env, argv[1], &result);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Failed checking if input is arraybuffer");
  }
  printf("The inputted data is an arraybuffer?: %d\n", result);
  //char string[10];
  int16_t *inputArrayBuffer[4];
  size_t lengthOfString;
  status = napi_get_arraybuffer_info(env,
                                     argv[1],
                                     (void **)inputArrayBuffer,
                                     &lengthOfString);
  // napi_get_value_string_utf8(env, argv[1], string, 10, NULL);

  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Invalid arraybuffer was passed as second argument");
  }
  printf("The inputted arraybuffer as string: %s\n", argv[1]);
  printf("The inputted arraybuffers data as string: %s\n", inputArrayBuffer);

  printf("length of arraybuffer: %d\n", lengthOfString);
  // printf("Input string: %s\n", inputString);
  // printf("Input arraybuffer: %d\n", argv[1]);
  printf("size of a single element in arraybuffer: %d\n", sizeof(inputArrayBuffer[0][0]));
  for (int i = 0; i < lengthOfString / sizeof(inputArrayBuffer[0][0]); i++)
  {
    printf("Input arraybuffers data at index %d : %d\n", i, inputArrayBuffer[0][i]);
    inputArrayBuffer[0][i] = 30;
    printf("Change arraybuffers data at index %d : %d\n", i, inputArrayBuffer[0][i]);
  }

  return argv[1]; // this should be the original array buffer, and we changed the data lying beneath? - yes
}

//end testing struct stuff

napi_value arrayTest(napi_env env, napi_callback_info info) // we create a uint32 array based on an input, to be sure we deliver data correctly
{
  napi_status status;
  size_t argc = 1;
  int number = 0;
  napi_value argv[1];
  status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Failed to parse arguments");
  }

  status = napi_get_value_int32(env, argv[0], &number);

  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Invalid number was passed as argument");
  }
  napi_value ret;
  printf("the number we got: %d\n", number);

  uint32_t uints[] = {number, number * 2, number * 3, number * 4};
  // TODO find out how we would actually convert this, seems very inconvenient from what i found in napi docs. Maybe we want to handle only single values later ? would be impractical in some cases though
  // status = napi_create_uint32(env, uints, &ret);
  printf("our array: \n");
  for (int i = 0; i < sizeof(uints) / sizeof(uints[0]); i++)
  {
    printf("%d\n", uints[i]);
  }
  // trying to create an array buffer from this input
  napi_create_external_arraybuffer(env,
                                   &uints,
                                   sizeof(uints),
                                   NULL,
                                   NULL,
                                   &ret);

  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to create return value");
  }

  return ret;
}

napi_value MempoolTest(napi_env env, napi_callback_info info)
{
  napi_status status;
  napi_value returnVal;

  size_t argc = 2;
  int num_entries = 1; // default to 1 i guess
  int entry_size = 0;  // will default inside memory.c if it stays 0
  napi_value argv[2];

  status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Failed to parse arguments");
  }

  status = napi_get_value_int32(env, argv[0], &num_entries);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Invalid number was passed as first argument");
  }

  status = napi_get_value_int32(env, argv[1], &entry_size);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Invalid number was passed as second argument");
  }

  //printf("Inside module.c:\nNum entries: "+num_entries+"\n"+"Entry size: "+ entry_size);

  struct mempool *myMempool = memory_allocate_mempool(num_entries, entry_size);

  status = napi_create_arraybuffer(env, sizeof(struct mempool), (void **)&myMempool, &returnVal);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to create return value");
  }

  return returnVal;
}
/* temporarily disable this
napi_value createIxyDevice(napi_env env, napi_callback_info info)
{
  //TODO use the c code here correctly
  napi_status status;
  napi_value returnVal;

  size_t argc = 3;
  char *pci_addr;
  uint16_t rx_queues = 1; // default to 1
  uint16_t tx_queues = 1; // default to 1
  napi_value argv[3];

  status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Failed to parse arguments");
  }

  status = napi_get_value_string_utf8(env, argv[0], pci_addr); //not sure if & is needed here or not /shrugs
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Invalid char was passed as first argument");
  }
  printf("this should be the pci address: %s", pci_addr);
  status = napi_get_value_uint16(env, argv[1], &rx_queues);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Invalid number was passed as second argument");
  }
  printf("this should be the rx queues count: %d", rx_queues);

  status = napi_get_value_uint16(env, argv[2], &tx_queues);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Invalid number was passed as second argument");
  }
  printf("this should be the tx queues count: %d", tx_queues);

  // creating the struct
  struct ixy_device *dev = ixy_init(pci_addr, rx_queues, tx_queues);

  status = napi_create_arraybuffer(env, sizeof(struct ixy_device), (void **)&dev, &returnVal);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to create return value");
  }

  return returnVal;
}
*/
// potential TODO rewrite these to Node API
/*
static inline uint32_t ixy_rx_batch(struct ixy_device *dev, uint16_t queue_id, struct pkt_buf *bufs[], uint32_t num_bufs)
{
  return dev->rx_batch(dev, queue_id, bufs, num_bufs);
}

static inline uint32_t ixy_tx_batch(struct ixy_device *dev, uint16_t queue_id, struct pkt_buf *bufs[], uint32_t num_bufs)
{
  return dev->tx_batch(dev, queue_id, bufs, num_bufs);
}

static inline void ixy_read_stats(struct ixy_device *dev, struct device_stats *stats)
{
  dev->read_stats(dev, stats);
}

static inline void ixy_set_promisc(struct ixy_device *dev, bool enabled)
{
  dev->set_promisc(dev, enabled);
}

static inline uint32_t get_link_speed(const struct ixy_device *dev)
{
  return dev->get_link_speed(dev);
}

// calls ixy_tx_batch until all packets are queued with busy waiting
static void ixy_tx_batch_busy_wait(struct ixy_device *dev, uint16_t queue_id, struct pkt_buf *bufs[], uint32_t num_bufs)
{
  uint32_t num_sent = 0;
  while ((num_sent += ixy_tx_batch(dev, 0, bufs + num_sent, num_bufs - num_sent)) != num_bufs)
  {
    // busy wait
  }
}

// getters/setters for PCIe memory mapped registers
static inline void set_reg32(uint8_t *addr, int reg, uint32_t value)
{
  __asm__ volatile(""
                   :
                   :
                   : "memory");
  *((volatile uint32_t *)(addr + reg)) = value;
}

static inline uint32_t get_reg32(const uint8_t *addr, int reg)
{
  __asm__ volatile(""
                   :
                   :
                   : "memory");
  return *((volatile uint32_t *)(addr + reg));
}

static inline void set_flags32(uint8_t *addr, int reg, uint32_t flags)
{
  set_reg32(addr, reg, get_reg32(addr, reg) | flags);
}

static inline void clear_flags32(uint8_t *addr, int reg, uint32_t flags)
{
  set_reg32(addr, reg, get_reg32(addr, reg) & ~flags);
}

static inline void wait_clear_reg32(const uint8_t *addr, int reg, uint32_t mask)
{
  __asm__ volatile(""
                   :
                   :
                   : "memory");
  uint32_t cur = 0;
  while (cur = *((volatile uint32_t *)(addr + reg)), (cur & mask) != 0)
  {
    debug("waiting for flags 0x%08X in register 0x%05X to clear, current value 0x%08X", mask, reg, cur);
    usleep(10000);
    __asm__ volatile(""
                     :
                     :
                     : "memory");
  }
}

static inline void wait_set_reg32(const uint8_t *addr, int reg, uint32_t mask)
{
  __asm__ volatile(""
                   :
                   :
                   : "memory");
  uint32_t cur = 0;
  while (cur = *((volatile uint32_t *)(addr + reg)), (cur & mask) != mask)
  {
    debug("waiting for flags 0x%08X in register 0x%05X, current value 0x%08X", mask, reg, cur);
    usleep(10000);
    __asm__ volatile(""
                     :
                     :
                     : "memory");
  }
}

// getters/setters for pci io port resources

static inline void write_io32(int fd, uint32_t value, size_t offset)
{
  if (pwrite(fd, &value, sizeof(value), offset) != sizeof(value))
    error("pwrite io resource");
  __asm__ volatile(""
                   :
                   :
                   : "memory");
}

static inline void write_io16(int fd, uint16_t value, size_t offset)
{
  if (pwrite(fd, &value, sizeof(value), offset) != sizeof(value))
    error("pwrite io resource");
  __asm__ volatile(""
                   :
                   :
                   : "memory");
}

static inline void write_io8(int fd, uint8_t value, size_t offset)
{
  if (pwrite(fd, &value, sizeof(value), offset) != sizeof(value))
    error("pwrite io resource");
  __asm__ volatile(""
                   :
                   :
                   : "memory");
}

static inline uint32_t read_io32(int fd, size_t offset)
{
  __asm__ volatile(""
                   :
                   :
                   : "memory");
  uint32_t temp;
  if (pread(fd, &temp, sizeof(temp), offset) != sizeof(temp))
    error("pread io resource");
  return temp;
}

static inline uint16_t read_io16(int fd, size_t offset)
{
  __asm__ volatile(""
                   :
                   :
                   : "memory");
  uint16_t temp;
  if (pread(fd, &temp, sizeof(temp), offset) != sizeof(temp))
    error("pread io resource");
  return temp;
}

static inline uint8_t read_io8(int fd, size_t offset)
{
  __asm__ volatile(""
                   :
                   :
                   : "memory");
  uint8_t temp;
  if (pread(fd, &temp, sizeof(temp), offset) != sizeof(temp))
    error("pread io resource");
  return temp;
}
*/
//ENDOF node API rewrites

napi_value Init(napi_env env, napi_value exports)
{
  napi_status status;
  napi_value fn;

  status = napi_create_function(env, NULL, 0, MyFunction, NULL, &fn);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to wrap native function");
  }

  status = napi_set_named_property(env, exports, "my_function", fn);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to populate exports");
  }

  //adding my test buffer thingy
  status = napi_create_function(env, NULL, 0, MempoolTest, NULL, &fn);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to wrap native function");
  }

  status = napi_set_named_property(env, exports, "mempoolTest", fn);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to populate exports");
  }

  //adding my test struct thingy
  status = napi_create_function(env, NULL, 0, changeAuthor, NULL, &fn);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to wrap native function");
  }

  status = napi_set_named_property(env, exports, "changeAuthor", fn);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to populate exports");
  }
  //adding my test string thingy
  status = napi_create_function(env, NULL, 0, writeString, NULL, &fn);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to wrap native function");
  }

  status = napi_set_named_property(env, exports, "writeString", fn);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to populate exports");
  }
  // adding my getIDs to get PCI id stuff
  status = napi_create_function(env, NULL, 0, getIDs, NULL, &fn);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to wrap native function");
  }

  status = napi_set_named_property(env, exports, "getIDs", fn);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to populate exports");
  }

  //adding my second test buffer thingy
  status = napi_create_function(env, NULL, 0, arrayTest, NULL, &fn);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to wrap native function");
  }

  status = napi_set_named_property(env, exports, "arrayTest", fn);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to populate exports");
  }
  //add getReg to exports
  status = napi_create_function(env, NULL, 0, getReg, NULL, &fn);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to wrap native function");
  }

  status = napi_set_named_property(env, exports, "getReg", fn);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to populate exports");
  }
  /* temproarily deactivate
  // adding ixy device creator
  status = napi_create_function(env, NULL, 0, createIxyDevice, NULL, &fn);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to wrap native function");
  }

  status = napi_set_named_property(env, exports, "createIxyDevice", fn);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to populate exports");
  }
*/
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)