BEGIN {
	highest_packet_id = 0;
        data = 0;
	timeTot = 0;
	data1= 0;
	timeTot1 = 0;
}

{
	action = $1; 
 	time = $2; 
 	node_1 = $3; 
 	node_2 = $4; 
 	type = $5; 
 	size =$6;
 	flow_id = $8; 
 	node_1_address = $9; 
 	node_2_address = $10; 
 	seq_no = $11; 
 	packet_id = $12;
	
	if ( packet_id > highest_packet_id ) highest_packet_id = packet_id; 

	if ( action != "d" ) { 
 		if ( action == "r" && (node_2 == 2)&& (node_2_address == 2.0 ) ){ 
			#tcp
			data = data + size*8/10^6 ;
	 		timeTot = time
	 	} 
		if ( action == "r" && (node_2 == 2)&& (node_2_address == 2.1 )) { 
			#udp
			data1 = data1 + size*8/10^6 ;
	 		timeTot1 = time
	 	} 
	}
  
}

END{
	printf("Avg TCP throughput : %f MBits/sec\n",data/timeTot)
	printf("Avg UDP throughput : %f Mbits/sec\n",data1/timeTot1)
}

