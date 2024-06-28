/*
  Copyright (C) 2024 Andrew Dunstan
  This file is part of teensy4_usbhost (https://github.com/A-Dunstan/teensy4_usbhost).

  teensy4_usbhost is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "RNDIS_QNEthernet.h"

#ifdef QNETHERNET_EXTERNAL_DRIVER_RNDIS

#include <lwip_driver.h>

#pragma message("Using RNDIS QNEthernet Driver")

USB_RNDIS& rndis(void) {
  static DMAMEM USB_RNDIS rndis_device;
  return rndis_device;
}

extern "C" {

void driver_get_system_mac(uint8_t mac[ETH_HWADDR_LEN]) {
  rndis().get_mac_address(mac);
}

void driver_set_mac(const uint8_t mac[ETH_HWADDR_LEN]) {
  puts("driver_set_mac");
}

bool driver_has_hardware() {
  puts("driver_has_hardware");
  return rndis().isUp();
}

void driver_set_chip_select_pin(int pin) {}

bool driver_init(const uint8_t mac[ETH_HWADDR_LEN]) {
  puts("driver_init");
  return true;
}

void driver_deinit() {
  puts("driver_deinit");
}

void driver_proc_input(struct netif *netif) {
  if (netif_is_link_up(netif) == 0) return;

  size_t data_len;
  while ((data_len = rndis().recv(NULL)) > 0) {
    struct pbuf *p = pbuf_alloc(PBUF_RAW, data_len+ETH_PAD_SIZE, PBUF_RAM);
    if (p == NULL) {
      puts("Failed to allocate pbuf");
      break;
    }
    p->len = p->tot_len = rndis().recv((uint8_t*)p->payload+ETH_PAD_SIZE)+ETH_PAD_SIZE;
    if (netif->input(p, netif) != ERR_OK)
      pbuf_free(p);
  }
}

void driver_poll(struct netif *netif) {
  uint8_t link_up = rndis().isUp() ? 1 : 0;
  if (netif_is_link_up(netif) != link_up) {
    if (link_up) {
      puts("Setting link up");
      netif_set_link_up(netif);
    } else {
      puts("Setting link down");
      netif_set_link_down(netif);
    }
  }
}

err_t driver_output(struct pbuf *p) {
  size_t header_length = rndis().packet_header_length();

  uint8_t* packet = (uint8_t*)aligned_alloc(32, header_length + p->tot_len - ETH_PAD_SIZE);
  if (packet == NULL) {
    puts("Failed to allocate packet for sending");
    return ERR_MEM;
  }

  pbuf_copy_partial(p, packet+header_length, p->tot_len - ETH_PAD_SIZE, ETH_PAD_SIZE);

  int i = rndis().send(packet, p->tot_len - ETH_PAD_SIZE);
  free(packet);
  return (i >= 0) ? ERR_OK : errno;
}

size_t driver_get_mtu() {
  return 1500;
}

size_t driver_get_max_frame_len() {
  return 1580 - 44;
}

#if QNETHERNET_ENABLE_RAW_FRAME_SUPPORT
bool driver_output_frame(const uint8_t *frame, size_t len) {
  puts("driver_output_frame");
  return false;
}
#endif

#if !QNETHERNET_ENABLE_PROMISCUOUS_MODE
bool driver_set_mac_address_allowed(const uint8_t mac[ETH_HWADDR_LEN], bool allow) {
  puts("driver_set_mac_address_allowed");
  return false;
}
#endif

// these functions don't seem to be used right now...
bool driver_is_unknown() { puts("driver_is_unknown"); return false; }
bool driver_is_link_state_detectable() { puts("driver_is_link_state_detectable"); return true; }
int driver_link_speed() { puts("driver_link_speed"); return 0; }
bool driver_link_is_full_duplex() { puts("driver_link_is_full_duplex"); return true; }
bool driver_link_is_crossover() { puts("driver_link_is_crossover"); return false; }

}

#endif
