#include "../include/simulator.h"
#include <cstdio>
#include <queue>
#include <cstring>
#include<map>
#include <string>
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

//std::map<std::string, std::make_pair(int, int)> state;
std::queue<pkt> buffer;
enum sequence_number{start, A,B};
sequence_number seq, pre_seq;
enum state {SUCCESSFUL, CORRUPT, LOST};
struct pkt current_paket;
state InputB, InputAIN;

int checksum(struct pkt packet){
//    printf("checksum: ack no %d seq no  %d payload %s\n", packet.acknum, packet.seqnum, packet.payload);
    int n = sizeof(packet.payload)/sizeof(packet.payload[0]);
    int sum = 0;
    for(int i =0; i<n; i++){
        sum+= (int)packet.payload[i];
    }
    sum += packet.seqnum + packet.acknum;
    return sum;
}

struct pkt create_packet(struct msg message, sequence_number seq_){
    struct pkt new_pkt;
    new_pkt.seqnum = 0 ? seq_ == A : 1;
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

sequence_number change_seq(sequence_number s){
    if(s==A)
        return B;
    return A;
}




/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{

    if(seq == start){
        seq = A;
        pre_seq = A;
        struct pkt packet = create_packet(message, seq);
        current_paket = packet;
//        printf("Starting packet: %s seq :\n", packet.payload, "A");
        tolayer3(0 , packet);
        starttimer(0 ,20);
    }else{

        if(InputB == SUCCESSFUL && InputAIN == SUCCESSFUL){
            struct pkt packet = create_packet(message, seq);
            buffer.push(packet);
            if(buffer.size()!=0){
                InputAIN = LOST;
                InputB = LOST;
                struct pkt p = buffer.front();
                current_paket = p;
//                char k = 'A'? seq == A: 'B';
//                printf("Sending to InputB packet: %s seq %c:\n", packet.payload, k);
                tolayer3(0 , p);
                starttimer(0, 20);
                buffer.pop();
            }
        } else{
            seq = change_seq(seq);
            struct pkt packet = create_packet(message, seq);
//            char k = 'A'? seq == A: 'B';
//            printf("Buffered packet: %s seq %c:\n", packet.payload, k);
            buffer.push(packet);
        }
    }


}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    if(check_corruption(packet, checksum(packet))){
        stoptimer(0);
        seq = change_seq(seq);
        InputAIN = SUCCESSFUL;
        if(buffer.size()!=0){
            InputAIN = LOST;
            InputB = LOST;
            struct pkt p = buffer.front();
            current_paket = p;
//            char k = 'A'? seq == A: 'B';
//            printf("Sending to InputB from AInput packet: %s seq %c:\n", p.payload, k);
            tolayer3(0 , p);
            starttimer(0, 20);
            buffer.pop();
        }
    }else{
//        char k = 'A'? seq == A: 'B';
//        printf("Input AIN corrupt packet: %s seq %c:\n", packet.payload, k);

        InputAIN = CORRUPT;
    }

}

/* called when A's timer goes off */
void A_timerinterrupt()
{
//    char k = 'A'? seq == A: 'B';
//    printf("Timedout so sending packet: %s seq %c:\n", current_paket.payload, k);
    tolayer3(0, current_paket);
    starttimer(0, 20);

}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    seq = start;
     InputAIN = LOST;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) {
    if (check_corruption(packet, checksum(packet))) {
        if(InputB == LOST || InputB == CORRUPT){
            InputB = SUCCESSFUL;
//            char k = 'A'? seq == A: 'B';
//            printf("sending to Layer 5 packet: %s seq %c:\n", packet.payload, k);
            tolayer5(1, packet.payload);
        }
//        char k = 'A'? seq == A: 'B';
//        printf("sending to Layer 3 packet: %s seq %c:\n", packet.payload, k);
        tolayer3(1, packet);
    }else{
//        char k = 'A'? seq == A: 'B';

//        printf("Input B corrupt packet: %s seq %c:\n", current_paket.payload, k);
        if(InputB != SUCCESSFUL){
            InputB = CORRUPT;
        }
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
     InputB = LOST;
}
