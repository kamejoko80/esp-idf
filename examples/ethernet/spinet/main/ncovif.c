/* Network Connection Over Virtual Interface

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "esp_log.h"
#include "lwip/inet.h"
#include "ncovif.h"
#include "spinet.h"

static const char *TAG = "ncovif";

uint8_t  my_macaddr[6] = {0x51, 0x80, 0x21, 0xfe, 0xad, 0xd2};
uint8_t  my_ipaddr[4]  = {192, 168, 2, 2};

uint8_t  ex_macaddr[6] = {0xA6, 0x99, 0x91, 0xAD, 0x9D, 0x6F};
uint8_t  ex_ipaddr[4]  = {192, 168, 2, 1};

struct ethIIhdr eth_frame_dst;

volatile bool arp_request = false;
volatile bool icmp_request = false;

extern void emac_spinet_task_wake(emac_spinet_t *emac);

void eth_header_print(struct ethIIhdr *frame);
void arp_header_print(struct arphdr *arp);
int icmp_build_package(struct iphdr *sender_iph, uint8_t *icmpd, uint16_t len);
uint8_t* ip_output_standalone(struct ethIIhdr *eth, uint8_t protocol, uint16_t ip_id,
								uint32_t saddr, uint32_t daddr, uint16_t payloadlen);

void ipc_network_input(emac_spinet_t *emac, uint8_t *data, uint16_t len)
{
	uint8_t *buffer = NULL;
    uint32_t length = 0;
	
    length = len;
    buffer = heap_caps_malloc(length, MALLOC_CAP_DMA);

    if (!buffer) {
        ESP_LOGE(TAG, "no mem for receive buffer");
    } else {
        /* pass the buffer to stack (e.g. TCP/IP layer) */
        if (length) {
			memcpy(buffer, data, length);
            emac->eth->stack_input(emac->eth, buffer, length);
        } else {
            free(buffer);
        }
    }	
}

void send_icmp_echo(emac_spinet_t *emac)
{
	if(icmp_request){
		ipc_network_input(emac, (uint8_t *)&eth_frame_dst, 98);
		icmp_request = false;
	}
}

void send_arp_response(emac_spinet_t *emac)
{
	if(arp_request)
	{
		ESP_LOGI(TAG, "send_arp_response");
		ipc_network_input(emac, (uint8_t *)&eth_frame_dst, 42);
		arp_request = false;
	}
}

void build_arp_reply_package(struct ethIIhdr *eth_frame_dst, struct ethIIhdr *eth_frame_src)
{
	struct arphdr *arp_dest = (struct arphdr *)eth_frame_dst->data;
	struct arphdr *arp_src  = (struct arphdr *)eth_frame_src->data;

	/* build response message */
	memcpy(eth_frame_dst->h_source, my_macaddr, 6);
	memcpy(eth_frame_dst->h_dest, eth_frame_src->h_source, 6);
	eth_frame_dst->h_proto = eth_frame_src->h_proto;

	arp_dest->ar_hrd = arp_src->ar_hrd;
	arp_dest->ar_pro = arp_src->ar_pro;
	arp_dest->ar_hln = arp_src->ar_hln;
	arp_dest->ar_pln = arp_src->ar_pln;
	arp_dest->ar_op = __swap16(ARPOP_REPLY);
	memcpy(arp_dest->ar_sha, my_macaddr, 6);
	memcpy(arp_dest->ar_sip, my_ipaddr, 4);
	memcpy(arp_dest->ar_tha, arp_src->ar_sha, 6);
	memcpy(arp_dest->ar_tip, arp_src->ar_sip , 4);
}

void arp_frame_process(emac_spinet_t *emac, struct ethIIhdr *eth_frame)
{
	struct arphdr *arp_frame = (struct arphdr *)eth_frame->data;

	switch (arp_frame->ar_op){
		case __swap16(ARPOP_REQUEST):
			build_arp_reply_package(&eth_frame_dst, eth_frame);
			arp_request = true;
			emac_spinet_task_wake(emac);
		break;

		case __swap16(ARPOP_REPLY):
		break;

		default:
		break;
	}
}

void icmp_frame_process(emac_spinet_t *emac, struct ethIIhdr *eth_frame)
{
	struct iphdr *iph = (struct iphdr *)eth_frame->data;
	struct icmphdr *icmph = (struct icmphdr *) IP_NEXT_PTR(iph);

	uint16_t len, data_len;

	switch (icmph->icmp_type){
		case ICMP_ECHO:
			len = ntohs(iph->tot_len);
			data_len = (uint16_t)(len-(iph->ihl<<2)-sizeof(struct icmphdr));
			icmp_build_package(iph, (uint8_t *)(icmph + 1), data_len);
			icmp_request = true;
			emac_spinet_task_wake(emac);
		break;

		default:

		break;
	}
}

void ip_frame_process(emac_spinet_t *emac, struct ethIIhdr *eth_frame)
{
	struct iphdr *ip_frame = (struct iphdr *)eth_frame->data;

	switch(ip_frame->protocol){
		case IPPROTO_ICMP:
			icmp_frame_process(emac, eth_frame);
		break;
		default:
		break;
	}
}

void eth_frame_process(emac_spinet_t *emac, struct ethIIhdr *eth_frame)
{
	switch (eth_frame->h_proto){
		case __swap16(ETH_P_ARP):
			arp_frame_process(emac, eth_frame);
		break;

		case __swap16(ETH_P_IP):
			ip_frame_process(emac, eth_frame);
		break;

		default:

		break;
	}
}

uint16_t icmp_checksum(uint16_t *icmph, int len)
{
	uint16_t ret = 0;
	uint32_t sum = 0;
	uint16_t odd_byte;

	while (len > 1) {
		sum += *icmph++;
		len -= 2;
	}

	if (len == 1) {
		*(uint8_t*)(&odd_byte) = *(uint8_t*)icmph;
		sum += odd_byte;
	}

	sum =  (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	ret =  ~sum;

	return ret;
}

uint16_t csum_fold(uint32_t csum)
{
	uint32_t sum = (uint32_t)csum;;

	sum += (sum << 16);
	csum = (sum < csum);
	sum >>= 16;
	sum += csum;

	return (uint16_t)~sum;
}

uint16_t ip_fast_csum(const void *iph, unsigned int ihl)
{
	const unsigned int *word = iph;
	const unsigned int *stop = word + ihl;
	unsigned int csum;
	int carry;

	csum = word[0];
	csum += word[1];
	carry = (csum < word[1]);
	csum += carry;

	csum += word[2];
	carry = (csum < word[2]);
	csum += carry;

	csum += word[3];
	carry = (csum < word[3]);
	csum += carry;

	word += 4;
	do {
		csum += *word;
		carry = (csum < *word);
		csum += carry;
		word++;
	} while (word != stop);

	return csum_fold(csum);
}

uint8_t* ip_output_standalone(struct ethIIhdr *eth, uint8_t protocol, uint16_t ip_id,
		                      uint32_t saddr, uint32_t daddr, uint16_t payloadlen)
{
	struct iphdr *iph = (struct iphdr *)eth->data;

	/* build up eth header */
	memcpy(eth->h_source, my_macaddr, 6);
	memcpy(eth->h_dest, ex_macaddr, 6);
	eth->h_proto = __swap16(ETH_P_IP);

	/* build up ip header */
	iph->ihl = IP_HEADER_LEN >> 2;
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = htons(IP_HEADER_LEN + payloadlen);
	iph->id = ip_id;
	iph->frag_off = htons(IP_DF);
	iph->ttl = 64;
	iph->protocol = protocol;
	iph->saddr = saddr;
	iph->daddr = daddr;
	iph->check = 0;

	/* calculate checksum */
	iph->check = ip_fast_csum(iph, iph->ihl);

	return (uint8_t *)(iph + 1);
}

int icmp_build_package(struct iphdr *sender_iph, uint8_t *icmpd, uint16_t len)
{
	struct icmphdr *icmph;
	struct icmphdr *sender_icmph = (struct icmphdr *) IP_NEXT_PTR(sender_iph);

	icmph = (struct icmphdr *)ip_output_standalone(&eth_frame_dst,
							IPPROTO_ICMP,
							sender_iph->id,
							sender_iph->daddr,
							sender_iph->saddr,
							sizeof(struct icmphdr) + len);
	if (!icmph)
		return -1;

	/* Fill in the icmp echo reply header */
	icmph->icmp_type = ICMP_ECHOREPLY;
	icmph->icmp_code = sender_icmph->icmp_code;
	icmph->icmp_checksum = 0;

	icmph->un.echo.icmp_id = sender_icmph->un.echo.icmp_id;
	icmph->un.echo.icmp_sequence = sender_icmph->un.echo.icmp_sequence;

	/* Fill in the icmp data */
	if (len > 0){
		memcpy((void *)(icmph + 1), icmpd, len);
	}

	/* Calculate ICMP Checksum with header and data */
	icmph->icmp_checksum = icmp_checksum((uint16_t *)icmph, sizeof(struct icmphdr) + len);

	return 0;
}


