const defines = require('./constants');
const packets = require('./packets');
const { RxDescriptor } = require('./descriptors');
const {
  set_reg_js, set_flags_js, wait_set_reg_js,
} = require('./regOps');
const mempool = require('./mempool');

function start_rx_queue(ixgbe_device, queue_id) {
  console.info(`starting rx queue ${queue_id}`);
  const queue = ixgbe_device.rx_queues[queue_id];
  // 2048 as pktbuf size is strictly speaking incorrect:
  // we need a few headers (1 cacheline), so there's only 1984 bytes left for the device
  // but the 82599 can only handle sizes in increments of 1 kb;
  // but this is fine since our max packet size
  // is the default MTU of 1518
  // this has to be fixed if jumbo frames are to be supported
  // mempool should be >= the number of rx and tx descriptors for a forwarding application
  const mempool_size = defines.NUM_RX_QUEUE_ENTRIES + defines.NUM_TX_QUEUE_ENTRIES;
  queue.mempool = mempool.alloc(mempool_size < 4096 ? 4096 : mempool_size, 2048);
  if (queue.num_entries % 2 !== 0) {
    throw new Error('number of queue entries must be a power of 2');
  }
  for (let i = 0; i < queue.num_entries; i++) {
    const rxd = new RxDescriptor(queue.descriptors, i);
    const buf = packets.alloc(queue.mempool);
    if (!buf) {
      throw new Error('failed to allocate rx descriptor');
    }
    // set pkt addr
    rxd.memView.d64[0 + rxd.offset / 8] = buf.buf_addr_phy;
    // set hdr addr
    rxd.memView.d64[1 + rxd.offset / 8] = BigInt(0)/* 0n */; // TODO rewrite to 1n syntax before running, but keep at BigInt(1) syntax because otherwise eslint will not work

    // we need to return the virtual address in the rx function
    // which the descriptor doesn't know by default
    queue.virtual_addresses[i] = buf;
  }
  // enable queue and wait if necessary
  set_flags_js(ixgbe_device, defines.IXGBE_RXDCTL(queue_id), defines.IXGBE_RXDCTL_ENABLE);
  wait_set_reg_js(ixgbe_device, defines.IXGBE_RXDCTL(queue_id), defines.IXGBE_RXDCTL_ENABLE);
  // rx queue starts out full
  set_reg_js(ixgbe_device, defines.IXGBE_RDH(queue_id), 0);
  // was set to 0 before in the init function
  set_reg_js(ixgbe_device, defines.IXGBE_RDT(queue_id), queue.num_entries - 1);
}

function start_tx_queue(dev, queue_id) {
  console.info(`starting tx queue ${queue_id}`);
  const queue = dev.tx_queues[queue_id];
  if (queue.num_entries & (queue.num_entries - 1)) {
    throw new Error('number of queue entries must be a power of 2');
  }
  // tx queue starts out empty
  set_reg_js(dev, defines.IXGBE_TDH(queue_id), 0);
  set_reg_js(dev, defines.IXGBE_TDT(queue_id), 0);
  // enable queue and wait if necessary
  set_flags_js(dev, defines.IXGBE_TXDCTL(queue_id), defines.IXGBE_TXDCTL_ENABLE);
  wait_set_reg_js(dev, defines.IXGBE_TXDCTL(queue_id), defines.IXGBE_TXDCTL_ENABLE);
}

module.exports = {
  startRX: start_rx_queue, startTX: start_tx_queue,
};
