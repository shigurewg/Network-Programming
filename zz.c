//
//  read-from-file.c
//  for http://qbsuranalang.blogspot.com
//  Created by TUTU on 2016/11/05.
//
//  Read frames from file.
//

#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include<time.h>
#include <arpa/inet.h>
#include<string.h>
#define MAC_ADDRSTRLEN 2*6+5+1

struct my_header{
    unsigned char dhost[6];
    unsigned char shost[6];
    unsigned short type;
};

int main(int argc, const char * argv[]) {
    char errbuf[PCAP_ERRBUF_SIZE];
    //const char *filename = "file.pcap";
    if(argc!=3) return 0;
    pcap_t *handle = pcap_open_offline(argv[2], errbuf);
    if(!handle) {
        fprintf(stderr, "pcap_open_offline(): %s\n", errbuf);
        exit(1);
    }//end if
    
    printf("Open: %s\n", /*filename*/argv[2]);
    int k,m;
    int total_amount = 0;
    int total_bytes = 0;
    int total_ip=0;
    unsigned int long_of_IP_packet,srcPort,dstPort,*tmp_ui;
    struct tm *ltime;
    unsigned char *tmp_uc;
    char timestr[16],char_ui[4],char_us[2];
    unsigned short protocol,*tmp_us;
    time_t local_tv_sec;
    char dst_mac[MAC_ADDRSTRLEN] = {0};
    char src_mac[MAC_ADDRSTRLEN] = {0};
    u_int16_t type_packet;
    struct my_header *head;
    struct pcap_pkthdr *header = NULL;
    const u_char *content = NULL;
    while(1) {
    //for(m=0;m<5;m++){
        header =NULL;
        content = NULL;
        int ret =
        pcap_next_ex(handle, &header, &content);
        if(ret == 1) { 
            total_amount++;
            printf("Packet number %d:\n",total_amount);
            total_bytes += header->caplen;
            printf("      Length: %d bytes\n",header->caplen);
            local_tv_sec = header->ts.tv_sec;
            ltime = localtime(&local_tv_sec);
            strftime(timestr, sizeof timestr, "%H:%M:%S", ltime);
            printf("\tTime: %s\n", timestr);
            head = (struct myheader *)content;
            type_packet = ntohs(head->type);
            printf("\ttype: %d\n",type_packet);
            printf("\tmacs: ");
            for(k=0;k<6;k++){
                printf("%02x ",*(head->shost+k));
            }
            printf("\n\tmacd: ");
            for(k=0;k<6;k++){
                printf("%02x ",*(head->dhost+k));
            }
            printf("\n");
            switch(type_packet){
                case(0x800):
                    total_ip++;
                    inet_ntop(AF_INET,(content+4*3+14),src_mac,INET_ADDRSTRLEN);
                    printf("\tsrcP: %s\n",src_mac);
                    inet_ntop(AF_INET,(content+4*4+14),dst_mac,INET_ADDRSTRLEN);
                    printf("\tdstP: %s\n",dst_mac);
                    tmp_us = (content+14+2);
                    long_of_IP_packet=ntohs(*tmp_us);
                    tmp_uc = content+14+9;
                    protocol=(*tmp_uc);
                        switch(protocol){
                            case(6):
                                tmp_us = content+14+4*5;
                                srcPort=htons(*tmp_us);
                                tmp_us = content+14+4*5+2;
                                dstPort=htons(*tmp_us);
                                printf("\tsPrt: %d\n",srcPort);
                                printf("\tdPrt: %d\n",dstPort);
                                printf("\tPrtT: TDP\n");
                                break;
                            case(17):
                                tmp_us = content+14+4*5;
                                srcPort=htons(*tmp_us);
                                tmp_us = content+14+4*5+2;
                                dstPort=htons(*tmp_us);
                                printf("\tsPrt: %d\n",srcPort);
                                printf("\tdPrt: %d\n",dstPort);
                                printf("\tPrtT: UDP\n");
                                break;
                            default:
                                printf("\tptcl: Unknown\n");
                                break;
                        }
                    break;
                case(0x86dd)://IPv6
                    total_ip++;
                    inet_ntop(AF_INET6,(content+4*2+14),src_mac,INET6_ADDRSTRLEN);
                    printf("\tsrcP: %s\n",src_mac);
                    inet_ntop(AF_INET6,(content+4*2+14+16),dst_mac,INET6_ADDRSTRLEN);
                    printf("\tdstP: %s\n",dst_mac);
                    //tmp_us = (content+14+2);
                    //long_of_IP_packet=ntohs(*tmp_us);
                    tmp_uc = content+14+6;
                    protocol=(*tmp_uc);
                        switch(protocol){
                            case(6):
                                tmp_us = content+14+4*2+16*2;
                                srcPort=htons(*tmp_us);
                                tmp_us = content+14+4*2+16*2+2;
                                dstPort=htons(*tmp_us);
                                printf("\tsPrt: %d\n",srcPort);
                                printf("\tdPrt: %d\n",dstPort);
                                printf("\tPrtT: TDP\n");
                                break;
                            case(17):
                                tmp_us = content+14+4*2+16*2;
                                srcPort=htons(*tmp_us);
                                tmp_us = content+14+4*2+16*2+2;
                                dstPort=htons(*tmp_us);
                                printf("\tsPrt: %d\n",srcPort);
                                printf("\tdPrt: %d\n",dstPort);
                                printf("\tPrtT: UDP\n");
                                break;
                            default:
                                printf("\tptcl: Unknown\n");
                                break;
                        }
                    break;
                default:
                    printf("\ttype: unknown\n");
                    break;
            }
        }//end if success
        else if(ret == 0) {
            printf("Timeout\n");
        }//end if timeout
        else if(ret == -1) {
            fprintf(stderr, "pcap_next_ex(): %s\n", pcap_geterr(handle));
        }//end if fail
        else if(ret == -2) {
            //printf("No more packet from file\n");
            break;
        }//end if read no more packet
    }//end while

    //result
    printf("Number of IP: %d\n",total_ip);
    printf("Number of Pc: %d\n",total_amount);
    //printf("Read: %d, byte: %d bytes\n", total_amount, total_bytes);

    //free
    pcap_close(handle);
    
    return 0;
}//end main