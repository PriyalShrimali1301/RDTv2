#include "../include/simulator.h"
#include <limits>
#include <cstdio>
#include <cstring>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
enum sender_type {SENT, NOT_SENT, NA};
enum receiver_type {N, RECEIVED};
enum state {SUCCESS, LOST, CORRUPT};
struct node{
    struct pkt packet;
    sender_type sender = NA;
    receiver_type receiver = N;
    state InputB = LOST;
    state InputAIN = LOST;
};
node buffer[1000];
int send_base;
int rcv_base;
int window_size;
int timeout_array[1000];
int counter;
int max_int;
int z_counter=0;
int checksum(struct pkt packet){
    int n = sizeof(packet.payload)/sizeof(packet.payload[0]);
    int sum = 0;
    for(int i =0; i<n; i++){
        sum+= (int)packet.payload[i];
    }
    sum += packet.seqnum + packet.acknum;
    return sum;
}

struct pkt create_packet(struct msg message){
    struct pkt new_pkt;
    new_pkt.seqnum = counter;
    new_pkt.acknum = 0;
    memcpy(new_pkt.payload, message.data, sizeof(message.data));
    new_pkt.checksum = checksum(new_pkt);
    return new_pkt;
}

bool check_corruption(struct pkt packet, int check_sum){
//    printf("cheksum: %d, %d, seq: %d, payload: %s\n", packet.checksum, check_sum, packet.seqnum, packet.payload);
//    printf("ack no: %d, %d\n", packet.acknum, check_sum);

    if(packet.checksum == check_sum){
        return true;
    }
    return false;
}

int get_min_timeout(){
    int min= 65590;
    int pos = 0;
    for(int i =0 ;i<counter; i++){
        if(timeout_array[i]<min){
            min = timeout_array[i];
            pos = i;
        }
    }
    return pos;
}

int timeout=0;
/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    struct pkt packet = create_packet(message);
    buffer[packet.seqnum].packet = packet;
    if(packet.seqnum >=send_base && packet.seqnum <=send_base+window_size-1){
//        printf("Sent to B %s\n",buffer[packet.seqnum].packet.payload);
        tolayer3(0,buffer[packet.seqnum].packet);
        z_counter++;
        if(timeout == 0){
            starttimer(0, 18);
            timeout = 1;
        }
        timeout_array[packet.seqnum] = get_sim_time();
        buffer[packet.seqnum].sender = SENT;
        counter++;
    }else{
//        printf("Buffered %s\n",buffer[packet.seqnum].packet.payload);
        buffer[packet.seqnum].packet = packet;
        buffer[packet.seqnum].sender = NOT_SENT;
        counter++;
    }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) {
    if(packet.seqnum <= counter && check_corruption(packet, checksum(packet))){
//        printf("Ainput %s\n",buffer[packet.seqnum].packet.payload);
        if(buffer[packet.seqnum].InputAIN != SUCCESS){
            buffer[packet.seqnum].InputAIN = SUCCESS;
            if(buffer[send_base].InputAIN == SUCCESS){
//                printf("timeout stopped:\n");
                stoptimer(0);
                timeout = 0;
                z_counter--;
                if(!(z_counter <= 0)){
//                    printf("Ye z counter mujhe jeene nahi deta starting timer\n");
                    timeout = 1;
                    starttimer(0, 18);
                }
            }
//            printf("Timeout for %s being set to %d\n",buffer[packet.seqnum].packet.payload, max_int);

            timeout_array[packet.seqnum] = max_int;
//            printf("SEND BASE before %d\n", send_base);
            int before = send_base;
            if(packet.seqnum == send_base){
                int q = packet.seqnum;
                while(buffer[q].InputAIN == SUCCESS){
                    timeout_array[q] = max_int;
                    q++;
                }
                send_base = q;
            }
//            printf("SEND BASE %d\n", send_base);
            for(int i = send_base; i<=send_base+window_size-1; i++){
                if(buffer[i].sender == NOT_SENT){
                    buffer[i].sender = SENT;
                    tolayer3(0, buffer[i].packet);
                    z_counter++;
//                    printf("Sent to B from AIN %s\n",buffer[i].packet.payload);
                    if(timeout == 0){
                        starttimer(0, 18);
                        timeout = 1;
                    }
                    timeout_array[i] = get_sim_time();
                }
            }
        }else{
            if(packet.seqnum <= counter)
                buffer[packet.seqnum].InputAIN = CORRUPT;
        }
        }


}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    timeout = 0;
    int n = get_min_timeout();
    if(buffer[n].InputAIN != SUCCESS){
//        printf("timedout so sending %s\n",buffer[n].packet.payload);
        tolayer3(0, buffer[n].packet);
        starttimer(0, 18);
        timeout_array[n] = get_sim_time();
        timeout = 1;
    }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    send_base = 0;
    rcv_base = 0;
    window_size = getwinsize();
    counter = 0;
    max_int = std::numeric_limits<int>::max();
    for(int i = 0; i<1000; i++){
        timeout_array[i] = -1;
    }
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{   /*printf("This is my packet %s %d counter\n", packet.payload, packet.seqnum);*/
    if(packet.seqnum <= counter && check_corruption(packet, checksum(packet))){
        if(buffer[packet.seqnum].InputB == LOST || buffer[packet.seqnum].InputB == CORRUPT){
            buffer[packet.seqnum].InputB = SUCCESS;
            buffer[packet.seqnum].receiver = RECEIVED;
            if(packet.seqnum == rcv_base){
                int q = packet.seqnum;
               // while(buffer[q].receiver == RECEIVED){
//                    printf("Sent counter : %d\n", q);
//                    printf("to Layer 5 %s\n",buffer[q].packet.payload);
                    tolayer5(1, buffer[q].packet.payload);
                    q++;
               // }
                rcv_base = q;
            }
        }
        if(buffer[packet.seqnum].InputAIN != SUCCESS){
            tolayer3(1, packet);
            timeout_array[packet.seqnum] = get_sim_time();
            z_counter++;
        }
//        printf("to Layer 1 %s\n",buffer[packet.seqnum].packet.payload);


    }else{
        if(packet.seqnum <= counter && buffer[packet.seqnum].InputB != SUCCESS){
            buffer[packet.seqnum].InputB = CORRUPT;
        }
    }

}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{

}
