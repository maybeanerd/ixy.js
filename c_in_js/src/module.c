#include <node_api.h>
#include "memory.h"

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

  struct mempool *myMempool = memory_allocate_mempool(num_entries, entry_size);

  status = napi_create_arraybuffer(env, myMempool->buf_size, (void **)&myMempool, &returnVal);
  if (status != napi_ok)
  {
    napi_throw_error(env, NULL, "Unable to create return value");
  }

  return returnVal;
}

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

  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)